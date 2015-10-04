-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "sax"

local SAX_N = 9
local SAX_W = 3
local SAX_C = 5

local data = {16,18,12,24,11,15,16,12,17}
local nan_data = {0/0,0/0,0/0,0/0,0/0,0/0,0/0,0/0,0/0}
local partial_inf_data = {16,0/0,12,1/0,11,15,-1/0,12,17}

unused = sax.window.new(SAX_N, SAX_W, SAX_C)
win = sax.window.new(SAX_N, SAX_W, SAX_C)
nan_win = sax.window.new(SAX_N, SAX_W, SAX_C)
partial_inf_win = sax.window.new(SAX_N, SAX_W, SAX_C)
mm = sax.window.new(100, 5, 8)
mm_tail = sax.window.new(100, 5, 8)

word = sax.word.new(data, SAX_W, SAX_C)
nan_word = sax.word.new("###", SAX_C)
w1 = word

assert(tostring(win) == "###")

function process()
    for i,v in ipairs(data) do
        win:add(v)
    end

    for i,v in ipairs(nan_data) do
        nan_win:add(v)
    end

    for i,v in ipairs(partial_inf_data) do
        partial_inf_win:add(v)
    end

    for i=1, 1000000 do
        mm:add(i-1)
    end

    for i=999001, 1000000 do
        mm_tail:add(i-1)
    end
    return 0
end

function report(tc)
    write_output(win, " ", word, " ", mm, " ", mm_tail)
end
