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
local lbrack    = l.S("[")
local rbrack    = l.S("]")
local timestamp = (l.P(1) - rbrack)^0
local colon     = l.S(":")  -- :
local class     = (l.P(1) - colon)^1 -- com.domain.client.jobs.OutgoingQueue
local msg       = (l.P(1) - sep)^0
local line      = (l.P(1) - sep)^0 * sep

-- Example: ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name
local logline = l.Cg(level, "Level")         -- ERROR
              * space * lbrack
              * l.Cg(timestamp, "Timestamp") -- 2014-11-21 16:35:59,501
              * rbrack * space
              * l.Cg(class, "Class")    -- com.domain.client.jobs.OutgoingQueue
              * colon * space
              * l.Cg(msg, "Message")    -- Error handling output...

-- TODO: Multiline, need to figure out how to capture the entire stack in a table
-- Example: ! java.net.SocketTimeoutException: Read timed out
local stackline = l.P"!" * space * class * colon * space * msg * sep
-- Example: ! at com.domain.inet.ftp.TransferMode.upload(Unknown Source)
local stackatline = l.P"!" * space * l.P"at" * space * (l.P(1) - sep)^0 * sep


-- Match line, use this to gather stuff we don't parse properly
-- local misc = line

local logevent = logline * sep * l.Cg(stackline^0 * stackatline^0 * line^0, "Stacktrace")

logevent_grammar = l.Ct(logevent)

-- local slf4j_severity = l.digit^1 / tonumber
-- local slf4j_log_levels_text = (
--   (l.P"fatal"   + "FATAL")      / "6"
-- + (l.P"error"   + "ERROR")      / "5"
-- + (l.P"warn"    + "WARN")       / "4"
-- + (l.P"info"    + "INFO")       / "3"
-- + (l.P"debug"   + "DEBUG")      / "2"
-- + (l.P"trace"   + "TRACE")      / "1"
-- ) / tonumber
-- TODO: Map slf4j to standard log levels for "Severity"

return M
