-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "cuckoo_filter"

cf = cuckoo_filter.new(16)

function process(ts)
    if not cf:query(ts) then
        if not cf:add(ts) then
            error("key existed")
        end
    end

    return 0
end

function report(tc)
    if tc == 98 then
        cf:delete(1);
        write_output(cf:count())
    elseif tc == 99 then
        cf:clear()
        write_output(cf:count())
    else
        write_output(cf:count())
    end
end

