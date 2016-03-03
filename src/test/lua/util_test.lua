-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local string = require("string")
local util = require("util")

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

local function alpha()
    return {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k",
            "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v",
            "w", "x", "y", "z"}
end

local function behead_array()
    local a = alpha()
    util.behead_array(10, a)
    assert(a[1] == "j", string.format("a[1] should be 'j', is '%s'", tostring(a[1])))
    assert(#a == 17, string.format("#a should be 17, is %d", #a))

    a = alpha()
    util.behead_array(16, a)
    assert(a[1] == "p", string.format("a[1] should be 'p', is '%s'", tostring(a[1])))
    assert(#a == 11, string.format("#a should be 11, is %d", #a))

    a = alpha()
    util.behead_array(0, a)
    assert(a[1] == "a", string.format("a[1] should be 'a', is '%s'", tostring(a[1])))
    assert(#a == 26, string.format("#a should be 26, is %d", #a))

    a = alpha()
    util.behead_array(45, a)
    assert(a[1] == nil, string.format("a[1] should be 'nil', is '%s'", tostring(a[1])))
    assert(#a == 0, string.format("#a should be 0, is %d", #a))
end

function process ()
    table_to_fields()
    behead_array()
    return 0
end
