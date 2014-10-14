-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local util = require("util")

local t = {toplevel=0, struct = { item0 = 0, item1 = 1, item2 = {nested = "n1"}}}
local f = {}

function process ()
    util.table_to_fields(t, f, nil)
    assert(f.toplevel == 0, f.toplevel)
    assert(f["struct.item0"] == 0, f["struct.item0"])
    assert(f["struct.item1"] == 1, f["struct.item1"])
    assert(f["struct.item2.nested"] == "n1", f["struct.item2.nested"])
    return 0
end
