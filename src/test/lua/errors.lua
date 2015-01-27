-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

data = ""

function process(tc)
    if tc == 0 then
        require("unknown")
    elseif tc == 1 then
        output()
    elseif tc == 2 then
        for i=1,500 do
            data = data .. "012345678901234567890123456789010123456789012345678901234567890123456789012345678901234567890123456789"
        end
    elseif tc == 3 then
        while true do
        end
    elseif tc == 4 then
        x = x + 1
    elseif tc == 5 then
        return nil
    elseif tc == 6 then
        return
    elseif tc == 7 then
        for i=1,15 do
            output("012345678901234567890123456789010123456789012345678901234567890123456789012345678901234567890123456789")
        end
    elseif tc == 8 then
        local v = require "bad_module"
    elseif tc == 9 then
        local v = require "../invalid"
    elseif tc == 10 then
        local v = require "pathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflow"
    elseif tc == 11 then
        local v = require "foo.bar"
    end
    return 0
end
