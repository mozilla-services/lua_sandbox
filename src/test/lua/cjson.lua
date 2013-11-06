local cj = require("cjson")

function process()
    if cj ~= cjson then
        return 1
    end

    local value = cjson.decode("[ true, { \"foo\": \"bar\" } ]")
    if "bar" ~= value[2].foo then
        return 2
    end

    return 0
end
