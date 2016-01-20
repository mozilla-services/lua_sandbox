-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"

local cbuf = circular_buffer.new(1440, 3, 60)

function process(tc)
    if tc == 0 then -- lua types
        write_output(1.2, " string ", nil, " ", true, " ", false)
    elseif tc == 1 then -- cbuf
        write_output("annotation\n", cbuf)
    end
    return 0
end
