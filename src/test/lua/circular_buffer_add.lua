-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"

data = circular_buffer.new(1440, 10, 1)

function process(ts)
    data:add(ts, 4, 1)
    return 0
end
