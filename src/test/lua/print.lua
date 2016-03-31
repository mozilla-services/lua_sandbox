-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

function process(tc)
    if tc == 0 then
        print()
    elseif tc == 1 then
        print("foo\n", 10, true)
    elseif tc == 2 then
        print("f\r\0", 10, true)
    elseif tc == 3 then
        tostring = nil
        local ok, err = pcall(print, "foo")
        assert(not ok)
        assert(err == "attempt to call a nil value", err)
    end
    return 0
end
