-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("lpeg")
local l = lpeg
l.locale(l)

local space = l.space^1

local timestamp = l.Cg((l.R"09"^1 / "%0000000000"), "Timestamp")

local severity = l.Cg(
    l.Cs(l.P"debug"               /"7")
    + l.Cs(l.P"info"              /"6")
    + l.Cs(l.P"notice"            /"5")
    + l.Cs((l.P"warning" + "warn")/"4")
    + l.Cs((l.P"error" + "err")   /"3")
    + l.Cs(l.P"crit"              /"2")
    + l.Cs(l.P"alert"             /"1")
    + l.Cs((l.P"emerg" + "panic") /"0")
    , "Severity")

local key = l.C(l.alpha^1)

local value = l.C(l.R"!~"^1)

local pair = space * l.Cg(key * "=" * value)

local fields = l.Cg(l.Cf(l.Ct("") * pair^0, rawset), "Fields")

local grammar = l.Ct(timestamp * space * severity * fields)

function process ()
    local t = grammar:match("1376389920 debug id=2321 url=example.com item=1")
    if t then
        write_message(t)
    else
        return -1
    end
    return 0
end
