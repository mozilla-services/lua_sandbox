-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"

function process(tc)
    if tc == 0 then
        local cb = circular_buffer.new(2) -- new() incorrect # args
    elseif tc == 1 then
        local cb = circular_buffer.new(nil, 1, 1) -- new() non numeric row
    elseif tc == 2 then
        local cb = circular_buffer.new(1, 1, 1) -- new() 1 row
    elseif tc == 3 then
        local cb = circular_buffer.new(2, nil, 1) -- new() non numeric column
    elseif tc == 4 then
        local cb = circular_buffer.new(2, 0, 1) -- new() zero column
    elseif tc == 5  then
        local cb = circular_buffer.new(2, 1, nil) -- new() non numeric seconds_per_row
    elseif tc == 6 then
        local cb = circular_buffer.new(2, 1, 0) -- new() zero seconds_per_row
    elseif tc == 7 then
        local cb = circular_buffer.new(2, 1, 86401) -- new() > day seconds_per_row
    elseif tc == 8  then
        local cb = circular_buffer.new(1000, 10, 1) -- new() too much memory
    elseif tc == 9 then
        local cb = circular_buffer.new(2, 1, 1) -- set() out of range column
        cb:set(0, 2, 1.0)
    elseif tc == 10  then
        local cb = circular_buffer.new(2, 1, 1) -- set() zero column
        cb:set(0, 0, 1.0)
    elseif tc == 11 then
        local cb = circular_buffer.new(2, 1, 1) -- set() non numeric column
        cb:set(0, nil, 1.0)
    elseif tc == 12 then
        local cb = circular_buffer.new(2, 1, 1) -- set() non numeric time
        cb:set(nil, 1, 1.0)
    elseif tc == 13 then
        local cb = circular_buffer.new(2, 1, 1) -- get() invalid object
        local invalid = 1
        cb.get(invalid, 1, 1)
    elseif tc == 14 then
        local cb = circular_buffer.new(2, 1, 1) -- set() non numeric value
        cb:set(0, 1, nil)
    elseif tc == 15 then
        local cb = circular_buffer.new(2, 1, 1) -- set() incorrect # args
        cb:set(0)
    elseif tc == 16 then
        local cb = circular_buffer.new(2, 1, 1) -- add() incorrect # args
        cb:add(0)
    elseif tc == 17 then
        local cb = circular_buffer.new(2, 1, 1) -- get() incorrect # args
        cb:get(0)
    elseif tc == 18 then
        local cb = circular_buffer.new(2, 1, 1) -- compute() incorrect # args
        cb:compute(0)
    elseif tc == 19 then
        local cb = circular_buffer.new(2, 1, 1) -- compute() incorrect function
        cb:compute("func", 1)
    elseif tc == 20 then
        local cb = circular_buffer.new(2, 1, 1) -- compute() incorrect column
        cb:compute("sum", 0)
    elseif tc == 21 then
        local cb = circular_buffer.new(2, 1, 1) -- compute() start > end
        cb:compute("sum", 1, 2e9, 1e9)
    elseif tc == 22 then
        local cb = circular_buffer.new(2, 1, 1) -- format() invalid
        cb:format("invalid")
    elseif tc == 23  then
        local cb = circular_buffer.new(2, 1, 1) -- format() extra
        cb:format("cbuf", true)
    elseif tc == 24 then
        local cb = circular_buffer.new(2, 1, 1) -- format() missing
        cb:format()
    elseif tc == 25 then
        local cb = circular_buffer.new(2, 1, 1) -- too few
        cb:fromstring("")
    elseif tc == 26 then
        local cb = circular_buffer.new(2, 1, 1) -- too few invalid
        cb:fromstring("0 0 na 1")
    elseif tc == 27 then
        local cb = circular_buffer.new(2, 1, 1) -- too many
        cb:fromstring("0 0 1 2 3")
    elseif tc == 28 then
        local cb = circular_buffer.new(2, 1, 1)
        cb:mannwhitneyu() -- incorrect # args
    elseif tc == 29 then
        local cb = circular_buffer.new(2, 1, 1)
        cb:mannwhitneyu(nil, 0, 0, 0, 0) -- non numeric column
    elseif tc == 30 then
        local cb = circular_buffer.new(2, 1, 1)
        cb:mannwhitneyu(0, 0, 0, 0, 0) -- invalid column
    elseif tc == 31 then
        local cb = circular_buffer.new(2, 1, 1)
        cb:mannwhitneyu(1, 0, 5, 2, 7) -- overlapping x,y
    elseif tc == 32 then
        local cb = circular_buffer.new(2, 1, 1)
        cb:mannwhitneyu(1, 5, 0, 2, 7) -- inverted x
    elseif tc == 33 then
        local cb = circular_buffer.new(2, 1, 1)
        cb:mannwhitneyu(1, 0, 5, 10, 6) -- inverted y
    elseif tc == 34 then
        local cb = circular_buffer.new(2, 1, 1)
        cb:mannwhitneyu(1, 0, 1, 2, 3, true, 5) -- incorrect # args
    elseif tc == 35 then
        local cb = circular_buffer.new(10, 1, 1)
        cb:mannwhitneyu(1, 0, 5, 6, 10, "a") -- invalid use_continuity flag
    elseif tc == 36 then
        local cb = circular_buffer.new(10, 1, 1)
        cb:get_header() -- incorrect # args
    elseif tc == 37 then
        local cb = circular_buffer.new(10, 1, 1)
        cb:get_header(99) -- out of range column
    end
return 0
end
