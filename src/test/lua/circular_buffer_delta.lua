-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"

data = circular_buffer.new(3, 3, 1, true)
local ADD_COL = data:set_header(1, "Add column")
local SET_COL = data:set_header(2, "Set column", "count")
local GET_COL = data:set_header(3, "Get column", "count", "sum")

local cb = circular_buffer.new(2, 2, 1, true)
local SUM_COL = cb:set_header(1, "Sum column")
local MIN_COL = cb:set_header(2, "Min", "count", "min")

function process(ts)
    if data:add(ts, ADD_COL, 1) then
        data:set(ts, GET_COL, data:get(ts, ADD_COL))
    end
    data:set(ts, SET_COL, 1)
    return 0
end

function report(tc)
    if tc == 0 then
        write(data:format("cbuf"))
    elseif tc == 1 then
        write(data:format("cbufd"))
    elseif tc == 2 then
        local ts = 2e9
        if data:add(ts, ADD_COL, 0/0) then
            data:set(ts, GET_COL, data:get(ts, ADD_COL))
        end
        write(data:format("cbufd"))
    elseif tc == 3 then
        -- the sum delta should reflect the difference
        -- the min delta should reflect the current value
        cb:set(0, SUM_COL, 3)
        cb:set(0, MIN_COL, 3)
        write(cb:format("cbufd"))
        cb:set(0, SUM_COL, 5)
        cb:set(0, MIN_COL, 5)
        write(cb:format("cbufd"))
    elseif tc == 4 then
        cb:add(0, SUM_COL, 3)
        cb:add(0, MIN_COL, 3)
        write(cb:format("cbufd"))
    end
end
