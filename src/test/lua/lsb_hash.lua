-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local hash = require "lsb_hash"

local tests = {
    {hash.crc32     , nil , "bad argument #1 to '?' (must be a string)"},
    {hash.crc32     , "foobar", 2666930069},
}

function process ()
    for i,v in ipairs(tests) do
        if v[1] then
            if type(v[2]) == "string" then
                local result = v[1](v[2])
                assert(result == v[3], result)
            else
                local ok, err = pcall(v[1], v[2])
                assert(not ok and err == v[3], err)
            end
        end
    end
    return 0
end
