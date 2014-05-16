-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "bloom_filter"

bf = bloom_filter.new(20, 0.01)
count = 0

function process(ts)
    if not bf:query(ts) then
        if not bf:add(ts) then
            error("key existed")
        end

        count = count + 1
    end

    return 0
end

function report(tc)
    if tc == 99 then
        bf:clear()
        count = 0
    else
        output(count)
        write()
    end
end

