-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Heka Protobuf Stream Input (stdin)

## Sample Configuration
```lua
filename = "heka_stdin.lua"
```
--]]

local stdin = require "io".stdin
require "string"

local hsr = create_stream_reader(read_config("Logger"))

function process_message()
    local cnt = 0
    local found, consumed, read
    repeat
        repeat
            found, consumed, read = hsr:find_message(stdin)
            if found then
                inject_message(hsr)
                cnt = cnt + 1
            end
        until not found
    until read == 0
    return 0, string.format("processed %d messages", cnt)
end
