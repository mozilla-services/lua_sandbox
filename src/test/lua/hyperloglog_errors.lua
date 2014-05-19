-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "hyperloglog"

function process(tc)
    if tc == 0 then
        local hll = hyperloglog.new(2) -- new() incorrect # args
    elseif tc == 1 then
        local hll = hyperloglog.new()
        hll:add({}) --incorrect argument type
    elseif tc == 2 then
        local hll = hyperloglog.new()
        hll:add() --incorrect # args
    elseif tc == 3 then
        local hll = hyperloglog.new()
        hll:count(1) --incorrect # args
    elseif tc == 4 then
        local hll = hyperloglog.new()
        hll:clear(1) --incorrect # args
    elseif tc == 5 then
        local hll = hyperloglog.new()
        hll:fromstring({}) --incorrect argument type
    elseif tc == 6 then
        local hll = hyperloglog.new()
        hll:fromstring("                       ") --incorrect argument length
    end
return 0
end
