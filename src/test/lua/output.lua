-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"
require "cjson"

local cbuf = circular_buffer.new(1440, 3, 60)

function process(tc)
    if tc == 0 then -- lua types
        write_output(1.2, " string ", nil, " ", true, " ", false)
    elseif tc == 1 then -- cbuf
        write_output("annotation\n", cbuf)
    elseif tc == 2 then -- heka message
        local hm = {Timestamp = 1e9, Type="type", Logger="logger", Payload="payload", EnvVersion="env_version", Hostname="hostname", Severity=9, }
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
        local hm = {Timestamp = 1e9, Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}
        write_message(hm)
    elseif tc == 8 then -- heka message force memmove
        local hm = {Timestamp = 1e9, Fields = {string="0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"}}
        write_message(hm)
    end
    return 0
end
