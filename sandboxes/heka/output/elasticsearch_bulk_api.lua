-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Elasticsearch Bulk API Output

## Sample Configuration
```lua
filename        = "elasticsearch_bulk_api.lua"
message_matcher = "Type == 'nginx'"
ticker_interval = 10 -- flush every 10 seconds or flush_count (50000) messages
memory_limit    = 200e6

address             = "127.0.0.1"
port                = 9200
timeout             = 10
flush_count         = 50000
flush_on_shutdown   = false
preserve_data       = not flush_on_shutdown --in most cases this should be the inverse of flush_on_shutdown

-- See the elasticsearch module directory for the various encoders and configuration documentation.
encoder_module  = "heka.elasticsearch.moz_telemetry"
encoder_cfg     = {
    es_index_from_timestamp = true,
    index                   = "%{Logger}-%{%Y.%m.%d}",
    type_name               = "%{Type}-%{Hostname}",
    fields                  = {"Fields[request]", "Fields[http_user_agent]"},
}
```
--]]

require "table"
require "rjson"
require "string"
local ltn12     = require "ltn12"
local time      = require "os".time
local em        = require(read_config("encoder_module"))
local socket    = require "socket"
local http      = require("socket.http")
local address   = read_config("address") or "127.0.0.1"
local port      = read_config("port") or 9200
local timeout   = read_config("timeout") or 10

local batch_file        = string.format("%s/%s.batch", read_config("output_path"), read_config("Logger"))
local flush_on_shutdown = read_config("flush_on_shutdown")
local ticker_interval   = read_config("ticker_interval")
local flush_count       = read_config("flush_count") or 50000
local last_flush        = time()

local client
local function create_client()
    local client = http.open(address, port)
    client.c:setoption("tcp-nodelay", true)
    client.c:setoption("keepalive", true)
    client.c:settimeout(timeout)
    return client
end
local pcreate_client = socket.protect(create_client);


local req_headers = {
    ["user-agent"]      = http.USERAGENT,
    ["content-length"]  = 0,
    ["host"]            = address .. ":" .. port,
    ["accept"]          = "application/json",
    ["connection"]      = "keep-alive",
}

local function send_request() -- hand coded since socket.http doesn't support keep-alive connections
    local fh = assert(io.open(batch_file, "r"))
    req_headers["content-length"] = fh:seek("end")
    client:sendrequestline("POST", "/_bulk")
    client:sendheaders(req_headers)
    fh:seek("set")
    client:sendbody(req_headers, ltn12.source.file(fh, "invalid file handle"))
    local code = client:receivestatusline()
    local headers
    while code == 100 do -- ignore any 100-continue messages
        headers = client:receiveheaders()
        code = client:receivestatusline()
    end
    headers = client:receiveheaders()
    local ok, err, ret = true, nil, 0
    if code ~= 204 and code ~= 304 and not (code >= 100 and code < 200) then
        if code == 200 and string.match(headers["content-type"], "^application/json") then
            local body = {}
            local sink = ltn12.sink.table(body)
            client:receivebody(headers, sink)
            local response = table.concat(body)
            local ok, doc = pcall(rjson.parse, response)
            if ok then
                if doc:value(doc:find("errors")) then
                    ret = -1
                    err = string.format("ElasticSearch server reported errors processing the submission")
                end
            else
                ret = -3
                err = string.format("HTTP response didn't contain valid JSON. err: %s", doc)
            end
        else
            client:receivebody(headers, ltn12.sink.null())
        end

        if not err and code > 304 then
            ret = -1
            err = string.format("HTTP response error. Status: %d", code)
        end
    end

    if headers.connection == "close" then
        client:close()
        client = nil
    end

    return true, err, ret
end
local psend_request = socket.protect(function(client) return send_request(client) end)


local batch = assert(io.open(batch_file, "a+"))
local function send_batch()
    local err
    if not client then
        client, err = pcreate_client()
    end
    if err then return -3, err end -- retry indefinitely

    batch:flush()
    local ok, err, ret = psend_request(client)
    if not ok then -- network error
        client = nil
        return -3, err
    end
    last_flush = time()
    return ret, err
end

batch_count = 0
retry       = false

function process_message()
    if not retry then
        batch_count = batch_count + 1
        batch:write(em.encode())
    end

    if batch_count == flush_count then
        local ret, err = send_batch()
        if ret == 0  then
            retry = false
            batch_count = 0
            batch:close()
            batch = assert(io.open(batch_file, "w"))
        elseif ret == -3 then
            retry = true
            client = nil
        end
        return ret, err
    end
    return 0
end


function timer_event(ns, shutdown)
    local timedout = (ns / 1e9 - last_flush) >= ticker_interval
    if (timedout or (shutdown and flush_on_shutdown)) and batch_count > 0 then
        local ret, err = send_batch()
        if ret == 0 then
            retry = false
            batch_count = 0
            batch:close()
            if not shutdown then
                batch = assert(io.open(batch_file, "w"))
            end
        end
    end
end
