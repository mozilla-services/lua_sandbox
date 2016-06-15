-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# MySQL Module

## Variables
### LPEG Grammars
* `slow_query_grammar`
* `mariadb_slow_query_grammar`
* `percona_slow_query_grammar`
* `short_slow_query_grammar`
* `mariadb_short_slow_query_grammar`
--]]

-- Imports
local l = require "lpeg"
l.locale(l)
local tonumber = tonumber
local ip = require "lpeg.ip_address"

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local unreserved    = l.alnum + l.S"-._~"
local space         = l.space^1
local sep           = l.P"\n"
local sql_end       = l.P";" * (l.P"\n" + -1)
local line          = (l.P(1) - sep)^0 * sep
local float         = l.digit^1 * "." * l.digit^1 / tonumber
local integer       = l.P"-"^-1 * l.digit^1 / tonumber

local time          = l.P"# Time: " * line

local user_legal    = (1 - l.S"[]")
local user_name     = user_legal^0 * "[" * l.Cg(user_legal^0, "Username") * "]"
local host_name     = ip.hostname^0 * l.space^0 * "[" * l.Cg((l.P(1) - "]")^1, "Hostname") * "]"
local user          = l.P"# User@Host: " * user_name * space * "@" * space * host_name * sep

local query_time    = l.P"# Query_time: " * l.Cg(l.Ct(l.Cg(float, "value") * l.Cg(l.Cc"s", "representation")), "Query_time")
local lock_time     = l.P"Lock_time: " * l.Cg(l.Ct(l.Cg(float, "value") * l.Cg(l.Cc"s", "representation")), "Lock_time")
local rows_sent     = l.P"Rows_sent: " * l.Cg(integer, "Rows_sent")
local rows_examined = l.P"Rows_examined: " * l.Cg(integer, "Rows_examined")
local query         = query_time * space * lock_time * space * rows_sent * space * rows_examined * sep

local use_db        = l.P"use " * line

local last_insert   = l.P"last_insert_id=" * l.Cg(integer, "Last_insert") * ","
local insert        = l.P"insert_id=" * l.Cg(integer, "Insert_id") * ","
local timestamp     = l.P"timestamp=" * l.Cg((l.digit^1 / "%0000000000"), "Timestamp")
local set           = l.P"SET " * last_insert^0 * insert^0 * timestamp * ";" * sep

local admin         = l.P"# administrator command: " * line

local sql           = l.Cg((l.P(1) - sql_end)^0 * sql_end, "Payload")

-- Maria DB extensions
local yes_no        = l.C(l.P"Yes" + "No")
local thread_id     = l.P"# Thread_id: " * l.Cg(integer, "Thread_id")
                    * l.P"  Schema: " * l.Cg(unreserved^0, "Schema")
                    * l.P"  QC_hit: " * l.Cg(yes_no, "QC_hit") * sep

local full_scan     = l.P"# Full_scan: " * l.Cg(yes_no, "Full_scan")
                    * "  Full_join: " * l.Cg(yes_no, "Full_join")
                    * "  Tmp_table: " * l.Cg(yes_no, "Tmp_table")
                    * "  Tmp_table_on_disk: " * l.Cg(yes_no, "Tmp_table_on_disk") * sep
                    * l.P"# Filesort: " * l.Cg(yes_no, "Filesort")
                    * "  Filesort_on_disk: " * l.Cg(yes_no, "Filesort_on_disk")
                    * "  Merge_passes: " * l.Cg(integer, "Merge_passes") * sep

-- Percona extensions
local percona_user     = "# User@Host: " * user_name * space * "@" * space * host_name * sep^0
local conn_id          = "  Id:" * space * l.Cg(integer, "Connection_id") * sep

local info             = l.P(("# Thread_id: " * l.Cg(integer, "Thread_id")
                       * "  Schema: " * l.Cg(unreserved^0, "Schema")
                       * "  Last_errno: " * l.Cg(integer, "Last_errno")
                       * "  Killed: " * l.Cg(integer, "Killed"))
                       + ("# Schema: " * l.Cg(unreserved^0, "Schema")
                       * "  Last_errno: " * l.Cg(integer, "Last_errno")
                       * "  Killed: " * l.Cg(integer, "Killed"))) * sep

local percona_query    = "# Query_time: " * l.Cg(l.Ct(l.Cg(float / tonumber, "value")
                       * l.Cg(l.Cc"s", "representation")), "Query_time")
                       * "  Lock_time: " * l.Cg(l.Ct(l.Cg(float / tonumber, "value")
                       * l.Cg(l.Cc"s", "representation")), "Lock_time")
                       * "  Rows_sent: " * l.Cg(integer, "Rows_sent")
                       * "  Rows_examined: " * l.Cg(integer, "Rows_examined")
                       * "  Rows_affected: " * l.Cg(integer, "Rows_affected")
                       * ("  Rows_read: " * l.Cg(integer, "Rows_read"))^0 * sep

local memory_footprint = "# Bytes_sent: " * l.Cg(l.Ct(l.Cg(integer, "value")
                       * l.Cg(l.Cc"B", "representation")), "Bytes_sent")
                       * ("  Tmp_tables: " * l.Cg(integer, "Tmp_tables")
                       * "  Tmp_disk_tables: " * l.Cg(integer, "Tmp_disk_tables")
                       * "  Tmp_table_sizes: " * l.Cg(l.Ct(l.Cg(integer, "value")
                       * l.Cg(l.Cc"B", "representation")), "Tmp_table_sizes"))^0 * sep

local stored_routine   = "# Stored routine: " * l.Cg((l.alnum + l.S"_.-")^1, "Stored_routine") * sep

local query_plan_info  = "# QC_Hit: " * l.Cg(yes_no, "QC_hit")
                       * "  Full_scan: " * l.Cg(yes_no, "Full_scan")
                       * "  Full_join: " * l.Cg(yes_no, "Full_join")
                       * "  Tmp_table: " * l.Cg(yes_no, "Tmp_table")
                       * "  Tmp_table_on_disk: " * l.Cg(yes_no, "Tmp_table_on_disk") * sep
                       * "# Filesort: " * l.Cg(yes_no, "Filesort")
                       * "  Filesort_on_disk: " * l.Cg(yes_no, "Filesort_on_disk")
                       * "  Merge_passes: " * l.Cg(integer, "Merge_passes") * sep

local innodb_usage     = "#   InnoDB_IO_r_ops: " * l.Cg(integer, "InnoDB_IO_r_ops")
                       * "  InnoDB_IO_r_bytes: " * l.Cg(l.Ct(l.Cg(integer, "value")
                       * l.Cg(l.Cc"B", "representation")), "InnoDB_IO_r_bytes")
                       * "  InnoDB_IO_r_wait: " * l.Cg(l.Ct(l.Cg(float, "value")
                       * l.Cg(l.Cc"s", "representation")), "InnoDB_IO_r_wait") * sep
                       * "#   InnoDB_rec_lock_wait: " * l.Cg(l.Ct(l.Cg(float, "value")
                       * l.Cg(l.Cc"s", "representation")), "InnoDB_rec_lock_wait")
                       * "  InnoDB_queue_wait: " * l.Cg(l.Ct(l.Cg(float, "value")
                       * l.Cg(l.Cc"s", "representation")), "InnoDB_queue_wait") * sep
                       * "#   InnoDB_pages_distinct: " * l.Cg(integer, "InnoDB_pages_distinct") * sep

slow_query_grammar          = l.Ct(time^0 * user * l.Cg(l.Ct(query), "Fields") * use_db^0 * set * admin^0 * sql)
mariadb_slow_query_grammar  = l.Ct(time^0 * user * l.Cg(l.Ct(thread_id * query * full_scan^0), "Fields") * use_db^0 * set * admin^0 * sql)
percona_slow_query_grammar  = l.Ct(time^0 * percona_user * l.Cg(l.Ct(conn_id^0 * info^0 * percona_query * memory_footprint^0 * stored_routine^0 * query_plan_info^0 * innodb_usage^0), "Fields") * use_db^0 * set * admin^0 * sql)

short_slow_query_grammar         = l.Ct(l.Cg(l.Ct(query), "Fields") * use_db^0 * set * admin^0 * sql)
mariadb_short_slow_query_grammar = l.Ct(l.Cg(l.Ct(thread_id * query * full_scan^0), "Fields") * use_db^0 * set * admin^0 * sql)

return M
