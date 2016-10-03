-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"

local tests = {
    {read_message("Uuid", 0, 0, true), "abcdefghijklmnop"},
    {read_message("Type", nil, nil, true), "type"},
    {read_message("Logger", 0, 0, true), "logger"},
    {read_message("Payload", 0, 0, true), "payload"},
    {read_message("EnvVersion", 0, 0, true), "env_version"},
    {read_message("Hostname", 0, 0, true), "hostname"},
    {read_message("Fields[string]"  , 0 , 0 , true), "string"},
    {read_message("Fields[notfound]", 0 , 0 , true), nil},
    {read_message("Fields[strings]" , 0 , 0 , true), "s1"},
    {read_message("Fields[strings]" , 0 , 2 , true), "s3"},
    {read_message("Fields[strings]" , 10, 0 , true), nil},
    {read_message("Fields[strings]" , 0 , 10, true), nil},
    {read_message("Fields[notfound]", 0 , 0 , true), nil},
}

local tests_size = {
    {read_message("raw", 0 , 0 , true), 208},
    {read_message("framed", 0 , 0 , true), 214},
}

local creation_errors = {
    {"Timestamp", 0, 0, true},
    {"Severity", 0, 0, true},
    {"Pid", 0, 0, true},
    {"size", 0, 0, true},
    {"Fields[string", 0, 0, true},
    {"morethan8", 0, 0, true},
    {"lt8", 0, 0, true},
    {"Fields[string]", -1, 0, true},
    {"Fields[string]", 0, -1, true},
    {"Fields[string]", "a", 0, true},
    {"Fields[string]", 0, "a", true},
    {"Fields[string]", 0, 0, true, 0},
}

local fetch_errors = {
    read_message("Fields[bool]", 0, 0, true),
    read_message("Fields[number]", 0, 0, true),
}

function process_message()
    for i, v in ipairs(tests) do
        local r = read_message_zc(v[1])
        assert(v[2] == r, string.format("test: %d expected: %s received: %s", i, tostring(v[2]), tostring(r)))
    end

    for i, v in ipairs(tests) do
        local r = tostring(v[1])
        assert(v[2] == r, string.format("test: %d expected: %s received: %s", i, tostring(v[2]), tostring(r)))
    end

    add_to_payload(read_message("Uuid", 0, 0, true))
    add_to_payload(read_message("framed", 0, 0, true))
    inject_payload()

    for i, v in ipairs(tests_size) do
        local r = read_message_zc(v[1])
        assert(v[2] == #r, string.format("test: %d expected: %d received: %d", i, tostring(v[2]), tostring(#r)))
    end

    for i, v in ipairs(tests_size) do
        local r = tostring(v[1])
        assert(v[2] == #r, string.format("test: %d expected: %d received: %d", i, tostring(v[2]), tostring(#r)))
    end

    for i, v in ipairs(creation_errors) do
        local ok, r = pcall(read_message, unpack(v))
        assert(not ok, string.format("test: %d creation should have errored", i))
    end

    for i, v in ipairs(fetch_errors) do
        local ok, r = pcall(read_message_zc, v)
        assert(not ok, string.format("test: %d fetch should have errored", i))
    end
    return 0
end
