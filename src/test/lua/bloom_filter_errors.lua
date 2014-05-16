-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "bloom_filter"

function process(tc)
    if tc == 0 then
        local bf = bloom_filter.new(2) -- new() incorrect # args
    elseif tc == 1 then
        local bf = bloom_filter.new(nil, 0.01) -- new() non numeric item
    elseif tc == 2 then
        local bf = bloom_filter.new(0, 0.01) -- invalid items
    elseif tc == 3 then
        local bf = bloom_filter.new(2, nil) -- nil probability
    elseif tc == 4 then
        local bf = bloom_filter.new(2, 0) -- invalid probability
    elseif tc == 5  then
        local bf = bloom_filter.new(2, 1) -- invalid probability
    elseif tc == 6 then
        local bf = bloom_filter.new(20, 0.01)
        bf:add() --incorrect # args
    elseif tc == 7 then
        local bf = bloom_filter.new(20, 0.01)
        bf:add({}) --incorrect argument type
    elseif tc == 8 then
        local bf = bloom_filter.new(20, 0.01)
        bf:query() --incorrect # args
    elseif tc == 9 then
        local bf = bloom_filter.new(20, 0.01)
        bf:query({}) --incorrect argument type
    elseif tc == 10 then
        local bf = bloom_filter.new(20, 0.01)
        bf:clear(1) --incorrect # args
    elseif tc == 11 then
        local bf = bloom_filter.new(20, 0.01)
        bf:fromstring({}) --incorrect argument type
    elseif tc == 12 then
        local bf = bloom_filter.new(20, 0.01)
        bf:fromstring("                       ") --incorrect argument length
    end
return 0
end
