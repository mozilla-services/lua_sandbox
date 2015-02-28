-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"
require "cjson"
require "hyperloglog"

local cbuf = circular_buffer.new(1440, 3, 60)
local benchmark = {Timestamp = 1e9, Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}

function process(tc)
    if tc == 0 then -- lua types
        write_output(1.2, " string ", nil, " ", true, " ", false)
    elseif tc == 1 then -- cbuf
        write_output("annotation\n", cbuf)
    elseif tc == 2 then -- heka message
        local hm = {Uuid = "ignored", Timestamp = 1e9, Type="type", Logger="logger", Payload="payload", EnvVersion="env_version", Hostname="hostname", Severity=9}
        write_message(hm)
    elseif tc == 3 then -- heka message field
        local hm = {Timestamp = 1e9, Fields = {count=1}}
        write_message(hm)
    elseif tc == 4 then -- heka message field array
        local hm = {Timestamp = 1e9, Fields = {counts={2,3,4}}}
        write_message(hm)
    elseif tc == 5 then -- hekamessage field metadata
        local hm = {Timestamp = 1e9, Fields = {count={value=5,representation="count"}}}
        write_message(hm)
    elseif tc == 6 then -- heka message field metadata array
        local hm = {Timestamp = 1e9, Fields = {counts={value={6,7,8},representation="count"}}}
        write_message(hm)
    elseif tc == 7 then -- heka message field all types
        local hm = benchmark
        write_message(hm)
    elseif tc == 8 then -- heka message force memmove
        local hm = {Timestamp = 1e9, Fields = {string="0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"}}
        write_message(hm)
    elseif tc == 9 then -- heka negative values
        local hm = {Timestamp = -1, Pid = -1, Severity = -1}
        write_message(hm)
    elseif tc == 10 then -- heka varint encoding
        local hm = {Timestamp = 1e9, Fields = { count = {value=1, value_type=2, representation="integer"}}}
        write_message(hm)
    elseif tc == 11 then -- heka varint packed encoding
        local hm = {Timestamp = 1e9, Fields = { count = {value={1,2}, value_type=2, representation="integer"}}}
        write_message(hm)
    elseif tc == 12 then -- heka field array message
        local hm = {Timestamp = 1e9, Fields = {{name = "count", value=1, value_type=2, representation="integer"}}}
        write_message(hm)
    elseif tc == 13 then -- heka field array message multiple fields
        local hm = {Timestamp = 1e9, Fields = {{name = "count", value=1, value_type=2, representation="integer"}, {name = "count", value=2, value_type=2, representation="integer"}}}
        write_message(hm)
    elseif tc == 14 then -- heka field array message value defaults to a double
        local hm = {Timestamp = 1e9, Fields = {{name = "count", value=1, representation="double"}}}
        write_message(hm)
    elseif tc == 15 then -- heka field array message value defaults to a double packed
        local hm = {Timestamp = 1e9, Fields = {{name = "count", value={1,1}, representation="double"}}}
        write_message(hm)
    elseif tc == 16 then -- heka array of byte fields
        local hm = {Timestamp = 1e9, Fields = {{name = "names", value={"s1","s2"}, value_type = 1, representation="list"}}}
        write_message(hm)
    elseif tc == 17 then -- heka array of explicit string fields
        local hm = {Timestamp = 1e9, Fields = {{name = "names", value={"s1","s2"}, value_type = 0, representation="list"}}}
        write_message(hm)
    elseif tc == 18 then -- heka array of byte fields no representation
        local hm = {Timestamp = 1e9, Fields = {{name = "names", value={"s1","s2"}, value_type = 1}}}
        write_message(hm)
    elseif tc == 19 then
        local hll = hyperloglog.new()
        local hm = {Timestamp = 1e9, Fields = {hll = {value=hll, representation="hll"}}}
        write_message(hm)
    elseif tc == 20 then
        local cb = circular_buffer.new(2, 1, 60)
        local hm = {Timestamp = 1e9, Fields = {cb = {value=cb, representation="cbuf"}}}
        write_message(hm)
    end
    return 0
end
