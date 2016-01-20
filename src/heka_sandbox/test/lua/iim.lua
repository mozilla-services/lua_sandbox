-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"
require "bloom_filter";
local bf = bloom_filter.new(10, 0.01)

-- Table tests
local msgs = {
    {{Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"}, nil},
    {{Timestamp = 1, Uuid = "00000000-0000-0000-0000-000000000000"}, nil},
    {{Timestamp = 2, Uuid = "00000000-0000-0000-0000-000000000000", Logger = "logger", Hostname = "hostname", Type = "type", Payload = "payload", EnvVersion = "envversion", Pid = 99, Severity = 5}, 99},
    {{Timestamp = 3, Uuid = "00000000-0000-0000-0000-000000000000", Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}, "foo.log:123"},
}

local err_msgs = {
    {msg = {Timestamp = 1e9, Fields = {counts={2,"ten",4}}}, err = "inject_message() failed: array has mixed types"},
    {msg = {Timestamp = 1e9, Fields = {counts={}}}, err = "inject_message() failed: unsupported type: nil"},
    {err = "inject_message() unsupported message type: nil"},
    {msg = {Timestamp = 1e9, Fields = {counts={{1},{2}}}}, err = "inject_message() failed: unsupported array type: table"},
    {msg = {Timestamp = 1e9, Fields = {counts={value="s", value_type=2}}}, err = "inject_message() failed: invalid string value_type: 2"},
    {msg = {Timestamp = 1e9, Fields = {counts={value="s", value_type=3}}}, err = "inject_message() failed: invalid string value_type: 3"},
    {msg = {Timestamp = 1e9, Fields = {counts={value="s", value_type=4}}}, err = "inject_message() failed: invalid string value_type: 4"},
    {msg = {Timestamp = 1e9, Fields = {counts={value=1, value_type=0}}}, err = "inject_message() failed: invalid numeric value_type: 0"},
    {msg = {Timestamp = 1e9, Fields = {counts={value=1, value_type=1}}}, err = "inject_message() failed: invalid numeric value_type: 1"},
    {msg = {Timestamp = 1e9, Fields = {counts={value=1, value_type=4}}}, err = "inject_message() failed: invalid numeric value_type: 4"},
    {msg = {Timestamp = 1e9, Fields = {counts={value=true, value_type=0}}}, err = "inject_message() failed: invalid boolean value_type: 0"},
    {msg = {Timestamp = 1e9, Fields = {counts={value=true, value_type=1}}}, err = "inject_message() failed: invalid boolean value_type: 1"},
    {msg = {Timestamp = 1e9, Fields = {counts={value=true, value_type=2}}}, err = "inject_message() failed: invalid boolean value_type: 2"},
    {msg = {Timestamp = 1e9, Fields = {counts={value=true, value_type=2}}}, err = "inject_message() failed: invalid boolean value_type: 2"},
    {msg = {Timestamp = 1e9, Fields = {bf = {value=bf, representation="bf"}}}, err = "inject_message() failed: user data object does not implement lsb_output"},
}

for i, v in ipairs(msgs) do
    inject_message(v[1], v[2])
end

for i, v in ipairs(err_msgs) do
    local ok, err = pcall(inject_message, v.msg)
    if ok then error(string.format("test: %d should have failed", i)) end
    assert(v.err == err, string.format("test: %d expected: %s received: %s", i, v.err, err))
end

-- Stream Reader tests
require "heka_stream_reader"
local hsr = heka_stream_reader.new("test")
ok, err = pcall(inject_message, hsr)
local eerr = "inject_message() attempted to inject a nil message"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))

hsr:decode_message("\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\004")
inject_message(hsr)

-- String tests
inject_message("\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\005")

ok, err = pcall(inject_message, "\000")
if ok then error(string.format("string test should have failed")) end
local eerr = "inject_message() attempted to inject a invalid protobuf string"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))

ok, err = pcall(inject_message, {})
if ok then error(string.format("test should have failed")) end
eerr = "inject_message() failed: rejected by the callback"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))


