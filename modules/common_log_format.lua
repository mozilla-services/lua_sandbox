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

local msec_time = l.Cg((l.digit^1 * "." * l.digit^1) / dt.seconds_to_ns, "time")

local nginx_format_variables = {
    body_bytes_sent = l.Cg(l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")), "body_bytes_sent"),
    bytes_sent = l.Cg(l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")), "bytes_sent"),
    connection_requests = l.Cg(l.digit^1 / tonumber, "connection_requests"),
    content_length = l.Cg(l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")), "content_length"),
    pid = l.Cg(l.digit^1 / tonumber, "pid"),
    request_length = l.Cg(l.Ct(l.Cg(l.digit^1 / tonumber, "value") * l.Cg(l.Cc"B", "representation")), "request_length"),
    request_time =  l.Cg(l.Ct(l.Cg((l.digit^1 * "." * l.digit^1) / tonumber, "value") * l.Cg(l.Cc"s", "representation")), "request_time"),
    status = l.Cg(l.digit^1 / tonumber, "status"),
    time_iso8601 = l.Cg(dt.rfc3339 / dt.time_to_ns, "time"),
    time_local = l.Cg(dt.clf_timestamp / dt.time_to_ns, "time"),
    msec = msec_time
}

local last_literal = l.space

local function nginx_lookup_grammar(var)
    local g = nginx_format_variables[var]
    if not g then
        g = l.Cg((l.P(1) - last_literal)^0, var)
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
