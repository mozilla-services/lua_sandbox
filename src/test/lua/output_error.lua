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
    if tc == 1 then -- "error internal reference"
        local a = {x = {1,2,3}, y = {2}}
        a.ir = a.x
        output(a)
    elseif tc == 2 then -- error circular reference
        local a = {x = 1, y = 2}
        a.self = a
        output(a)
    elseif tc == 3 then -- error escape overflow
        local escape = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
        for i=1, 10 do
            escape = escape .. escape
        end
        output({escape = escape})
    elseif tc == 4 then -- heka error mis-match field array
        local hm = {Timestamp = 1e9, Fields = {counts={2,"ten",4}}}
        output(hm)
    elseif tc == 5 then -- heka error nil field
        local hm = {Timestamp = 1e9, Fields = {counts={}}}
        output(hm)
    end
    write()
    return 0
end
