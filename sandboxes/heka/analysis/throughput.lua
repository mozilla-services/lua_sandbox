-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Throughput Analysis

Simple message throughput counter/visualization.

## Sample Configuration
```lua
filename                = "throughput.lua"
message_matcher         = "TRUE"
ticker_interval         = 60
preserve_data           = true
preservation_version    = 0

-- rows (integer) - Number of aggregate count buckets
-- rows = 10080 -- one week

-- sec_per_row (integer) - Number of seconds each aggregate bucket represents
-- sec_per_row = 60

-- preservation_version (integer) - This must be incremented if the rows or
-- sec_per_row value is change when using preservation (the old data is cleared)
-- preservation_version = 0
```
--]]
_PRESERVATION_VERSION = read_config("preservation_version") or 0

require "circular_buffer"
require "os"
local time = os.time

local rows        = read_config("rows") or 10080
local sec_per_row = read_config("sec_per_row") or 60

local cb = circular_buffer.new(rows, 1, sec_per_row)
cb:set_header(1, "messages")

function process_message()
    cb:add(time() * 1e9, 1, 1)
    return 0
end

function timer_event(ns)
    inject_payload("cbuf", "counts", cb)
end
