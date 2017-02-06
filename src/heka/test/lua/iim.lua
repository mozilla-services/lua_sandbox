-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"
require "io"

-- Table tests
local msgs = {
    {{Timestamp = 0, Uuid = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"}, nil},
    {{Timestamp = 1, Uuid = "00000000-0000-0000-0000-000000000000"}, nil},
    {{Timestamp = 2, Uuid = "00000000-0000-0000-0000-000000000000", Logger = "logger", Hostname = "hostname", Type = "type", Payload = "payload", EnvVersion = "envversion", Pid = 99, Severity = 5}, 99},
    {{Timestamp = 3, Uuid = "00000000-0000-0000-0000-000000000000", Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}, "foo.log:123"},
    {nil, "foo.log:123"},
    {nil, 123},
}

local err_msgs = {
    {msg = {Timestamp = 1e9, Fields = {counts={2,"ten",4}}}, err = "inject_message() failed: array has mixed types"},
    {msg = {Timestamp = 1e9, Fields = {counts={}}}, err = "inject_message() failed: unsupported type: nil"},
    {err = "inject_message() message cannot be nil without a checkpoint update"},
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
    {msg = {Timestamp = 1e9, Fields = {bf = {value=io.stdin, representation="bf"}}}, err = "inject_message() failed: userdata object does not implement lsb_output"},
}

for i, v in ipairs(msgs) do
    local ok, err = pcall(inject_message, v[1], v[2])
    assert(ok, string.format("test: %d err: %s", i, tostring(err)))
end

for i, v in ipairs(err_msgs) do
    local ok, err = pcall(inject_message, v.msg)
    if ok then error(string.format("test: %d should have failed", i)) end
    assert(v.err == err, string.format("test: %d expected: %s received: %s", i, v.err, err))
end

-- Stream Reader tests
require "io"
local hsr = create_stream_reader("test")
ok, err = pcall(inject_message, hsr)
local eerr = "inject_message() attempted to inject a nil message"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))
ok, err = pcall(hsr.decode_message, hsr, true)
assert(not ok)
assert("buffer must be string" == err, string.format("received: %s", err))
ok, err = pcall(hsr.decode_message, hsr, "")
assert(not ok)
assert("empty protobuf string" == err, string.format("received: %s", err))

hsr:decode_message("\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\004")
inject_message(hsr)
local ts = hsr:read_message("Timestamp")
assert(4 == ts, string.format("received: %g", ts))

local framed = "\030\002\008\020\031\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\006"
local fh = assert(io.open("hekamsg.pb", "w+"))
fh:write(framed)
fh:seek("set")
local found, consumed, read = hsr:find_message(fh)
assert(not found)
assert(0 == consumed, string.format("expected: 0 received %d", consumed))
assert(25 == read, string.format("expected: 25 received: %d", read))
found, consumed, read = hsr:find_message(fh)
assert(found)
assert(25 == consumed, string.format("expected: 25 received %d", consumed))
assert(0 == read, string.format("expected: >0 received: %d", read))
fh:close()

local found, consumed, need = hsr:find_message(framed)
assert(true == found)
assert(25 == consumed, string.format("expected: 25 received %d", consumed))
assert(0 < need, string.format("expected: >0 received: %d", need))

found, consumed, need = hsr:find_message("\0\0\0\0\0\030\002\008\020\031")
assert(false == found)
assert(5 == consumed, string.format("expected: 5 received %d", consumed))
assert(20 == need, string.format("expected: 20 received: %d", need))

local found, consumed, need = hsr:find_message("")
assert(not found)

ok, err = pcall(hsr.find_message, hsr, assert)
assert(not ok, "accepted function as second arg")

ok, err = pcall(hsr.find_message, hsr, nil, "str")
assert(not ok, "accepted string as third arg")

ok, err = pcall(hsr.find_message, hsr, hsr)
assert(not ok, "accepted non FILE userdata")

fh = assert(io.open("lua/iim.lua"))
found, consumed, read = hsr:find_message(fh)
fh:close()
assert(false == found)
assert(0 == consumed, string.format("expected: 0 received %d", consumed))
assert(0 < need, string.format("expected: >0 received: %d", need))

ok, err = pcall(hsr.read_message)
assert(not ok)
assert("read_message() incorrect number of arguments" == err, string.format("received: %s", err))

ok, err = pcall(hsr.read_message, hsr, 1, 2, 3, 4)
assert(not ok)
assert("read_message() incorrect number of arguments" == err, string.format("received: %s", err))

ok, err = pcall(hsr.read_message, 1, 2)
assert(not ok)
assert("bad argument #1 to '?' (lsb.heka_stream_reader expected, got number)" == err, string.format("received: %s", err))


-- String tests
local pbstr = "\010\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\016\005"
inject_message(pbstr)

ok, err = pcall(inject_message, pbstr, 2)
if ok then error(string.format("test should have failed")) end
eerr = "inject_message() failed: checkpoint update"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))

ok, err = pcall(inject_message, "\000")
if ok then error(string.format("string test should have failed")) end
local eerr = "inject_message() attempted to inject a invalid protobuf string"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))

ok, err = pcall(inject_message, {})
if ok then error(string.format("test should have failed")) end
eerr = "inject_message() failed: rejected by the callback rv: 1"
assert(eerr == err, string.format("expected: %s received: %s", eerr, err))
