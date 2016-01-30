-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local msg = {Timestamp = 8}

function process_message(cp)
    if cp == 0 then
        return -2, "host specific failure"
    elseif cp == 1 then
        return -1, "failed"
    elseif cp == 2 then
        return 0, "ok"
    elseif cp == "string" then
        return 0, "string"
    end
    return 0, "no cp"
end
