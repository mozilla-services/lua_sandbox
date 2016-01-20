-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

function process_message(sequence_id)
    if not sequence_id then
        update_checkpoint()
        return 0
    else
        update_checkpoint(sequence_id, 7)
        return -5
    end
end

function timer_event(ns)
end

