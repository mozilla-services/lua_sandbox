-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"

function process(tc)
    if tc == 0  then
        local cb = circular_buffer.new(1000, 10, 1) -- new() too much memory
    end
    return 0
end
