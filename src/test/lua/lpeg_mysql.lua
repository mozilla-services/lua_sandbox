-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"
local mysql = require("mysql")

local slow_query_log = {
[[
# Time: 140507 18:14:18
# User@Host: syncrw[syncrw] @  [127.0.0.1]
# Query_time: 2.964652  Lock_time: 0.000050 Rows_sent: 251  Rows_examined: 9773
SET timestamp=1399500744;
/* [queryName=FIND_ITEMS] */ SELECT *
FROM widget
WHERE id = 10;
]],
[[
# User@Host: syncrw[syncrw] @  [127.0.0.1]
# Query_time: 2.964652  Lock_time: 0.000050 Rows_sent: 251  Rows_examined: 9773
use widget;
SET last_insert_id=999,insert_id=1000,timestamp=1399500744;
# administrator command: do something
/* [queryName=FIND_ITEMS] */ SELECT *
FROM widget
WHERE id = 10;
]],
[[
# Query_time: 2.964652  Lock_time: 0.000050 Rows_sent: 251  Rows_examined: 9773
SET last_insert_id=999,timestamp=1399500744;
/* [queryName=FIND_ITEMS] */ SELECT *
FROM widget
WHERE id = 10;
]]
}

local fields = {
    {"Query_time", 2.964652, "s"},
    {"Rows_examined", 9773},
    {"Lock_time", 0.000050, "s"},
    {"Rows_sent", 251},
}

local function validate(tc, fields, t)
    if t.Timestamp ~= "1399500744000000000" then return error("Timestamp:" .. t.Timestamp) end
    if t.Payload ~= "/* [queryName=FIND_ITEMS] */ SELECT *\nFROM widget\nWHERE id = 10;\n" then return error("Payload:" .. t.Payload) end
    for i, v in ipairs(fields) do
        if #v == 3 then
            if t.Fields[v[1]].value ~= v[2] or t.Fields[v[1]].representation ~= v[3] then
                if type(v[2]) == "string" then
                    return error(string.format("test: %d %s: %s (%s)", tc, v[1], t.Fields[v[1]].value, t.Fields[v[1]].representation))
                else
                    return error(string.format("test: %d %s: %G (%s)", tc, v[1], t.Fields[v[1]].value, t.Fields[v[1]].representation))
                end
            end
        else
            if t.Fields[v[1]] ~= v[2] then return error(string.format("test: %d %s:", tc, v[1]) .. t.Fields[v[1]]) end
        end
    end
end

function process(tc)
    if tc == 0 then
        local t = mysql.slow_query_grammar:match(slow_query_log[tc+1])
        if not t then return error("no match") end
        if t.Hostname ~= "127.0.0.1" then return error("Hostname:" .. t.Hostname) end
        validate(tc, fields, t)
    elseif tc == 1 then
        local t = mysql.slow_query_grammar:match(slow_query_log[tc+1])
        if not t then return error("no match") end
        if t.Hostname ~= "127.0.0.1" then return error("Hostname:" .. t.Hostname) end
        validate(tc, fields, t)
    elseif tc == 2 then
        local t = mysql.short_slow_query_grammar:match(slow_query_log[tc+1])
        if not t then return error("no match") end
        if t.Hostname then return error("Hostname:" .. t.Hostname) end
        validate(tc, fields, t)
    end

   return 0
end
