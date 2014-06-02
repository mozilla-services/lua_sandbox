-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.
_PRESERVATION_VERSION = 1

count = 100

function process()
    count = count + 1
    write_output(count)
    return 0
end


function report(ns)
    _PRESERVATION_VERSION = ns
end


