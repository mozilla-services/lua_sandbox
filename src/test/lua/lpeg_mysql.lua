-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "string"
local mysql = require("mysql")

local slow_query_log_5_1 =
[[# User@Host: weaverw[weaverw] @  [10.14.214.11]
# Thread_id: 78865  Schema: weave0  Last_errno: 0  Killed: 0
# Query_time: 10.354393  Lock_time: 0.000081  Rows_sent: 0  Rows_examined: 0  Rows_affected: 0  Rows_read: 1
# Bytes_sent: 369  Tmp_tables: 0  Tmp_disk_tables: 0  Tmp_table_sizes: 0
# InnoDB_trx_id: 9C0F
use weave0;
SET timestamp=1364506803;
SELECT * from widget;
]]

local slow_query_log_5_5 =
[[
# Time: 140507 18:14:18
# User@Host: syncrw[syncrw] @  [127.0.0.1]
# Query_time: 2.964652  Lock_time: 0.000050 Rows_sent: 251  Rows_examined: 9773
SET timestamp=1399500744;
/* [queryName=FIND_ITEMS] */ SELECT *
FROM widget
WHERE id = 10;
]]

local function validate(tc, fields, t)
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
        local t = mysql.slow_query_grammar_5_1:match(slow_query_log_5_1)
        if not t then return error("no match") end
        local fields = {
            {"Rows_read", 1},
            {"Tmp_disk_tables", 0},
            {"Bytes_sent", 369, "B"},
            {"Tmp_table_sizes", 0},
            {"Query_time", 10.354393, "s"},
            {"Rows_examined", 0},
            {"Tmp_tables", 0},
            {"Lock_time", 0.000081, "s"},
            {"Rows_sent", 0},
            {"Schema", "weave0"},
            {"Rows_affected", 0}
        }
        if t.Timestamp ~= "1364506803000000000" then return error("Timestamp:" .. t.Timestamp) end
        if t.Hostname ~= "10.14.214.11" then return error("Hostname:" .. t.Hostname) end
        if t.Payload ~= "SELECT * from widget;\n" then return error("Payload:" .. t.Payload) end
        validate(tc, fields, t)
    elseif tc == 1 then
        local t = mysql.slow_query_grammar_5_5:match(slow_query_log_5_5)
        if not t then return error("no match") end
        local fields = {
            {"Query_time", 2.964652, "s"},
            {"Rows_examined", 9773},
            {"Lock_time", 0.000050, "s"},
            {"Rows_sent", 251},
        }
        if t.Timestamp ~= "1399500744000000000" then return error("Timestamp:" .. t.Timestamp) end
        if t.Hostname ~= "127.0.0.1" then return error("Hostname:" .. t.Hostname) end
        if t.Payload ~= "/* [queryName=FIND_ITEMS] */ SELECT *\nFROM widget\nWHERE id = 10;\n" then return error("Payload:" .. t.Payload) end
        validate(tc, fields, t)
    end

   return 0
end
