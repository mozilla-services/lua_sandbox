-- Using this as a reference: https://github.banksimple.com/gist/sjl/92b4b5750739a46d38a7
--

-- imports
local l = require "lpeg"
local tonumber = tonumber

l.locale(l)

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local space  = l.space^1
local sep    = l.P"\n"
local level  = l.alpha^5 -- Log level (5 characters)
local brack  = l.S("[]") -- [ and ]
local timestamp = (l.P(1) - brack)^1
local colon  = l.S(":")  -- :
local class  = (l.P(1) - colon)^1 -- com.domain.client.jobs.OutgoingQueue
local line   = (l.P(1) - sep)^0 * sep

-- Example: ERROR [2014-11-21 16:35:59,501] com.domain.client.jobs.OutgoingQueue: Error handling output file with job job-name
local logline = l.Cg(level, "Level")         -- ERROR
              * space
              * brack
              * l.Cg(timestamp, "Timestamp") -- 2014-11-21 16:35:59,501
              * brack
              * space
              * l.Cg(class, "class")         -- com.domain.client.jobs.OutgoingQueue
              * colon
              * space
              * l.Cg(line, "ErrorMessage")                   -- Error handling output...

-- TODO: Multiline, need to figure out how to capture the entire stack in a table
-- -- Example: ! com.domain.inet.ftp.Exception: java.io.IOException: FAILED
-- local stackline = l.P"!"
--                 * space
--                 * line   -- com.domain.inet.ftp.Exception
-- 
-- -- Example: ! at com.domain.inet.ftp.TransferMode.upload(Unknown Source)
-- local stackatline = l.P"!"
--                   * space
--                   * l.P"at"
--                   * space
--                   * line    -- com.domain.inet.ftp.Exception
-- 
-- -- Matches entire line, use this to gather stuff we don't parse properly
-- local miscline = line

-- local logevent = logline
--                * stackline
--                * stackatline
--                * miscline
--                * sep

local slf4j_severity = l.digit^1 / tonumber

local slf4j_log_levels_text = (
  (l.P"fatal"   + "FATAL")      / "6"
+ (l.P"error"   + "ERROR")      / "5"
+ (l.P"warn"    + "WARN")       / "4"
+ (l.P"info"    + "INFO")       / "3"
+ (l.P"debug"   + "DEBUG")      / "2"
+ (l.P"trace"   + "TRACE")      / "1"
) / tonumber

-- TODO: Map slf4j to standard log levels for "Severity"

logevent_grammar = l.Ct(logline)

return M
