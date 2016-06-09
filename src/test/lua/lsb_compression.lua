-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local comp = require "lsb_compression"

local tests = {
    {comp.ungzip, nil, nil, false, "bad argument #1 to '?' (must be a string)"},
    {comp.ungzip, "string", true, false, "bad argument #2 to '?' (must be a positive number or nil)"},
    {comp.ungzip, "string", -100, false, "bad argument #2 to '?' (must be a positive number or nil)"},
    {comp.ungzip, "\031\139\008\000\000\000\000\000\000\003\075\203\207\079\074\044\226\002\000\071\151\044\178\007\000\000\000", nil, true, "foobar\n"},
    {comp.ungzip, "\031\139\008\000\000\000\000\000\000\003\075\203\207\079\074\044\226\002\000\071\151\044\178\007\000\000\000", 4, true, nil},
    {comp.ungzip, "not gzip", nil, true, nil},
}

function process ()
    for i,v in ipairs(tests) do
        if v[1] then
            if v[4] then
                local result = v[1](v[2], v[3])
                assert(result == v[5], result)
            else
                local ok, err = pcall(v[1], v[2], v[3])
                assert(not ok and err == v[5], err)
            end
        end
    end
    return 0
end
