-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"

local tests = {
    {"Timestamp", 1e9},
    {"Uuid", "abcdefghijklmnop"},
    {"Type", "type"},
    {"Logger", "logger"},
    {"Payload", "payload"},
    {"EnvVersion", "env_version"},
    {"Hostname", "hostname"},
    {"Severity", 9},
    {"Pid", nil},
    {"raw", 208},
    {"size", 208},
    {"framed", 214},
    {"Fields[string]", "string"},
    {"Fields[notfound]", nil},
}

local fields = {
    {{"Fields[number]"   , 0 , 0 }, 1},
    {{"Fields[number]"   , 0 , 0, false }, 1},
    {{"Fields[numbers]"  , 0 , 2 }, 3},
    {{"Fields[bool]"     , 0 , 0 }, true},
    {{"Fields[bools]"    , 0 , 1 }, false},
    {{"Fields[strings]"  , 0 , 0 }, "s1"},
    {{"Fields[strings]"  , 0 , 2 }, "s3"},
    {{"Fields[strings]"  , 10, 0 }, nil},
    {{"Fields[strings]"  , 0 , 10}, nil},
    {{"Fields[notfound]" , 0 , 0 }, nil},
}

local errors = {
    {"Fields[string"},
    {"morethan8"},
    {"lt8"},
    {"Fields[string]", -1, 0},
    {"Fields[string]", 0, -1},
    {"Fields[string]", "a", 0},
    {"Fields[string]", 0, "a"},
    {"Fields[string]", 0, 0, "str"},
}


function process_message()
    for i, v in ipairs(tests) do
        local r = read_message(v[1])
        if v[1] == "raw" or v[1] == "framed" then
            assert(v[2] == string.len(r), string.format("test: %d expected: %d received: %d", i, string.len(v[2]), string.len(r)))
        elseif v[1] == "size" then
            assert(v[2] == r, string.format("test: %d expected: %d received: %d", i, v[2], r))
        else
            assert(v[2] == r, string.format("test: %d expected: %s received: %s", i, tostring(v[2]), tostring(r)))
        end
    end

    for i, v in ipairs(fields) do
        local r = read_message(unpack(v[1]))
        assert(v[2] == r, string.format("test: %d expected: %s received: %s", i, tostring(v[2]), tostring(r)))
    end

    for i, v in ipairs(errors) do
        local ok, r = pcall(read_message, unpack(v))
        assert(not ok, string.format("test: %d should have errored", i))
    end

    return 0
end
