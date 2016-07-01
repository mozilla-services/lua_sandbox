-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local sid

assert(read_config)
assert(read_message)
assert(decode_message)
assert(encode_message)
assert(update_checkpoint)
assert(create_message_matcher)
assert(not inject_message)
assert(not add_to_payload)
assert(not inject_payload)

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
            local ok, mm = pcall(create_message_matcher, "Type == 'type'")
            assert(ok, mm)
            assert(mm:eval())

            ok, mm = pcall(create_message_matcher, "Type == 'foo'")
            assert(ok, mm)
            assert(not mm:eval())

            ok, err = pcall(create_message_matcher, "Widget == 'foo'")
            assert(not ok)

            ok, err = pcall(update_checkpoint, sequence_id)
            assert(not ok)

            return -3 -- retry
        end
    end
    return 0
end

function timer_event(ns, shutdown)
    local ok, mm = pcall(create_message_matcher, "Type == 'foo'");
    assert(ok, mm)
    ok, err = pcall(mm.eval, mm)
    assert(err == "no active message")
end
