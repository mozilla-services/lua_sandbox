-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"
l.locale(l)
local tonumber = tonumber

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local space         = l.space^1
local sep           = l.P"\n"
local sql_end       = l.P";" * (l.P"\n" + -1)
local line          = (l.P(1) - sep)^0 * sep
local float         = l.digit^1 * "." * l.digit^1

local time          = l.P"# Time: " * line

local user_name     = l.alpha^1 * "[" * l.alpha^1 * "]"
local host_name     = l.alpha^0 * l.space^0 * "[" * l.Cg((l.P(1) - "]")^1, "Hostname") * "]"
local user          = l.P"# User@Host: " * user_name * space * "@" * space * host_name * sep

local query_time    = l.P"# Query_time: " * l.Cg(l.Ct(l.Cg(float / tonumber, "value") * l.Cg(l.Cc"s", "representation")), "Query_time")
local lock_time     = l.P"Lock_time: " * l.Cg(l.Ct(l.Cg(float / tonumber, "value") * l.Cg(l.Cc"s", "representation")), "Lock_time")
local rows_sent     = l.P"Rows_sent: " * l.Cg(l.digit^1 / tonumber, "Rows_sent")
local rows_examined = l.P"Rows_examined: " * l.Cg(l.digit^1 / tonumber, "Rows_examined")
local query         = query_time * space * lock_time * space * rows_sent * space * rows_examined * sep

local use_db        = l.P"use " * line

local last_insert   = l.P"last_insert_id=" * l.digit^1 * ","
local insert        = l.P"insert_id=" * l.digit^1 * ","
local timestamp     = l.P"timestamp=" * l.Cg((l.digit^1 / "%0000000000"), "Timestamp")
local set           = l.P"SET " * last_insert^0 * insert^0 * timestamp * ";" * sep

local admin         = l.P"# administrator command: " * line

local sql           = l.Cg((l.P(1) - sql_end)^0 * sql_end, "Payload")

-- Maria DB extensions
local yes_no        = l.C(l.P"Yes" + "No")
local thread_id     = l.P"# Thread_id: " * l.Cg(l.digit^1 / tonumber, "Thread_id")
                    * l.P"  Schema: " * l.Cg(l.alnum^0, "Schema")
                    * l.P"  QC_hit: " * l.Cg(yes_no, "QC_hit") * sep

local full_scan     = l.P"# Full_scan: " * l.Cg(yes_no, "Full_scan")
                    * l.P"  Full_join: " * l.Cg(yes_no, "Full_join")
                    * l.P"  Tmp_table: " * l.Cg(yes_no, "Tmp_table")
                    * l.P"  Tmp_table_on_disk: " * l.Cg(yes_no, "Tmp_table_on_disk") * sep
                    * l.P"# Filesort: " * l.Cg(yes_no, "Filesort")
                    * "  Filesort_on_disk: " * l.Cg(yes_no, "Filesort_on_disk")
                    * "  Merge_passes: " * l.Cg(l.digit^1 / tonumber, "Merge_passes") * sep

slow_query_grammar          = l.Ct(time^0 * user * l.Cg(l.Ct(query), "Fields") * use_db^0 * set * admin^0 * sql)
mariadb_slow_query_grammar  = l.Ct(time^0 * user * l.Cg(l.Ct(thread_id * query * full_scan^0), "Fields") * use_db^0 * set * admin^0 * sql)

short_slow_query_grammar        = l.Ct(l.Cg(l.Ct(query), "Fields") * use_db^0 * set * admin^0 * sql)
mariadb_short_slow_query_grammar= l.Ct(l.Cg(l.Ct(thread_id * query * full_scan^0), "Fields") * use_db^0 * set * admin^0 * sql)

return M
