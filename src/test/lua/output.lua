-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"

local cbuf = circular_buffer.new(1440, 3, 60)
local test = circular_buffer.new(2, 2, 60)

function process(tc)
    if tc == 0 then -- lua types
        write_output(1.2, " string ", nil, " ", true, " ", false)
    elseif tc == 1 then -- cbuf
        write_output(cbuf)
    elseif tc == 2 then
        test:annotate(0, 1, "info", "annotation\"\t\b\r\n\240 end")
        test:annotate(60e9, 2, "alert", "alert")
        test:annotate(120e9, 2, "info", "out of range, should be ignored")
        write_output(test)
    elseif tc == 3 then
        test:set(120e9, 1, 0/0) -- advance the buffer (pruning the annotation)
        write_output(test)
    end
    return 0
end
