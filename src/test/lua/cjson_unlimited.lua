require "cjson"

function process()
    local ls = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
    local t = {}

    for i=1, 1000 do
        t[#t+1] = ls
    end
    write_output(cjson.encode(t))

    return 0
end
