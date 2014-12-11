-- Using this as a reference: https://github.banksimple.com/gist/sjl/92b4b5750739a46d38a7
--

-- imports
local l  = require "lpeg"
local tonumber = tonumber

l.locale(l)

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local space     = l.space^1
local sep       = l.P"\n"
local level     = l.alpha^5 -- Log level (5 characters)
local timestamp = (l.P(1) - l.P("]"))^0
local class     = (l.P(1) - l.P(":"))^1 -- com.domain.client.jobs.OutgoingQueue
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
/ tonumber, "Severity")

-- Example: ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name
local logline = sfl4j_levels                 -- ERROR
              * space * l.P("[")
              * l.Cg(timestamp, "Timestamp") -- 2014-11-21 16:35:59,501
              * l.P("]") * space
              * l.Cg(class, "Class")         -- com.domain.client.jobs.OutgoingQueue
              * l.P(":") * space
              * l.Cg(msg, "Message")         -- Error handling output...

-- Example: ! java.net.SocketTimeoutException: Read timed out
local stackline = l.P"!" * space * line
-- Example: ! at com.domain.inet.ftp.TransferMode.upload(Unknown Source)
local stackatline = l.P"!" * space * l.P"at" * space * line

-- A representation of a full log event
local logevent = logline * sep * l.Cg(stackline^0 * stackatline^0 * line^0, "Stacktrace")

logevent_grammar = l.Ct(logevent)

return M
