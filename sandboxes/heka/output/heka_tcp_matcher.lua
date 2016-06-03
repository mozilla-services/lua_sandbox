-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Heka Compatible TCP Output with Dynamic Matching

Used for generating a debug stream of messages for as long as the connection is
maintained.

## Sample Configuration
```lua
filename = "heka_tcp_matcher.lua"
instruction_limit = 0
messsage_matcher = "TRUE"
ticker_interval = 1

address = "127.0.0.1"
port    = 5566

ssl_params = {
  mode = "server",
  protocol = "tlsv1",
  key = "/etc/hindsight/certs/serverkey.pem",
  certificate = "/etc/hindsight/certs/server.pem",
  cafile = "/etc/hindsight/certs/CA.pem",
  verify = {"peer", "fail_if_no_peer_cert"},
  options = {"all", "no_sslv3"}
}
```
--]]

require "string"
local socket = require "socket"
require "table"

local address = read_config("address") or "127.0.0.1"
local port = read_config("port") or 5566
local ssl_params = read_config("ssl_params")
local ssl_ctx = nil
if ssl_params then
    require "ssl"
    ssl_ctx = assert(ssl.newcontext(ssl_params))
end
local server = assert(socket.bind(address, port))
server:settimeout(0)
local sockets = {server}
local subscribers = {}

local function send_message(socket, msg, i)
    local len, err, i = socket:send(msg, i)
    if not len then
        if err == "timeout" or err == "closed" then
            return false
        end
        return send_message(msg, i)
    end
    return true
end


local num_subs = 0
function process_message()
    local msg
    for i = num_subs, 1, -1 do
        local sub = subscribers[i]
        if sub[2]:eval() then
            if not msg then msg = read_message("framed") end
            if not send_message(sub[1], msg, 1) then
                sub[1]:close()
                table.remove(subscribers, i)
                num_subs = num_subs - 1
            end
        end
    end
    return 0
end


function timer_event(ns, shutdown)
    if shutdown then
        for i,v in ipairs(subscribers) do
            v[1]:close()
        end
        return
    end

    local ready = socket.select(sockets, nil, 0)
    if ready then
        for _, s in ipairs(ready) do
            local client = s:accept()
            if client then
                if ssl_ctx then
                    client = ssl.wrap(client, ssl_ctx)
                    client:dohandshake()
                end
                local exp, err = client:receive("*l")
                local ok, mm = pcall(create_message_matcher, exp)
                if ok then
                    num_subs = num_subs + 1
                    subscribers[num_subs] = {client, mm}
                else
                    print("bad matcher ", exp, mm)
                    local msg = encode_message(
                        {
                            Type = "bad matcher",
                            Payload = string.format("'%s' %s", tostring(exp), tostring(mm))
                        },
                        true)
                    send_message(client, msg, 1)
                    client:close()
                end
            end
        end
    end
end

