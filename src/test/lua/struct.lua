require "struct"

local fmt = "<i4"
local s = "\007\000\000\000"

function process()
    assert(s == struct.pack(fmt, 7))
    assert(7 == struct.unpack(fmt, s))
    assert(struct.size(fmt) == 4)
    return 0
end
