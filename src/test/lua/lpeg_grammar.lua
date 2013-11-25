-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "lpeg"
require "os"
require "string"

local rfc3339 = require("rfc3339")
local rfc5424 = require("rfc5424")

function process(tc)
    if tc == 0 then
        output(lpeg.match(rfc3339.grammar, "1999-05-05T23:23:59.217-07:00"))
    elseif tc == 1 then
        output(string.format("%s %s %s %s %s %s %s %s %s %s %s",
                             lpeg.match(rfc5424.severity, "debug"),
                             lpeg.match(rfc5424.severity, "info"),
                             lpeg.match(rfc5424.severity, "notice"),
                             lpeg.match(rfc5424.severity, "warning"),
                             lpeg.match(rfc5424.severity, "warn"),
                             lpeg.match(rfc5424.severity, "error"),
                             lpeg.match(rfc5424.severity, "err"),
                             lpeg.match(rfc5424.severity, "crit"),
                             lpeg.match(rfc5424.severity, "alert"),
                             lpeg.match(rfc5424.severity, "emerg"),
                             lpeg.match(rfc5424.severity, "panic")
                          ))
    elseif tc == 2 then
        local t = lpeg.match(rfc3339.grammar, "1999-05-05T23:23:59.217-07:00")
        output(rfc3339.time_ns(t))
    elseif tc == 3 then
        output(lpeg.match(rfc3339.grammar, "1985-04-12T23:20:50.52Z"))
    elseif tc == 4 then
        output(lpeg.match(rfc3339.grammar, "1996-12-19T16:39:57-08:00"))
    elseif tc == 5 then
        output(lpeg.match(rfc3339.grammar, "1990-12-31T23:59:60Z"))
    elseif tc == 6 then
        output(lpeg.match(rfc3339.grammar, "1990-12-31T15:59:60-08:00"))
    elseif tc == 7 then
        output(lpeg.match(rfc3339.grammar, "1937-01-01T12:00:27.87+00:20"))
    end

    write()
    return 0
end
