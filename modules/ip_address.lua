-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"
l.locale(l)

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local d8    = "1" * l.digit * l.digit
              + "2" * l.R"04" * l.digit
              + "25" * l.R"05"
              + l.R"19" * l.digit
              + l.digit
v4          = d8 * "." * d8 * "." * d8 * "." * d8


local h16   = l.xdigit * l.xdigit^-3
local hg    = h16 * ":"
local ls32  = hg * h16
            + v4

v6          =             hg * hg * hg * hg * hg * hg * ls32
+                       "::" * hg * hg * hg * hg * hg * ls32
+ h16^-1             *  "::" * hg * hg * hg * hg * ls32
+ (hg * hg^-1 + ":") *  ":"  * hg * hg * hg * ls32
+ (hg * hg^-2 + ":") *  ":"  * hg * hg * ls32
+ (hg * hg^-3 + ":") *  ":"  * hg * ls32
+ (hg * hg^-4 + ":") *  ":"  * ls32
+ (hg * hg^-5 + ":") *  ":"  * h16
+ (hg * hg^-6 + ":") *  ":"

return M
