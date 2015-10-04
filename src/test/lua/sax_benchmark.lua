-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "sax"

local data = {16,18,12,24,11,15,16,12,17,13,16,18,12,24,11,15,16,12,17,13,
    16,18,12,24,11,15,16,12,17,13,16,18,12,24,11,15,16,12,17,13,
    16,18,12,24,11,15,16,12,17,13,16,18,12,24,11,15,16,12,17,13,
    16,18,12,24,11,15,16,12,17,13,16,18,12,24,11,15,16,12,17,13,
    16,18,12,24,11,15,16,12,17,13,16,18,12,24,11,15,16,12,17,13}

win = sax.window.new(100, 5, 8)
win:add(data)

function process(ts)
    win:add(ts)
    return 0
end

function report(tc)
    write_output(tostring(win:get_word()))
end

