-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"

-- Table tests
local msgs = {
    {Timestamp = 1, Uuid = "\001\002\003\000\000\000\000\000\000\000\000\000\000\000\000\000"},
    {Pid = 2, Timestamp = 3, Uuid = "\004\005\006\000\000\000\000\000\000\000\000\000\000\000\000\000", Logger = "ignore", Hostname = "spoof"},
}

local err_msgs = {
    {err = "bad argument #1 to '?' (table expected, got nil)"},
}

for i, v in ipairs(msgs) do
    inject_message(v)
end

for i, v in ipairs(err_msgs) do
    local ok, err = pcall(inject_message, v.msg)
    if ok then error(string.format("test: %d should have failed", i)) end
    assert(v.err == err, string.format("test: %d expected: %s received: %s", i, v.err, err))
end
