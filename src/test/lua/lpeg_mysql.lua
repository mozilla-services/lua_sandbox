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

local mariadb_slow_query_log = {
[[
# User@Host: syncrw[syncrw] @  [127.0.0.1]
# Thread_id: 110804  Schema: weave0  QC_hit: No
# Query_time: 1.178108  Lock_time: 0.000053  Rows_sent: 198  Rows_examined: 198
SET timestamp=1399500744;
/* [queryName=FIND_ITEMS] */ SELECT *
FROM widget
WHERE id = 10;
]],
[[
# Thread_id: 110804  Schema: weave0  QC_hit: No
# Query_time: 1.178108  Lock_time: 0.000053  Rows_sent: 198  Rows_examined: 198
SET timestamp=1399500744;
/* [queryName=FIND_ITEMS] */ SELECT *
FROM widget
WHERE id = 10;
]],
[[
# Time: 140623 16:43:22
# User@Host: syncrw[syncrw] @  [127.0.0.1]
# Thread_id: 110804  Schema: weave0  QC_hit: No
# Query_time: 1.178108  Lock_time: 0.000053  Rows_sent: 198  Rows_examined: 198
# Full_scan: Yes  Full_join: Yes  Tmp_table: Yes  Tmp_table_on_disk: No
# Filesort: Yes  Filesort_on_disk: No  Merge_passes: 3
SET timestamp=1399500744;
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

local mariadb_fields = {
    {"Query_time", 1.178108, "s"},
    {"Rows_examined", 198},
    {"Lock_time", 0.000053, "s"},
    {"Rows_sent", 198},
    {"QC_hit", "No"},
    {"Thread_id", 110804},
    {"Schema", "weave0"}
}

local mariadb_verbose_fields = {
    {"Query_time", 1.178108, "s"},
    {"Rows_examined", 198},
    {"Lock_time", 0.000053, "s"},
    {"Rows_sent", 198},
    {"QC_hit", "No"},
    {"Thread_id", 110804},
    {"Schema", "weave0"},
    {"Full_scan", "Yes"},
    {"Full_join", "Yes"},
    {"Filesort", "Yes"},
    {"Filesort_on_disk", "No"},
    {"Tmp_table_on_disk", "No"},
    {"Tmp_table", "Yes"},
    {"Merge_passes", 3}
}

local function validate(fields, t)
    if t.Timestamp ~= "1399500744000000000" then return error("Timestamp:" .. t.Timestamp) end
    if t.Payload ~= "/* [queryName=FIND_ITEMS] */ SELECT *\nFROM widget\nWHERE id = 10;\n" then return error("Payload:" .. t.Payload) end
    for i, v in ipairs(fields) do
        if #v == 3 then
            if t.Fields[v[1]].value ~= v[2] or t.Fields[v[1]].representation ~= v[3] then
                if type(v[2]) == "string" then
                    return error(string.format("field:%s: %s (%s)", v[1], t.Fields[v[1]].value, t.Fields[v[1]].representation), 2)
                else
                    return error(string.format("field:%s: %G (%s)", v[1], t.Fields[v[1]].value, t.Fields[v[1]].representation), 2)
                end
            end
        else
            if t.Fields[v[1]] ~= v[2] then return error(string.format("field:%s:", v[1]) .. t.Fields[v[1]], 2) end
        end
    end
end

local function header()
    local t = mysql.slow_query_grammar:match(slow_query_log[1])
    if not t then return error("no match") end
    if t.Hostname ~= "127.0.0.1" then return error("Hostname:" .. t.Hostname) end
    validate(fields, t)
end

local function standard()
    local t = mysql.slow_query_grammar:match(slow_query_log[2])
    if not t then return error("no match") end
    if t.Hostname ~= "127.0.0.1" then return error("Hostname:" .. t.Hostname) end
    validate(fields, t)
end

local function short()
    local t = mysql.short_slow_query_grammar:match(slow_query_log[3])
    if not t then return error("no match") end
    if t.Hostname then return error("Hostname:" .. t.Hostname) end
    validate(fields, t)
end

local function mariadb_standard()
    local t = mysql.mariadb_slow_query_grammar:match(mariadb_slow_query_log[1])
    if not t then return error("no match") end
    if t.Hostname ~= "127.0.0.1" then return error("Hostname:" .. t.Hostname) end
    validate(mariadb_fields, t)
end

local function mariadb_short()
    local t = mysql.mariadb_short_slow_query_grammar:match(mariadb_slow_query_log[2])
    if not t then return error("no match") end
    if t.Hostname then return error("Hostname:" .. t.Hostname) end
    validate(mariadb_fields, t)
end

local function mariadb_verbose()
    local t = mysql.mariadb_slow_query_grammar:match(mariadb_slow_query_log[3])
    if not t then return error("no match") end
    if t.Hostname ~= "127.0.0.1" then return error("Hostname:" .. t.Hostname) end
    validate(mariadb_verbose_fields, t)
end


function process(tc)
    header()
    standard()
    short()
    mariadb_standard()
    mariadb_short()
    mariadb_verbose()

    return 0
end
