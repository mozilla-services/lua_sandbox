local ok, err = pcall(require, "io")
assert(not ok, "the io module is disabled")
assert(err == "module 'io' disabled", err)

require "math"
require "os"
require "string"
require "table"

local function all_there(test)
    for k, v in pairs(test.values) do
        assert(test.t[k], string.format("missing: %s.%s", test.name, k))
    end
end

local function nothing_extra(test)
    for k, v in pairs(test.t) do
        assert(test.values[k], string.format("extra: %s.%s", test.name, k))
    end
end

local tests = {
    {name = "base", t = _G, values = {
        _G=1,
        _VERSION=1,
        assert=1,
        error=1,
        gcinfo=1,
        getfenv=1,
        getmetatable=1,
        ipairs=1,
        math=1,
        module=1,
        next=1,
        os=1,
        output=1,
        package=1,
        pairs=1,
        pcall=1,
        rawequal=1,
        rawget=1,
        rawset=1,
        require=1,
        select=1,
        setfenv=1,
        setmetatable=1,
        string=1,
        table=1,
        tonumber=1,
        tostring=1,
        type=1,
        unpack=1,
        xpcall=1,
        }
    },

    {name = "math", t = math, values = {
        abs=1,
        acos=1,
        asin=1,
        atan2=1,
        atan=1,
        ceil=1,
        cos=1,
        cosh=1,
        deg=1,
        exp=1,
        floor=1,
        fmod=1,
        frexp=1,
        huge=1,
        ldexp=1,
        log10=1,
        log=1,
        max=1,
        min=1,
        mod=1, -- compat
        modf=1,
        pi=1,
        pow=1,
        rad=1,
        random=1,
        randomseed=1,
        sin=1,
        sinh=1,
        sqrt=1,
        tan=1,
        tanh=1,
        }
    },

    {name = "os", t = os, values = {
        clock=1,
        date=1,
        difftime=1,
        time=1,
        }
    },

    {name = "package", t = package, values = {}},

    {name = "string", t = string, values = {
        byte=1,
        char=1,
        dump=1,
        find=1,
        format=1,
        gfind=1, -- compat
        gmatch=1,
        gsub=1,
        len=1,
        lower=1,
        match=1,
        rep=1,
        reverse=1,
        sub=1,
        upper=1,
        }
    },

    {name = "table", t = table, values = {
        concat=1,
        foreach=1,
        foreachi=1,
        getn=1,
        insert=1,
        maxn=1,
        remove=1,
        setn=1,
        sort=1,
        }
    },
}

for i, v in ipairs(tests) do
    all_there(v)
    nothing_extra(v)
end
