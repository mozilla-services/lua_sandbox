-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Heka Compatible TCP Input

## Sample Configuration
```lua
filename = "heka_tcp.lua"
instruction_limit = 0

address = "127.0.0.1"
port    = 5565

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

require "coroutine"
local socket = require "socket"
require "string"
require "table"

local address = read_config("address") or "127.0.0.1"
local port = read_config("port") or 5565
local ssl_params = read_config("ssl_params")
local ssl_ctx = nil
if ssl_params then
    require "ssl"
    ssl_ctx = assert(ssl.newcontext(ssl_params))
end
local server = assert(socket.bind(address, port))
server:settimeout(0)
local threads = {}
local sockets = {server}

local function handle_client(client, caddr, cport)
    local found, consumed, need = false, 0, 8192 * 4
    local hsr = create_stream_reader(string.format("%s:%d -> %s:%d", caddr, cport, address, port))
    client:settimeout(0)
    while client do
        local buf, err, partial = client:receive(need)
        if partial then buf = partial end
        if not buf then break end

        repeat
            found, consumed, need = hsr:find_message(buf)
            if found then inject_message(hsr) end
            buf = nil
        until not found

        if err == "closed" then break end

        coroutine.yield()
    end
end

function process_message()
    while true do
        local ready = socket.select(sockets, nil, 1)
        if ready then
            for _, s in ipairs(ready) do
                if s == server then
                    local client = s:accept()
                    if client then
                        local caddr, cport = client:getpeername()
                        if not caddr then
                            caddr = "unknown"
                            cport = 0
                        end
                        if ssl_ctx then
                            client = ssl.wrap(client, ssl_ctx)
                            client:dohandshake()
                        end
                        sockets[#sockets + 1] = client
                        threads[client] = coroutine.create(
                            function() handle_client(client, caddr, cport) end)
                    end
                else
                    if threads[s] then
                        local status = coroutine.resume(threads[s])
                        if not status then
                            s:close()
                            for i = #sockets, 2, -1 do
                                if s == sockets[i] then
                                    table.remove(sockets, i)
                                    break
                                end
                            end
                            threads[s] = nil
                        end
                    end
                end
            end
        end
    end
    return 0
end
