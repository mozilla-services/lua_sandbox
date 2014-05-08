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
        t, c = stats:compute("variance", 1)
        if 1.25 ~= t then
            error(string.format("no range variance = %G", t))
        end
        t, c = stats:compute("sd", 1)
        if math.sqrt(1.25) ~= t then
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
    elseif tc == 8 then
        local cb = circular_buffer.new(20,1,1)
        local u, p = cb:mannwhitneyu(1, 0e9, 9e9, 10e9, 19e9)
        if u or p then
            error("all the same values should return nil results")
        end
    elseif tc == 9 then -- default
        local cb = circular_buffer.new(40,1,1)
        local data = {15309,14092,13661,13412,14205,15042,14142,13820,14917,13953,14320,14472,15133,13790,14539,14129,14363,14202,13841,13610,13759,14428,14851,13838,13819,14468,14989,15557,14380,13500,14818,14632,13631,14663,14532,14188,14537,14109,13925,15022}
        for i,v in ipairs(data) do
            cb:set(i*1e9, 1, v)
        end
        local u, p = cb:mannwhitneyu(1, 1e9, 20e9, 21e9, 40e9)
        if u ~= 171 or math.floor(p * 100000) ~=  22037 then
            error(string.format("u is %g p is %g", u, p))
        end
    elseif tc == 10 then -- no continuity correction
        local cb = circular_buffer.new(40,1,1)
        local data = {15309,14092,13661,13412,14205,15042,14142,13820,14917,13953,14320,14472,15133,13790,14539,14129,14363,14202,13841,13610,13759,14428,14851,13838,13819,14468,14989,15557,14380,13500,14818,14632,13631,14663,14532,14188,14537,14109,13925,15022}
        for i,v in ipairs(data) do
            cb:set(i*1e9, 1, v)
        end
        local u, p = cb:mannwhitneyu(1, 1e9, 20e9, 21e9, 40e9, false)
        if u ~= 171 or math.floor(p * 100000) ~=  21638 then
            error(string.format("u is %g p is %g", u, p))
        end
    elseif tc == 11 then -- tie correction
        local cb = circular_buffer.new(40,1,1)
        local data = {15309,14092,13661,13412,14205,15042,14142,13820,14917,13953,14320,14472,15133,13790,14539,14129,14363,14202,13841,13610,13759,14428,14851,13838,13819,14468,14989,15557,14380,13500,14818,14632,13631,14663,14532,14188,14537,14109,13925,15309}
        for i,v in ipairs(data) do
            cb:set(i*1e9, 1, v)
        end
        local u, p = cb:mannwhitneyu(1, 1e9, 20e9, 21e9, 40e9)
        if u ~= 168.5 or math.floor(p * 100000) ~=  20084 then
            error(string.format("u is %g p is %g", u, p))
        end
    elseif tc == 12 then
        local cb = circular_buffer.new(40,1,1)
        local u, p = cb:mannwhitneyu(1, 41e9, 60e9, 61e9, 80e9)
        if u or p then
            error("times outside of buffer should return nil results")
        end
    elseif tc == 13 then
        local cb = circular_buffer.new(10,1,1)
        local data = {1,1,1,1,1,1,1,1,1,1}
        for i,v in ipairs(data) do
            cb:set(i*1e9, 1, v)
        end
        local u, p = cb:mannwhitneyu(1, 1e9, 5e9, 6e9, 10e9)
        if u or p then
            error("all the same values should return nil results")
        end
    elseif tc == 14 then
        local cb = circular_buffer.new(10,1,1)
        local rows, cols, spr = cb:get_configuration()
        assert(rows == 10, "invalid rows")
        assert(cols == 1 , "invalid columns")
        assert(spr  == 1 , "invalid seconds_per_row")
    elseif tc == 15 then
        local cb = circular_buffer.new(10,1,1)
        local args = {"widget", "count", "max"}
        local col = cb:set_header(1, args[1], args[2], args[3])
        assert(col == 1, "invalid column")
        local n, u, m = cb:get_header(col)
        assert(n == args[1], "invalid name")
        assert(u == args[2], "invalid unit")
        assert(m == args[3], "invalid aggregation_method")
    elseif tc == 16 then
        local cb = circular_buffer.new(10,1,1)
        assert(not cb:get(10*1e9, 1), "value found beyond the end of the buffer")
        cb:set(20*1e9, 1, 1)
        assert(not cb:get(10*1e9, 1), "value found beyond the start of the buffer")
    elseif tc == 17 then -- default
        local cb = circular_buffer.new(120,1,1)
        local data = {1,1,1,2,1,3,3,6,4,0/0,0/0,0/0,1,0/0,2,0/0,0/0,0/0,0/0,0/0,1,5,1,0/0,1,1,0/0,0/0,3,4,1,1,1,0/0,7,1,0/0,6,0/0,0/0,1,3,4,3,0/0,1,5,0/0,1,0/0,0/0,1,6,4,0/0,4,2,6,4,3,2,6,2,11,2,0/0,2,0/0,2,0/0,0/0,0/0,4,0/0,3,2,0/0,0/0,1,2,2,2,1,1,0/0,3,0/0,4,0/0,0/0,2,3,5,6,3,1,0/0,0/0,3,2,0/0,4,1,2,1,1,0/0,0/0,0/0,0/0,0/0,0/0,0/0,7,1,1,2,1,0/0,0/0}
        for i,v in ipairs(data) do
            cb:set(i*1e9, 1, v)
        end
        local u1 = cb:mannwhitneyu(1, 61e9, 120e9, 1e9, 60e9)
        local u2 = cb:mannwhitneyu(1, 1e9, 60e9, 61e9, 120e9)
        if u1 + u2 ~= 3600 then
            error(string.format("u1 is %g u2 is %g %g", u1, u2, maxu))
        end
    end
end
