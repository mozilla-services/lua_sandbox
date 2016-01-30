-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

function process_message()
    return 0
end

function timer_event(ns, shutdown)
    local cnt = 0
    for i=1, 10000 do
        cnt = cnt + 1 -- make sure we have something to measure even with a low res clock
    end

    if ns == 1e9 then
        assert(shutdown, "not shutting down")
        return
    end

    if ns == 2e9 then error("boom") end

    if shutdown then error("should not have a shutdown signal") end
end

