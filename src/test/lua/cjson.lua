require "string"
require "table"
local cj = require "cjson"

function process()
    if cj ~= cjson then
        return 1
    end

    local value = cjson.decode("[ true, { \"foo\": \"bar\" } ]")
    if "bar" ~= value[2].foo then
        return 2
    end

    local ls = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
    local t = {}

    for i=1, 650 do
        t[#t+1] = ls
    end
    local success, str = pcall(cj.encode, t) -- exceed the max_output size
    if success then
        return 3
    end

    return 0
end
