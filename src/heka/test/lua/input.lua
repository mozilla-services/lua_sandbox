-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

assert(read_config)
assert(decode_message)
assert(inject_message)
assert(create_stream_reader)
assert(is_running)
assert(not read_message)
assert(not encode_message)
assert(not update_checkpoint)
assert(not add_to_payload)
assert(not inject_payload)

require "string"
msg = {Timestamp = 8}

local work_cnt = 1
local function add_work()
    local cnt = 0
    for i = 1, work_cnt * 1000 do
        cnt = cnt + 1
    end
    work_cnt = work_cnt + 1
end

function process_message(cp)
    if cp == 0 then
        add_work()
        return -2, "host specific failure"
    elseif cp == 1 then
        add_work()
        return -1, "failed"
    elseif cp == 2 then
        add_work()
        return 0, "ok"
    elseif cp == 3 then
        error("boom")
    elseif cp == 4 then
        return 0, msg
    elseif cp == 5 then
        return msg
    elseif cp == 6 then
        error(string.rep("a", 255))
    elseif cp == "string" then
        add_work()
        return 0, "string"
    elseif cp == 7 then
        error(nil)
    elseif cp == 8 then
        assert(is_running(), "running")
    elseif cp == 9 then
        assert(not is_running(), "not running")
    end
    add_work()
    return 0, "no cp"
end
