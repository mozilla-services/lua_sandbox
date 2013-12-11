-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

--[[ RFC5424 severity grammar
sample input:  debug
output: 7 (string)
--]]
severity =
  l.P"debug"             /"7"
+ l.P"info"              /"6"
+ l.P"notice"            /"5"
+ (l.P"warning" + "warn")/"4"
+ (l.P"error" + "err")   /"3"
+ l.P"crit"              /"2"
+ l.P"alert"             /"1"
+ (l.P"emerg" + "panic") /"0"

return M
