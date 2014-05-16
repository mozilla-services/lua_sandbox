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
local sql_end       = l.P";\n"
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
local query         = l.Ct(query_time * space * lock_time * space * rows_sent * space * rows_examined * sep)

local use_db        = l.P"use " * line

local last_insert   = l.P"last_insert_id=" * l.digit^1 * ","
local insert        = l.P"insert_id=" * l.digit^1 * ","
local timestamp     = l.P"timestamp=" * l.Cg((l.digit^1 / "%0000000000"), "Timestamp")
local set           = l.P"SET " * last_insert^0 * insert^0 * timestamp * ";" * sep

local admin         = l.P"# administrator command: " * line

local sql           = l.Cg((l.P(1) - sql_end)^0 * sql_end, "Payload")

slow_query_grammar          = l.Ct(time^0 * user *  l.Cg(query, "Fields") * use_db^0 * set * admin^0 * sql)
short_slow_query_grammar    = l.Ct(l.Cg(query, "Fields") * use_db^0 * set * admin^0 * sql)

return M
