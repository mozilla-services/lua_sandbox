-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"
require "math"
require "circular_buffer"

data = circular_buffer.new(3, 3, 1)
local ADD_COL = data:set_header(1, "Add column")
local SET_COL = data:set_header(2, "Set column", "count")
local GET_COL = data:set_header(3, "Get column", "count", "sum")

function process(ts)
    if data:add(ts, ADD_COL, 1) then
        data:set(ts, GET_COL, data:get(ts, ADD_COL))
    end
    data:set(ts, SET_COL, 1)
    return 0
end

function report(tc)
    if tc == 0 then
        output(data)
        write()
    elseif tc == 1 then
        cbufs = {}
        for i=1,3,1 do
            cbufs[i] = circular_buffer.new(2,1,1)
            cbufs[i]:set_header(1, "Header_1", "count")
        end
    elseif tc == 2 then
        write(cbufs[1])
    elseif tc == 3 then
        local stats = circular_buffer.new(5, 1, 1)
        stats:set(1e9, 1, 1)
        stats:set(2e9, 1, 2)
        stats:set(3e9, 1, 3)
        stats:set(4e9, 1, 4)
        local t, c = stats:compute("sum", 1)
        if 10 ~= t then
            error(string.format("no range sum = %G", t))
        end
        if 4 ~= c then
            error(sting.format("active_rows = %d", c))
        end
        t, c = stats:compute("avg", 1)
        if 2.5 ~= t then
            error(string.format("no range avg = %G", t))
        end
        if 4 ~= c then
            error(sting.format("active_rows = %d", c))
        end
        t, c = stats:compute("sd", 1)
        if math.abs(math.sqrt(1.25)) ~= t then
            error(string.format("no range sd = %G", t))
        end
        if 4 ~= c then
            error(sting.format("active_rows = %d", c))
        end
        t, c = stats:compute("min", 1)
        if 1 ~= t then
            error(string.format("no range min = %G", t))
        end
        if 4 ~= c then
            error(sting.format("active_rows = %d", c))
        end
        t, c = stats:compute("max", 1)
        if 4 ~= t then
            error(string.format("no range max = %G", t))
        end
        if 4 ~= c then
            error(sting.format("active_rows = %d", c))
        end

        t = stats:compute("sum", 1, 3e9, 4e9)
        if 7 ~= t then
            error(string.format("range 3-4 sum = %G", t))
        end
        t = stats:compute("avg", 1, 3e9, 4e9)
        if 3.5 ~= t then
            error(string.format("range 3-4 avg = %G", t))
        end
        t = stats:compute("sd", 1, 3e9, 4e9)
        if math.sqrt(0.25) ~= t then
            error(string.format("range 3-4 sd = %G", t))
        end

        t = stats:compute("sum", 1, 3e9)
        if 7 ~= t then
            error(string.format("range 3- = %G", t))
        end
        t = stats:compute("sum", 1, 3e9, nil)
        if 7 ~= t then
            error(string.format("range 3-nil = %G", t))
        end
        t = stats:compute("sum", 1, nil, 2e9)
        if 3 ~= t then
            error(string.format("range nil-2 sum = %G", t))
        end
        t = stats:compute("sum", 1, 11e9, 14e9)
        if nil ~= t then
            error(string.format("out of range = %G", t))
        end
    elseif tc == 4 then
        local stats = circular_buffer.new(4, 1, 1)
        stats:set(1e9, 1, 0/0)
        stats:set(2e9, 1, 8)
        stats:set(3e9, 1, 8)
        local t = stats:compute("avg", 1)
        if 8 ~= t then
            error(string.format("no range avg = %G", t))
        end
    elseif tc == 5 then
        local stats = circular_buffer.new(2, 1, 1)
        local nan = stats:get(0, 1)
        if nan == nan then
            error(string.format("initial value is a number %G", nan))
        end
        local v = stats:set(0, 1, 1)
        if v ~= 1 then
            error(string.format("set failed = %G", v))
        end
        v = stats:add(0, 1, 0/0)
        if v == v then
            error(string.format("adding nan returned a number %G", v))
        end
    elseif tc == 6 then
        local stats = circular_buffer.new(2, 1, 1)
        local cbuf_time = stats:current_time()
        if cbuf_time ~= 1e9 then
            error(string.format("current_time = %G", cbuf_time))
        end
        local v = stats:set(0, 1, 1)
        if stats:get(0, 1) ~= 1 then
            error(string.format("set failed = %G", v))
        end
        stats:fromstring("1 1 nan 99")
        local nan = stats:get(0, 1)
        if nan == nan then
            error(string.format("restored value is a number %G", nan))
        end
        v = stats:get(1e9, 1)
        if v ~= 99 then
            error(string.format("restored value is %G", v))
        end
    elseif tc == 7 then
        local empty = circular_buffer.new(4,1,1)
        local nan = empty:compute("avg", 1)
        if nan == nan then
            error(string.format("avg is a number %G", nan))
        end
        nan = empty:compute("sd", 1)
        if nan == nan then
            error(string.format("std is a number %G", nan))
        end
        nan = empty:compute("max", 1)
        if nan == nan then
            error(string.format("max is a number %G", m))
        end
        nan = empty:compute("min", 1)
        if nan == nan then
            error(string.format("min is a number %G", m))
        end

    end
end
