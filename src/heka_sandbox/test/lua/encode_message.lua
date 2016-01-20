-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"
require "table"

local msgs = {
    {
        msg = {Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"},
        rv = "\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\000\034\002sl\074\002\sh"
    },
    {
        msg = {Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000", Logger = "l", Hostname = "h"},
        rv = "\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\000\034\001l\074\001\h"
    },
    {
        msg = {Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"},
        rv = "\030\002\008\028\031\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\000\034\002sl\074\002\sh",
        framed = true,
    },
}

for i, v in ipairs(msgs) do
    local rv
    if v.framed then
        rv = encode_message(v.msg, v.framed)
    else
        rv = encode_message(v.msg)
    end

    if v.rv ~= rv then
        local et = {string.byte(v.rv, 1, -1)}
        local rt = {string.byte(rv, 1, -1)}
        assert(v.rv == rv, string.format("test: %d\nexpected: %s\nreceived: %s", i, table.concat(et, " "), table.concat(rt, " ")))
    end

end

local ok, err = pcall(encode_message, "foo")
if ok then error("encode_message should not accept a string") end
local eerr = "bad argument #1 to '?' (table expected, got string)"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))
