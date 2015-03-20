-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "cuckoo_filter"

cf = cuckoo_filter.new(2e6)

function process(ts)
    cf:add(ts)
    return 0
end

function report(tc)
    write_output(cf:count())
end

