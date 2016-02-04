-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local sid

function process_message(sequence_id)
    if not sequence_id then
        update_checkpoint()
        local ok, err = pcall(update_checkpoint, 1, 2, 3)
        assert(not ok)
        return 0
    else
        if not sid then
            update_checkpoint(sequence_id, 7)
            update_checkpoint(sequence_id)
            local ok, err = pcall(update_checkpoint, true)
            assert(not ok)
            ok, err = pcall(update_checkpoint, sequence_id, "foo")
            assert(not ok)
            sid = sequence_id
            return -5
        else
            local ok, err = pcall(update_checkpoint, sequence_id)
            assert(not ok)
            return -3 -- retry
        end
    end
    return 0
end

function timer_event(ns)
end
