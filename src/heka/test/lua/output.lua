-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local sid

function process_message(sequence_id)
    if not sequence_id then
        update_checkpoint()
        return 0
    else
        if not sid then
            update_checkpoint(sequence_id, 7)
            sid = sequence_id
            return -5
        else
            return -3 -- retry
        end
    end
    return 0
end

function timer_event(ns)
end
