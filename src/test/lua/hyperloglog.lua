-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "hyperloglog"

hll = hyperloglog.new()

function process(ts)
    hll:add(ts)
    return 0
end

function report(tc)
    if tc == 99 then
        hll:clear()
    else
        write_output(hll:count())
    end
end

