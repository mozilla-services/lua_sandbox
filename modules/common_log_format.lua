-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"
l.locale(l)
local string = require "string"
local dt = require "date_time"
local tonumber = tonumber
local ipairs = ipairs

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local double = l.digit^1 * "." * l.digit^1
local msec_time = double / dt.seconds_to_ns

local nginx_format_variables = {
    body_bytes_sent = l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")),
    bytes_sent = l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")),
    connection_requests = l.digit^1 / tonumber,
    content_length = l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")),
    pid = l.digit^1 / tonumber,
    request_length = l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")),
    request_time =  l.Ct(l.Cg(double / tonumber, "value") * l.Cg(l.Cc"s", "representation")),
    status = l.digit^1 / tonumber,
    time_iso8601 = dt.rfc3339 / dt.time_to_ns,
    time_local = dt.clf_timestamp / dt.time_to_ns,
    msec = msec_time
}

local last_literal = l.space

local function nginx_lookup_grammar(var)
    local g = nginx_format_variables[var]
    if not g then
        g = l.Cg((l.P(1) - last_literal)^0, var)
    elseif var == "time_local" or var == "time_iso8601" or var == "msec" then
        g = l.Cg(g, "time")
    else
        g = l.Cg(g, var)
    end

   return g
end

local function space_grammar()
    last_literal = l.space
    return l.space^1
end

local function literal_grammar(var)
    last_literal = string.sub(var, -1)
    return l.P(var)
end

function build_nginx_grammar(log_format)
    local ws = l.C(l.space^1) / space_grammar
    local variable = l.P"$" * l.C((l.alnum + "_")^1) / nginx_lookup_grammar
    local literal =  l.C((1 - (ws + variable))^1) / literal_grammar
    local item = ws + variable + literal

    local p = l.Ct(item * (item)^0)
    local t = p:match(log_format)
    if not t then return nil end

    local grammar = nil
    for i,v in ipairs(t) do
        if not grammar then
            grammar = v
        else
            grammar = grammar * v
        end
    end
    return l.Ct(grammar)
end

return M
