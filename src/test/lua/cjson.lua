require "string"
require "table"
local cj = require "cjson"

function process()
    assert(cj == cjson, "cjson not creating a global table")

    local value = cjson.decode("[ true, { \"foo\": \"bar\" } ]")
    assert("bar" == value[2].foo, string.format("bar: %s", tostring(value[2].foo)))

    local ls = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
    local t = {}

    for i=1, 650 do
        t[#t+1] = ls
    end
    local success, str = pcall(cj.encode, t)
    assert(not success, "should have exceeded the max_output size")

    local null_json = '{"test" : 1, "null" : null}'
    local value = cjson.decode(null_json)
    assert(value.null == nil, "null not discarded")

    cjson.decode_null(true)
    value = cjson.decode(null_json)
    assert(type(value.null) == "userdata", "null discarded")
    return 0
end
