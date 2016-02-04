-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"
require "table"
require "circular_buffer"

local cb = circular_buffer.new(2,1,1)

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
    {
        msg = {Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000", Logger = "l", Hostname = "h",
            Fields = {
                number  = 1,
                numbers = {value = {1,2,3}, representation = "count"},
                string  = "string",
                strings = {"s1","s2","s3"},
                bool    = true,
                bools   = {true,false,false}}},
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\1\108\74\1\104\82\19\10\6\110\117\109\98\101\114\16\3\57\0\0\0\0\0\0\240\63\82\44\10\7\110\117\109\98\101\114\115\16\3\26\5\99\111\117\110\116\58\24\0\0\0\0\0\0\240\63\0\0\0\0\0\0\0\64\0\0\0\0\0\0\8\64\82\14\10\5\98\111\111\108\115\16\4\66\3\1\0\0\82\10\10\4\98\111\111\108\16\4\64\1\82\16\10\6\115\116\114\105\110\103\34\6\115\116\114\105\110\103\82\21\10\7\115\116\114\105\110\103\115\34\2\115\49\34\2\115\50\34\2\115\51"
    },
    {
        msg = {Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000", Logger = "l", Hostname = "h",
            Fields = {
                {name = "number"    ,value = 1},
                {name = "numbers"   ,value = {1,2,3}, representation="count"},
                {name = "string"    ,value = "string"},
                {name = "strings"   ,value = {"s1","s2","s3"}},
                {name = "bool"      ,value = true},
                {name = "bools"     ,value = {true,false,false}}}
            },
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\1\108\74\1\104\82\19\10\6\110\117\109\98\101\114\16\3\57\0\0\0\0\0\0\240\63\82\44\10\7\110\117\109\98\101\114\115\16\3\26\5\99\111\117\110\116\58\24\0\0\0\0\0\0\240\63\0\0\0\0\0\0\0\64\0\0\0\0\0\0\8\64\82\16\10\6\115\116\114\105\110\103\34\6\115\116\114\105\110\103\82\21\10\7\115\116\114\105\110\103\115\34\2\115\49\34\2\115\50\34\2\115\51\82\10\10\4\98\111\111\108\16\4\64\1\82\14\10\5\98\111\111\108\115\16\4\66\3\1\0\0"
    },
    {
        msg = {Timestamp = 0, Uuid = string.rep("\0", 16), Fields = {key = {value = "value", value_type = 0, representation = "widget"}}}, -- string
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\2\115\108\74\2\115\104\82\20\10\3\107\101\121\26\6\119\105\100\103\101\116\34\5\118\97\108\117\101",
    },
    {
        msg = {Timestamp = 0, Uuid = string.rep("\0", 16), Fields = {key = {value = "value", value_type = 1, representation = "widget"}}}, -- bytes
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\2\115\108\74\2\115\104\82\22\10\3\107\101\121\16\1\26\6\119\105\100\103\101\116\42\5\118\97\108\117\101",
    },
    {
        msg = {Timestamp = 0, Uuid = string.rep("\0", 16), Fields = {key = {value = 34, value_type = 2, representation = "widget"}}}, -- int
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\2\115\108\74\2\115\104\82\17\10\3\107\101\121\16\2\26\6\119\105\100\103\101\116\48\34",
    },
    {
        msg = {Timestamp = 0, Uuid = string.rep("\0", 16), Fields = {key = {value = 34, value_type = 3, representation = "widget"}}}, -- double
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\2\115\108\74\2\115\104\82\24\10\3\107\101\121\16\3\26\6\119\105\100\103\101\116\57\0\0\0\0\0\0\65\64",
    },
    {
        msg = {Timestamp = 0, Uuid = string.rep("\0", 16), Fields = {key = {value = true, value_type = 4, representation = "widget"}}}, -- bool
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\2\115\108\74\2\115\104\82\17\10\3\107\101\121\16\4\26\6\119\105\100\103\101\116\64\1",
    },
    {
        msg = {Timestamp = 0, Uuid = string.rep("\0", 16), Fields = {key = {value = {1,2,3}, value_type = 2, representation = "widget"}}}, -- int
        rv = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\2\115\108\74\2\115\104\82\20\10\3\107\101\121\16\2\26\6\119\105\100\103\101\116\50\3\1\2\3",
    },
    {
        msg = {Timestamp = 0, Uuid = string.rep("\0", 16), Fields = {key = cb}}, -- userdata
        rv = '\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\34\2\115\108\74\2\115\104\82\141\1\10\3\107\101\121\16\1\42\131\1\123\34\116\105\109\101\34\58\48\44\34\114\111\119\115\34\58\50\44\34\99\111\108\117\109\110\115\34\58\49\44\34\115\101\99\111\110\100\115\95\112\101\114\95\114\111\119\34\58\49\44\34\99\111\108\117\109\110\95\105\110\102\111\34\58\91\123\34\110\97\109\101\34\58\34\67\111\108\117\109\110\95\49\34\44\34\117\110\105\116\34\58\34\99\111\117\110\116\34\44\34\97\103\103\114\101\103\97\116\105\111\110\34\58\34\115\117\109\34\125\93\125\10\110\97\110\10\110\97\110\10'
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
        --local et = {string.byte(v.rv, 1, -1)}
        local rt = {string.byte(rv, 1, -1)}
        assert(v.rv == rv, string.format("test: %d\received: %s", i, table.concat(rt, " ")))
    end

end

local ok, err = pcall(encode_message, "foo")
if ok then error("encode_message should not accept a string") end
local eerr = "bad argument #1 to '?' (table expected, got string)"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))

ok, err = pcall(encode_message, "", true, "extra")
assert(not ok)
assert("bad argument #3 to '?' (incorrect number of arguments)" == err, string.format("received: %s", err))

ok, err = pcall(encode_message, {Fields = {foo = {value = {"s", true}}}})
assert(not ok)
assert("encode_message() failed: array has mixed types" == err, string.format("received: %s", err))

ok, err = pcall(encode_message, {Fields = { {noname = "foo", value = 1}} })
assert(not ok)
assert("encode_message() failed: field name must be a string" == err, string.format("received: %s", err))
