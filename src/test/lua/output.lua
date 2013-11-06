-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"

local cbuf = circular_buffer.new(1440, 3, 60)
local simple_table = {value=1}
local metric = {MetricName="example",Timestamp=0,Unit="s",Value=0, 
Dimensions={{Name="d1",Value="v1"}, {Name="d2",Value="v2"}},
StatisticValues={{Maximum=0,Minimum=0,SampleCount=0,Sum= 0},{Maximum=0,Minimum=0,SampleCount=0,Sum=0}}}

function process(tc)
    if tc == 0 then -- lua types
        output(simple_table, 1.2, " string ", nil, " ", true, " ", false)
        write("txt")
    elseif tc == 1 then -- cbuf
        output("annotation\n")
        output(cbuf)
        write("acbuf", "stats")
    elseif tc == 2 then -- table
        output(metric)
        write("json", "stat metrics")
    elseif tc == 3 then -- external reference
        local a = {x = 1, y = 2}
        local b = {a = a}
        output(b)
        write()
    elseif tc == 4 then -- array only
        local a = {1,2,3}
        output(a)
        write()
    elseif tc == 5 then -- private keys
        local a = {x = 1, _m = 1, _private = {1,2}}
        output(a)
        write()
    elseif tc == 6 then -- table name
        local a = {1,2,3,_name="array"}
        output(a)
        write()
    elseif tc == 7 then -- global table
        output(_G)
        write()
    elseif tc == 8 then -- special characters
        output({['special\tcharacters'] = '"\t\r\n\b\f\\/'})
        write()
    elseif tc == 9 then -- heka message
        local hm = {Timestamp = 1e9, Type="type", Logger="logger", Payload="payload", EnvVersion="env_version", Hostname="hostname", Severity=9, }
        write(hm)
    elseif tc == 10 then -- heka message field
        local hm = {Timestamp = 1e9, Fields = {count=1}}
        write(hm)
    elseif tc == 11 then -- heka message field array
        local hm = {Timestamp = 1e9, Fields = {counts={2,3,4}}}
        write(hm)
    elseif tc == 12 then -- hekamessage field metadata
        local hm = {Timestamp = 1e9, Fields = {count={value=5,representation="count"}}}
        write(hm)
    elseif tc == 13 then -- heka message field metadata array
        local hm = {Timestamp = 1e9, Fields = {counts={value={6,7,8},representation="count"}}}
        write(hm)
    elseif tc == 14 then -- heka message field all types
        local hm = {Timestamp = 1e9, Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}
        write(hm)
    elseif tc == 15 then -- heka message force memmove
        local hm = {Timestamp = 1e9, Fields = {string="0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"}}
        write(hm)
    end
    return 0
end
