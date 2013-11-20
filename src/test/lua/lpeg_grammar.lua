-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("lpeg")

local rfc3339 = require("rfc3339")
local severity = require("rfc5424_severity")

function process(tc)
    if tc == 0 then
        output(lpeg.match(rfc3339, "1999-05-05T23:23:59.217-07:00"))
    elseif tc == 1 then
        output(lpeg.match(severity, "error"))
    end

    write()
    return 0
end
