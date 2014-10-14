require "string"
require "table"
local cj = require "cjson"
local js = '["this is a test","this is a test","this is a test","this is a test","this is a test"]'

function process()
    assert(cj == cjson, "cjson not creating a global table")

    local value = cjson.decode("[ true, { \"foo\": \"bar\" } ]")
    assert("bar" == value[2].foo, string.format("bar: %s", tostring(value[2].foo)))

    local null_json = '{"test" : 1, "null" : null}'
    local value = cjson.decode(null_json)
    assert(value.null == nil, "null not discarded")

    cjson.decode_null(true)
    value = cjson.decode(null_json)
    assert(type(value.null) == "userdata", "null discarded")

    local t = cjson.decode(js)
    assert(#t == 5, "could not decode a JSON string bigger than the output buffer")

    local ok, json = pcall(cjson.encode, t)
    assert(not ok, "could encode an array bigger than the the output buffer")

    assert(not cjson.new)

    assert(not cjson.encode_keep_buffer)

    return 0
end
