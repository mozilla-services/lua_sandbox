-- imports
local l  = require "lpeg"
local dt = require "date_time"
local tonumber = tonumber

l.locale(l)

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local space     = l.space^1
local sep       = l.P"\n"
local class     = (l.P(1) - ":")^1 -- com.domain.client.jobs.OutgoingQueue
local msg       = (l.P(1) - sep)^0
local line      = (l.P(1) - sep)^0 * sep

-- slf4j uses the following levels/severity
-- "FATAL" = 6
-- "ERROR" = 5
-- "WARN"  = 4
-- "INFO"  = 3
-- "DEBUG" = 2
-- "TRACE" = 1

-- Map sfl4j log levels to syslog severity
local sfl4j_levels = l.Cg((
  (l.P"TRACE" + "trace") / "7"
+ (l.P"DEBUG" + "debug") / "7"
+ (l.P"INFO"  + "info")  / "6"
+ (l.P"WARN"  + "warn")  / "4"
+ (l.P"ERROR" + "error") / "3"
+ (l.P"FATAL" + "fatal") / "0")
/ tonumber, "severity")

local function convert_to_frac(sec)
    return tonumber("0." .. sec)
end

-- Example: 2014-11-21 16:35:59,501
local time_secfrac = l.Cg(l.digit^3 / convert_to_frac, "sec_frac")
local partial_time = dt.time_hour * ":" * dt.time_minute * ":" * dt.time_second * "," * time_secfrac^-1
local sfl4j_datetime = l.Ct(dt.rfc3339_full_date * space * partial_time) / dt.time_to_ns

-- Example: ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name
local logline = sfl4j_levels                 -- ERROR
              * space * "["
              * l.Cg(sfl4j_datetime, "timestamp") -- 2014-11-21 16:35:59,501
              * "]" * space
              * l.Cg(class, "class")         -- com.domain.client.jobs.OutgoingQueue
              * ":" * space
              * l.Cg(msg, "message")         -- Error handling output...

-- A representation of a full log event
local logevent = logline * sep * l.Cg(line^0, "stacktrace")

logevent_grammar = l.Ct(logevent)

return M
