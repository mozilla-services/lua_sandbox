-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local l = require "lpeg"
l.locale(l)
local math = require "math"
local string = require "string"
local dt = require "date_time"
local ip = require "ip_address"
local tonumber = tonumber
local error = error
local type = type
local ipairs = ipairs
local rawset = rawset

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local function unescape_param_value(param_value)
    return string.gsub(param_value, '\\([]"\\])', '%1')
end

local function prefix_param_name(param_name)
    return '_' .. param_name
end

-- http://tools.ietf.org/html/rfc5424#page-8
local octet           = l.P(1)
local utf8_string     = octet^0
local sp              = l.P" "
local printusascii    = l.R"!~"
local nonzero_digit   = l.R"19"
local digit           = l.R"09"
local nilvalue        = l.P"-"
local bom             = l.P("\239\187\191")
local msg_any         = octet^0
local msg_utf8        = bom * utf8_string
local msg             = msg_utf8 + msg_any
local hostname        = nilvalue + printusascii^-255
local sd_name         = (printusascii - l.S'=]" ')^-32
local param_name      = sd_name / prefix_param_name
local esc             = l.P'\\'
local param_value_esc = l.S'"\\]'
local param_value     = ((octet - param_value_esc) + (esc * octet))^0 / unescape_param_value
local sd_id           = l.Cg(l.Cc"id" * l.C(sd_name))
local sd_param        = l.Cg(param_name * '="' * param_value * '"')
local sd_params       = l.Cf(l.Ct"" * sd_id * (sp * sd_param)^0, rawset)
local sd_element      = l.P"[" * sd_params * "]"
local syslog_facility = digit^-3 / tonumber
local syslog_severity = digit / tonumber

local function convert_pri(pri)
    pri = tonumber(pri)
    local facility = math.floor(pri/8)
    local severity = pri % 8

    return {facility = facility, severity = severity}
end
local pri = digit^-3 / convert_pri

-- https://github.com/rsyslog/rsyslog/blob/35ef2408dfec0e8abdebd33c74578f9bb3299f20/runtime/msg.c#L298
local syslog_severity_text = (
  (l.P"debug"   + "DEBUG")      / "7"
+ (l.P"info"    + "INFO")       / "6"
+ (l.P"notice"  + "NOTICE")     / "5"
+ (l.P"warning" + "WARNING")    / "4"
+ (l.P"warn"    + "WARN")       / "4"
+ (l.P"error"   + "ERROR")      / "3"
+ (l.P"err"     + "ERR")        / "3"
+ (l.P"crit"    + "CRIT")       / "2"
+ (l.P"alert"   + "ALERT")      / "1"
+ (l.P"emerg"   + "EMERG")      / "0"
+ (l.P"panic"   + "PANIC")      / "0"
) / tonumber

local syslog_facility_text = (
  (l.P"kern"            + "KERN")           / "0"
+ (l.P"user"            + "USER")           / "1"
+ (l.P"mail"            + "MAIL")           / "2"
+ (l.P"daemon"          + "DAEMON")         / "3"
+ (l.P"auth"            + "AUTH")           / "4"
+ (l.P"security"        + "SECURITY")       / "4"
+ (l.P"syslog"          + "SYSLOG")         / "5"
+ (l.P"lpr"             + "LPR")            / "6"
+ (l.P"news"            + "NEWS")           / "7"
+ (l.P"uucp"            + "UUCP")           / "8"
+ (l.P"cron"            + "CRON")           / "9"
+ (l.P"authpriv"        + "AUTHPRIV")       / "10"
+ (l.P"ftp"             + "FTP")            / "11"
+ (l.P"ntp"             + "NTP")            / "12"
+ (l.P"audit"           + "AUDIT")          / "13"
+ (l.P"alert"           + "ALERT")          / "14"
+ (l.P"clock"           + "CLOCK")          / "15"
+ (l.P"local0"          + "LOCAL0")         / "16"
+ (l.P"local1"          + "LOCAL1")         / "17"
+ (l.P"local2"          + "LOCAL2")         / "18"
+ (l.P"local3"          + "LOCAL3")         / "19"
+ (l.P"local4"          + "LOCAL4")         / "20"
+ (l.P"local5"          + "LOCAL5")         / "21"
+ (l.P"local6"          + "LOCAL6")         / "22"
+ (l.P"local7"          + "LOCAL7")         / "23"
) / tonumber

local time_formats = {
["date-rfc3164"]            = dt.rfc3164_timestamp,
["date-rfc3164-buggyday"]   = dt.rfc3164_timestamp,
["date-mysql"]              = dt.mysql_timestamp,
["date-pgsql"]              = dt.pgsql_timestamp,
["date-rfc3339"]            = dt.rfc3339,
["date-unixtimestamp"]      = digit^1,
["date-subseconds"]         = digit^1
}

local function lookup_time_format(property)
    local f
    if property.options then
        local format = string.lower(property.options)
        f = time_formats[format]
    end
    if property.name == "timereported" then
        property.name = "timestamp"
    end

    if not f then
        f = time_formats["date-rfc3164"]
    end

    if format == "date-unixtimestamp" then
        f = f / dt.seconds_to_ns
    elseif format == "date-subseconds" then
        f = f / tonumber
    else
        f = f / dt.time_to_ns
    end

    return f
end

-- http://rsyslog-5-8-6-doc.neocities.org/property_replacer.html
local programname = (printusascii - l.S" :[")^0
local rsyslog_properties = {
   -- special case msg since the rsyslog template can break the rfc5424 msg rules
   rawmsg                  = octet^0,
   hostname                = hostname,
   source                  = hostname,
   fromhost                = hostname,
   ["fromhost-ip"]         = ip.v4 + ip.v6,
   syslogtag               = l.Ct(l.Cg(programname, "programname") * ("[" * l.Cg(l.digit^1 / tonumber, "pid") * "]")^-1 * l.P":"^-1),
   programname             = programname,
   pri                     = pri, -- pri table with facility and severity keys
   ["pri-text"]            = syslog_facility_text * "." * syslog_severity_text * "<" * pri * ">",
   iut                     = l.digit^1,
   syslogfacility          = syslog_facility,
   ["syslogfacility-text"] = syslog_facility_text,
   syslogseverity          = syslog_severity,
   ["syslogseverity-text"] = syslog_severity_text,
   syslogpriority          = syslog_severity,
   ["syslogpriority-text"] = syslog_severity_text,
   timegenerated           = lookup_time_format,
   timereported            = lookup_time_format,
   timestamp               = lookup_time_format,
   ["protocol-version"]    = digit^1,
   ["structured-data"]     = (l.Ct"" * nilvalue) + sd_element,
   ["app-name"]            = nilvalue + printusascii^-48,
   procid                  = nilvalue + printusascii^-128,
   msgid                   = nilvalue + printusascii^-32,
   inputname               = printusascii^0, -- doesn't appear to generate output (don't use it)
   ["$bom"]                = bom,
   ["$now"]                = dt.rfc3339_full_date,
   ["$year"]               = dt.date_fullyear,
   ["$month"]              = dt.date_month,
   ["$day"]                = dt.date_mday,
   ["$hour"]               = dt.time_hour,
   ["$hhour"]              = l.P"0" * l.S"01",
   ["$qhour"]              = l.P"0" * l.R"03",
   ["$minute"]             = dt.time_minute
}

local function space_grammar()
    return l.space
end

local last_literal = l.P"\n"
local function rsyslog_lookup(property)
   if property.options and property.options == "sp-if-no-1st-sp" then
       return sp^1
   end

   property.name = string.lower(property.name)
   local g
   if property.name == "msg" then
       g = l.Cg((l.P(1) - last_literal)^0, property.name) -- todo account for an escaped literal
   else
       g = rsyslog_properties[property.name]
   end

   if not g then
       error(string.format("invalid rsyslog property: '%s'", property.name))
   end

   if type(g) == "function" then
       g = g(property)
   end
   last_literal = l.P"\n"
   return l.Cg(g, property.name)
end

local escape_chars = {t = "\9", n = "\10", r = "\13", ['"'] = '"', ["'"] = "'"
    , ["\\"] = "\\", ["%"] = "%", a = "\7", b = "\8", f = "\12", v = "\11"}
local function substitute_escape(seq)
    seq = string.sub(seq, 2)
    local e = escape_chars[seq]
    if not e then
        e = string.char(tonumber(seq))
    end
    return e
end
local escape_sequence = l.Cs(((l.P"\\" * (l.S'tnr"\'\\%abfv' + digit^-3)) / substitute_escape + 1)^0)
local function literal_grammar(var)
    local literal = l.match(escape_sequence, var)
    last_literal = string.sub(literal, -1)
    return l.P(literal)
end

--
-- Public Interface
--

-- http://rsyslog-5-8-6-doc.neocities.org/rsyslog_conf_templates.html
function build_rsyslog_grammar(template)
    local ws = l.space / space_grammar
    local options = l.P":" * l.Cg((1 - l.P"%")^0, "options") -- todo support multiple options
    local tochar = l.Cg((1 - l.S":%")^0, "tochar")
    local fromchar = l.Cg((1 - l.P":")^0, "fromchar")
    local substr = l.P":" * fromchar * ":" * tochar
    local propname = l.Cg((l.alnum + l.S"-$")^1, "name")
    local property = l.P"%" * l.Ct(propname * substr^-1 * options^-1) / rsyslog_lookup * l.P"%"
    local literal =  (l.P(1) - (ws + property))^1 / literal_grammar
    local item = ws + property + literal

    local p = l.Ct(item * (item)^0)
    local t = p:match(template)
    if not t then
        error("could not parse the rsyslog template")
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

severity = syslog_severity_text

return M
