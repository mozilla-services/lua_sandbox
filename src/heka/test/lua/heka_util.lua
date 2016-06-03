-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local string = require "string"
local util = require "heka.util"

local t = {toplevel=0, struct = { item0 = 0, item1 = 1, item2 = {nested = "n1"}}}
local fa = {}
local fb = {}

local function table_to_fields()
    util.table_to_fields(t, fa, nil)
    assert(fa.toplevel == 0, fa.toplevel)
    assert(fa["struct.item0"] == 0, fa["struct.item0"])
    assert(fa["struct.item1"] == 1, fa["struct.item1"])
    assert(fa["struct.item2.nested"] == "n1", fa["struct.item2.nested"])
    util.table_to_fields(t, fb, nil, "_", 2)
    assert(fb.toplevel == 0, fb.toplevel)
    assert(fb["struct_item0"] == 0, fb["struct_item0"])
    assert(fb["struct_item1"] == 1, fb["struct_item1"])
    assert(fb["struct_item2"] == '{"nested":"n1"}', fb["struct_item2"])
end

function process_message ()
    table_to_fields()
    return 0
end
