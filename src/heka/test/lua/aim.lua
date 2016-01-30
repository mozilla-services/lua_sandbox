-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"

-- Table tests
local msgs = {
    {Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"},
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

add_to_payload("foo bar")
inject_payload()

add_to_payload("foo")
inject_payload("dat", "test", " bar")

local ok, err = pcall(add_to_payload, add_to_payload)
if ok then error("cannot output functions") end
local eerr = "bad argument #1 to '?' (unsupported type)"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))

ok, err = pcall(inject_payload, "txt", "name", add_to_payload)
if ok then error("cannot output functions") end
eerr = "bad argument #3 to '?' (unsupported type)"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))
