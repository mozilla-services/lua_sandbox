-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"
l.locale(l)
local ip = require "ip_address"
local tonumber = tonumber

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local space     = l.space^1
local sep       = l.P("\n")
local line      = (l.P(1) - sep)^0 * sep
local float     = l.digit^1 * "." * l.digit^1

local time_5_5      = l.P"# Time: " * l.digit^1 * l.space * l.digit * l.digit * ":" * l.digit * l.digit * ":" * l.digit * l.digit * sep

local ip_address    = ip.v4 + ip.v6
local user_name     = l.alpha^1 * "[" * l.alpha^1 * "]"
local host_name     = l.alpha^0 * l.space^0 * "[" * l.Cg(ip_address^0, "Hostname") * "]"
local user          = l.P"# User@Host: " * user_name * space * "@" * space * host_name * sep

local thread_id     = l.P"# Thread_id: " * l.digit^1
local schema        = l.P"Schema: " * l.Cg(l.alnum^0, "Schema")
local last_errno    = l.P"Last_errno: " * l.digit^1
local killed        = l.P"Killed: " * l.digit^1
local thread        = thread_id * space * schema * space * l.Cg(last_errno / tonumber, "Last_errno") * space * killed * sep

local query_time    = l.P"# Query_time: " * l.Cg(l.Ct(l.Cg(float / tonumber, "value") * l.Cg(l.Cc"s", "representation")), "Query_time")
local lock_time     = l.P"Lock_time: " * l.Cg(l.Ct(l.Cg(float / tonumber, "value") * l.Cg(l.Cc"s", "representation")), "Lock_time")
local rows_sent     = l.P"Rows_sent: " * l.Cg(l.digit^1 / tonumber, "Rows_sent")
local rows_examined = l.P"Rows_examined: " * l.Cg(l.digit^1 / tonumber, "Rows_examined")
local rows_affected = l.P"Rows_affected: " * l.Cg(l.digit^1 / tonumber, "Rows_affected")
local rows_read     = l.P"Rows_read: " * l.Cg(l.digit^1 / tonumber, "Rows_read")
local query_5_1     = query_time * space * lock_time * space * rows_sent * space * rows_examined * space * rows_affected * space * rows_read * sep
local query_5_5     = query_time * space * lock_time * space * rows_sent * space * rows_examined * sep

local bytes_sent        = l.P"# Bytes_sent: " * l.Cg(l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")), "Bytes_sent")
local tmp_tables        = l.P"Tmp_tables: " * l.Cg(l.digit^1 / tonumber, "Tmp_tables")
local tmp_disk_tables   = l.P"Tmp_disk_tables: " * l.Cg(l.digit^1 / tonumber, "Tmp_disk_tables")
local tmp_table_sizes   = l.P"Tmp_table_sizes: " * l.Cg(l.digit^1 / tonumber, "Tmp_table_sizes")
local bytes             =  bytes_sent * space * tmp_tables * space * tmp_disk_tables * space * tmp_table_sizes * sep

local inno_db   = l.P"# InnoDB" * line
local use_db    = l.P"use " * line
local timestamp = l.P"SET timestamp=" * l.Cg((l.digit^1 / "%0000000000"), "Timestamp") * line
local sql       = l.Cg((l.P(1))^0, "Payload")

slow_query_grammar_5_1 = l.Ct(user * l.Cg(l.Ct(thread * query_5_1 * bytes), "Fields") * inno_db^0 * use_db^0 * timestamp * sql)
slow_query_grammar_5_5 = l.Ct(time_5_5 * user * l.Cg(l.Ct(query_5_5), "Fields") * timestamp * sql)

return M
