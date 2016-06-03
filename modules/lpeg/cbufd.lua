-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Circular Buffer Delta Module

## Variables

* `grammar` - LPEG grammar to parse cbufd output

## Usage
```lua
local cbufd = require "cbufd".grammar

function process_message()
    local payload = read_message("Payload")
    local t = cbufd:match(payload)
    if not t then
        return -1, "invalid cbufd string"
    end
    -- use t
    return 0
end
```

## Sample Input/Output

### Circular Buffer Delta Input
```
{"time":1379574900,"rows":1440,"columns":2,"seconds_per_row":60,"column_info":[{"name":"Requests","unit":"count","aggregation":"sum"},{"name":"Total_Size","unit":"KiB","aggregation":"sum"}]}
1379660520      12075   159901
1379660280      11837   154880
```

### Lua Table Output
```lua
{
header='{"time":1379574900,"rows":1440,"columns":2,"seconds_per_row":60,"column_info":[{"name":"Requests","unit":"count","aggregation":"sum"},{"name":"Total_Size","unit":"KiB","aggregation":"sum"}]}',
{12075, 159901, time = 1379660520000000000},
{11837, 154880, time = 1379660280000000000}
}
```
--]]

-- Imports
local l = require "lpeg"
l.locale(l)
local tonumber = tonumber

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local function not_a_number()
    return 0/0
end

local eol = l.P"\n"
local header = l.Cg((1 - eol)^1, "header") * eol
local timestamp = l.digit^1 / "%0000000000" / tonumber
local sign = l.P"-"
local float = l.digit^1 * "." * l.digit^1
local nan = l.P"nan" / not_a_number
local number = (sign^-1 * (float + l.digit^1)) / tonumber
+ nan
local row = l.Ct(l.Cg(timestamp, "time") * ("\t" * number)^1 * eol)

grammar = l.Ct(header * row^1) * -1

return M
