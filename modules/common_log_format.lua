-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"
l.locale(l)
local string = require "string"
local dt = require "date_time"
local ip = require "ip_address"
local tonumber = tonumber
local ipairs = ipairs
local pairs = pairs
local error = error
local type = type

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

--[[ Nginx access log grammar generation and parsing --]]

local pct_encoded   = l.P"%" * l.xdigit * l.xdigit
local unreserved    = l.alnum + l.S"-._~"
local sub_delims    = l.S"!$&'()*+,;="

local last_literal  = l.space
local integer       = l.digit^1 / tonumber
local double        = l.digit^1 * "." * l.digit^1 / tonumber
local msec_time     = double / dt.seconds_to_ns
local host          = l.Ct(l.Cg(ip.v4, "value") * l.Cg(l.Cc"ipv4", "representation"))
                    + l.Ct(l.Cg(ip.v6, "value") * l.Cg(l.Cc"ipv6", "representation"))
                    + l.Ct(l.Cg((unreserved + pct_encoded + sub_delims)^1, "value") * l.Cg(l.Cc"hostname", "representation"))
local http_status   = l.digit * l.digit * l.digit / tonumber

local nginx_error_levels = l.Cg((
  l.P"debug"   / "7"
+ l.P"info"    / "6"
+ l.P"notice"  / "5"
+ l.P"warn"    / "4"
+ l.P"error"   / "3"
+ l.P"crit"    / "2"
+ l.P"alert"   / "1"
+ l.P"emerg"   / "0")
/ tonumber, "Severity")

local nginx_upstream_sep        = ", "
local nginx_upstream_gsep       = " : "
local nginx_upstream_addr       = l.C((1 - l.S(nginx_upstream_sep))^1)
local nginx_upstream_addrs      = nginx_upstream_addr * (nginx_upstream_sep * nginx_upstream_addr)^0
local nginx_upstream_time       = double + l.P"-" / function () return 0 end
local nginx_upstream_times      = nginx_upstream_time * (nginx_upstream_sep * nginx_upstream_time)^0
local nginx_upstream_lengths    = integer * (nginx_upstream_sep * integer)^0
local nginx_upstream_status     = http_status + l.P"-" / function () return 0 end
local nginx_upstream_statuses   = nginx_upstream_status * (nginx_upstream_sep * nginx_upstream_status)^0

local nginx_format_variables = {
    --arg_*
    --args
      binary_remote_addr    = l.P(1) * l.P(1) * l.P(1) * l.P(1)
    , body_bytes_sent       = l.Ct(l.Cg(integer, "value") * l.Cg(l.Cc"B", "representation"))
    , bytes_sent            = l.Ct(l.Cg(integer, "value") * l.Cg(l.Cc"B", "representation"))
    , connection            = integer
    , connection_requests   = integer
    , content_length        = l.Ct(l.Cg(integer, "value") * l.Cg(l.Cc"B", "representation"))
    --content_type
    --cookie_*
    --document_root
    --document_uri
    , host                  = host
    , hostname              = host
    --http_*
    , https                 = l.P"on"^-1
    , is_args               = l.P"?"^-1
    --limit_rate
    , msec                  = msec_time
    , nginx_version         = l.digit^1 * "." * l.digit^1 * "." * l.digit^1
    , pid                   = integer
    , pipe                  = l.P"p" + "."
    , proxy_protocol_addr   = host
    --query_string
    --realpath_root
    , remote_addr           = host
    , remote_port           = integer
    --remote_user
    --request
    --request_body
    --request_body_file
    , request_completion    = l.P"OK"^-1
    --request_filename
    , request_length        = l.Ct(l.Cg(integer, "value") * l.Cg(l.Cc"B", "representation"))
    , request_method        = l.P"GET" + "POST" + "HEAD" + "PUT" + "DELETE" + "OPTIONS" + "TRACE" + "CONNECT"
    , request_time          = l.Ct(l.Cg(double, "value") * l.Cg(l.Cc"s", "representation"))
    --request_uri
    , scheme                = l.P"https" + "http"
    --sent_http_*
    , server_addr           = host
    , server_name           = host
    , server_port           = integer
    , server_protocol       = l.P"HTTP/" * l.digit^1 * "." * l.digit^1
    , status                = http_status
    , tcpinfo_rtt           = integer
    , tcpinfo_rttvar        = integer
    , tcpinfo_snd_cwnd      = integer
    , tcpinfo_rcv_space     = integer
    , time_iso8601          = dt.rfc3339 / dt.time_to_ns
    , time_local            = dt.clf_timestamp / dt.time_to_ns
    --uri

    -- Upstream Module Grammars
    -- http://nginx.org/en/docs/http/ngx_http_upstream_module.html#variables
    , upstream_addr                 = l.Ct(nginx_upstream_addrs * (nginx_upstream_gsep * nginx_upstream_addrs)^0) + "-"
    , upstream_cache_status         = l.P"HIT" + "MISS" + "EXPIRED" + "BYPASS" + "STALE" + "UPDATING" + "REVALIDATED" + "-"
    , upstream_cache_last_modified  = dt.build_strftime_grammar("%a, %d %b %Y %T GMT") / dt.time_to_ns  + "-"
    , upstream_response_length      = l.Ct(l.Cg(l.Ct(nginx_upstream_lengths * (nginx_upstream_gsep * nginx_upstream_lengths)^0), "value") * l.Cg(l.Cc"B", "representation")) + "-"
    , upstream_response_time        = l.Ct(l.Cg(l.Ct(nginx_upstream_times * (nginx_upstream_gsep * nginx_upstream_times)^0), "value") * l.Cg(l.Cc"s", "representation"))
    , upstream_status               = l.Ct(nginx_upstream_statuses * (nginx_upstream_gsep * nginx_upstream_statuses)^0)
    --upstream_http_* handled by the generic grammar
}

local function get_string_grammar(s)
    if last_literal ~= '"' then
        return l.Cg((l.P(1) - last_literal)^0, s)
    else
        return l.Cg((l.P'\\"' + (l.P(1) - l.P'"'))^0, s)
    end
end

local function nginx_lookup_grammar(var)
    local g = nginx_format_variables[var]
    if not g then
        g = get_string_grammar(var)
    elseif var == "time_local" or var == "time_iso8601" or var == "msec" then
        g = l.Cg(g, "time")
    else
        g = l.Cg(g, var)
    end

    return g
end


local apache_format_variables = {
 ["%"] = l.P"%"
   , a = l.Cg(host, "remote_addr")
   , A = l.Cg(host, "server_addr")
   , B = l.Cg(nginx_format_variables["body_bytes_sent"], "body_bytes_sent")
   , b = l.Cg(l.Ct(l.Cg(integer + l.P"-" / function () return 0 end, "value") * l.Cg(l.Cc"B", "representation")), "body_bytes_sent")
   , D = l.Cg(l.Ct(l.Cg(integer, "value") * l.Cg(l.Cc"us", "representation")), "request_time")
   , f = "request_filename"
   , h = l.Cg(host, "remote_addr")
   , H = l.Cg(nginx_format_variables["server_protocol"], "server_protocol")
   , k = l.Cg(integer, "connection_requests")
   , l = "remote_user"
   , L = "request_log_id"
   , m = l.Cg(nginx_format_variables["request_method"], "request_method")
   , p = l.Cg(integer, "server_port")
   , P = l.Cg(integer, "pid")
   , q = "query_string"
   , r = "request"
   , R = "request_handler"
   , s = l.Cg(nginx_format_variables["status"], "status")
   , t = l.P"[" * l.Cg(nginx_format_variables["time_local"], "time")  * "]"
   , T = l.Cg(l.Ct(l.Cg(integer, "value") * l.Cg(l.Cc"s", "representation")), "request_time")
   , u = "remote_user"
   , U = "uri"
   , v = l.Cg(host, "server_name")
   , V = l.Cg(host, "server_name")
   , X = l.Cg(l.S"X+-", "connection_status")
   , I = l.Cg(nginx_format_variables["request_length"], "request_length")
   , O = l.Cg(nginx_format_variables["request_length"], "response_length")
   , S = l.Cg(nginx_format_variables["request_length"], "total_length")
}

local function port_format(a)
    if a == "canonical" or a == "local" then
        return l.Cg(integer, "server_port")
    elseif a == "remote" then
        return l.Cg(integer, "remote_port")
    end
    error("unknown port format: " .. a)
end

local function pid_format(a)
    if a == "pid" then
        return l.Cg(integer, "pid")
    elseif a == "tid" then
        return l.Cg(integer, "tid")
    elseif a == "hextid" then
        return l.Cg(l.xdigit^1 / function (n) return tonumber(n, 16) end, "tid")
    end
    error("unknown pid format: " .. a)
end

local function time_format(a)
    local pos = 1
    if string.find(a, "^end:") then
        pos = 5
    elseif string.find(a, "^begin:") then
        pos = 7
    end

    if string.find(a, "^sec", pos) then
        return l.Cg(l.digit^1 / dt.seconds_to_ns, "time")
    elseif string.find(a, "^msec_frac", pos) or string.find(a, "^usec_frac", pos) then
        return l.Cg(l.digit^1 / function (t) return tonumber("." .. t)  end, "sec_frac")
    elseif string.find(a, "^msec", pos) then
        return l.Cg(l.digit^1 / function (t) return tonumber(t) * 1e6 end, "time")
    elseif string.find(a, "^usec", pos) then
        return l.Cg(l.digit^1 / function (t) return tonumber(t) * 1e3 end, "time")
    end

    return l.Cg(dt.build_strftime_grammar(a) / dt.time_to_ns, "time")
end

local apache_format_arguments = {
     a = function(a) return apache_format_variables["a"] end
   , C = function(a) return "cookie_" .. a end
   , e = function(a) return "env_" .. a end
   , i = function(a) return "http_" .. string.gsub(string.lower(a), "-", "_") end -- make it compatible with the Nginx naming convention
   , n = function(a) return "module_" .. a end
   , o = function(a) return "sent_http_" .. string.gsub(string.lower(a), "-", "_") end -- make it compatible with the Nginx naming convention
   , p = port_format
   , P = pid_format
   , t = time_format
}

local function apache_lookup_variable(var)
    local g = apache_format_variables[var]
    if not g then
        error("unknown variable: " .. var)
    elseif type(g) == "string" then
        g = get_string_grammar(g)
    end

   return g
end

local function apache_lookup_argument(t)
    local f = apache_format_arguments[t.variable]
    if not f then
        error("unknown variable: " .. t.variable)
    end

    local g = f(t.argument)
    if not g then
        error(string.format("variable: %s invalid arguments: %s", t.variable, t.arguments))
    elseif type(g) == "string" then
        g = get_string_grammar(g)
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


local escape_chars = {t = "\9", n = "\10", ['"'] = '"', ["\\"] = "\\"}
local function substitute_escape(seq)
    seq = string.sub(seq, 2)
    local e = escape_chars[seq]
    if not e then
        e = string.char(tonumber(seq))
    end
    return e
end

local escape_sequence = l.Cs(((l.P"\\" * (l.S'tn"\\' + l.digit^-3)) / substitute_escape + 1)^0)
local function escape_literal_grammar(var)
    local literal = l.match(escape_sequence, var)
    last_literal = string.sub(literal, -1)
    return l.P(literal)
end

--[[ User Agent normalization
Derived from the Persona user agent parser
https://github.com/mozilla/persona/blob/dev/lib/coarse_user_agent_parser.js
]]--

-- find the version of the last product in the user agent string
local function ua_basic(ua)
    local cur, last = 0, nil
    while cur do
        cur = ua:find("/", cur+1)
        if cur then
            last = cur
        end
    end

    if last then
        return tonumber(ua:match("^(%d+)", last+1))
    end
end

-- find the version of the product identified by the keyword
local function ua_keyword(kw)
    return function(ua)
        local i = ua:find(kw)
        if i then
            local s = i + kw:len()
            if s then
                return tonumber(ua:match("^(%d+)", s))
            end
        end
    end
end

local ua_os_matchers = {
    -- search, replace
      {"iPod"                    ,"iPod"         }
    , {"iPad"                    ,"iPad"         }
    , {"iPhone"                  ,"iPhone"       }
    , {"Android"                 ,"Android"      }
    , {"BlackBerry"              ,"BlackBerry"   }
    , {"Linux"                   ,"Linux"        }
    , {"Macintosh"               ,"Macintosh"    }
    , {"Mozilla/5.0 (Mobile;"    ,"FirefoxOS"    }
    --http://en.wikipedia.org/wiki/Microsoft_Windows#Timeline_of_releases
    , {"Windows NT 10.0"         ,"Windows 10"   }
    , {"Windows NT 6.3"          ,"Windows 8.1"  }
    , {"Windows NT 6.2"          ,"Windows 8"    }
    , {"Windows NT 6.1"          ,"Windows 7"    }
    , {"Windows NT 6.0"          ,"Windows Vista"}
    , {"Windows NT 5.1"          ,"Windows XP"   }
    , {"Windows NT 5.0"          ,"Windows 2000" }
}

local ua_browser_matchers = {
      {"Chrome"        , ua_keyword("Chrome/")}
    , {"Opera Mini"    , ua_basic}
    , {"Opera Mobi"    , ua_basic}
    , {"Opera"         , ua_basic}
    , {"MSIE"          , ua_keyword("MSIE ")}
    , {"Safari"        , ua_basic}
    , {"Firefox"       , ua_keyword("Firefox/")}
}

--[[ Public Interface --]]

-- Returns an LPeg grammar based on the Nginx log_format configuration string.
function build_nginx_grammar(log_format)
    last_literal = l.space
    local ws = l.space^1 / space_grammar
    local variable = l.P"$" * ((l.alnum + "_")^1 / nginx_lookup_grammar)
    local literal =  (l.P(1) - (ws + variable))^1 / literal_grammar
    local item = ws + variable + literal

    local p = l.Ct(item * (item)^0)
    local t = p:match(log_format)
    if not t then
        error("could not parse the log_format configuration")
    end

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

-- Returns the user agent browser, version, and os
function normalize_user_agent(ua)
    if not ua then return end

    local browser, version, os
    for i, v in ipairs(ua_os_matchers) do
        if ua:find(v[1], 1, true) then
            os = v[2]
            break
        end
    end

    for i, v in ipairs(ua_browser_matchers) do
        if ua:find(v[1], 1, true) then
            browser = v[1]
            version = v[2](ua)
            break
        end
    end

    return browser, version, os
end

-- Returns an LPeg grammar based on the Apache LogFormat configuration string.
function build_apache_grammar(log_format)
    last_literal = l.space
    local ws = l.space^1 / space_grammar
    local http_status_code = l.digit * l.digit * l.digit
    local http_status_codes = l.P"!"^-1 * http_status_code * ("," * http_status_code)^0
    local variable = l.alpha + "%"
    local argument = "{" * l.Cg((l.P(1) - "}")^1, "argument") * "}"
    local directive = l.P"%" * l.S"<>"^-1 * (variable / apache_lookup_variable)
                    + "%" *  http_status_codes^-1 * l.Ct(argument * l.Cg(variable, "variable")) / apache_lookup_argument
    local literal =  (l.P(1) - (ws + directive))^1 / escape_literal_grammar
    local item = ws + directive + literal

    local p = l.Ct(item * (item)^0)
    local t = p:match(log_format)
    if not t then
        error("could not parse the log_format configuration")
    end

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

nginx_error_grammar = l.Ct(l.Cg(dt.build_strftime_grammar("%Y/%m/%d %T") / dt.time_to_ns, "Timestamp")
                           * l.space * "[" * nginx_error_levels * "]"
                           * l.space * l.Cg(l.digit^1 / tonumber, "Pid") * "#"
                           * l.Cg(l.Ct(l.Cg(l.digit^1 / tonumber, "tid") * ": " * (l.P"*" * l.Cg(l.digit^1 / tonumber, "connection") * " ")^-1), "Fields")
                           * l.Cg(l.P(1)^0, "Payload"))

return M
