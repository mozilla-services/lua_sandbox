-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# RST Heka Message Output

Writes a user friendly version (RST format) of the full Heka message to stdout

## Sample Configuration
```lua
filename        = "debug.lua"
message_matcher = "TRUE"
```
--]]

local write  = require "io".write
local flush  = require "io".flush
local concat = require "table".concat
local mi     = require "heka.msg_interpolate"

function process_message()
    local raw = read_message("raw")
    local msg = decode_message(raw)
    write(":Uuid: ", mi.get_uuid(msg.Uuid), "\n")
    write(":Timestamp: ", mi.get_timestamp(msg.Timestamp), "\n")
    write(":Type: ", msg.Type or "<nil>", "\n")
    write(":Logger: ", msg.Logger or "<nil>", "\n")
    write(":Severity: ", msg.Severity or 7, "\n")
    write(":Payload: ", msg.Payload or "<nil>", "\n")
    write(":EnvVersion: ", msg.EnvVersion or "<nil>", "\n")
    write(":Pid: ", msg.Pid or "<nil>", "\n")
    write(":Hostname: ", msg.Hostname or "<nil>", "\n")
    write(":Fields:\n")
    for i, v in ipairs(msg.Fields or {}) do
        write("    | name: ", v.name,
              " type: ", v.value_type or 0,
              " representation: ", v.representation or "<nil>",
              " value: ")
        if v.value_type == 4 then
            for j, w in ipairs(v.value) do
                if j ~= 1 then write(",") end
                if w then write("true") else write("false") end
            end
            write("\n")
        else
            write(concat(v.value, ","), "\n")
        end
    end
    write("\n")
    flush()
    return 0
end

function timer_event(ns)
    -- no op
end
