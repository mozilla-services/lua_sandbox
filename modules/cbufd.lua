-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"
l.locale(l)
local tonumber = tonumber

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

--[[ cbufd grammar
sample input:
{"time":1379574900,"rows":1440,"columns":2,"seconds_per_row":60,"column_info":[{"name":"Requests","unit":"count","aggregation":"sum"},{"name":"Total_Size","unit":"KiB","aggregation":"sum"}]}
1379660520	12075	159901
1379660280	11837	154880

output table:
1
    1=12075 (number)
    2=159901 (number)
    time=1379660520000000000 (number)

2
    1=11837 (number)
    2=154880 (number)
    time=1379660280000000000 (number)

header={"time":1379574900,"rows":1440,"columns":2,"seconds_per_row":60,"column_info":[{"name":"Requests","unit":"count","aggregation":"sum"},{"name":"Total_Size","unit":"KiB","aggregation":"sum"}]}
--]]

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
