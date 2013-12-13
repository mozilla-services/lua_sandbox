-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "lpeg"

local cbufd = require("cbufd")

function process(tc)
    if tc == 0 then
        local t = lpeg.match(cbufd.grammar, "header\n1\t2\t3\n2\tnan\t-4\n3\t-4.56\t5.67\n")
        if not t then
            return error("no match")
        end
        if t.header ~= "header" then error("header:" .. t.header) end
        if t[1].time ~= 1e9 then return error("col 1 timestamp:" .. t[1].time) end
        if t[1][1] ~= 2 then return error("col 1 val 1:" .. t[1][1]) end
        if t[1][2] ~= 3 then return error("col 1 val 2:" .. t[1][2]) end

        if t[2].time ~= 2e9 then return error("col 2 timestamp:" .. t[2].time) end
        if t[2][1] == t[2][1] then error("col 2 val 1:" .. t[2][1]) end
        if t[2][2] ~= -4 then return error("col 2 val 2:" .. t[2][2]) end

        if t[3].time ~= 3e9 then error("col 3 timestamp:" .. t[3].time) end
        if t[3][1] ~= -4.56 then error("col 3 val 1:" .. t[3][1]) end
        if t[3][2] ~= 5.67 then error("col 3 val 2:" .. t[3][2]) end
    end

   return 0
end
