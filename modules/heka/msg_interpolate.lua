-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Simple Templating to Transform Message Fields into Strings

## Functions

### interpolate

Interpolates values from the currently processed message into the provided
string value. A `%{}` enclosed field name will be replaced by the field value
from the current message. All message header fields are supported ("Uuid",
"Timestamp", "Type", "Logger", "Severity", "Payload", "EnvVersion", "Pid",
"Hostname"). Any other values will be checked against the defined dynamic
message fields. If no field matches, then a
[C strftime](http://man7.org/linux/man-pages/man3/strftime.3.html) (\*nix)
or
[C89 strftime](http://msdn.microsoft.com/en-us/library/fe06s4ak.aspx) (Windows)
time substitution will be attempted. The time used for time substitution will be
the seconds-from-epoch timestamp passed in as the `secs` argument, if provided.
If `secs` is nil, local system time is used. Note that the message timestamp is
*not* automatically used; if you want to use the message timestamp for time
substitutions, then you need to manually extract it and convert it from
nanoseconds to seconds (i.e. divide by 1e9).

*Arguments*
* value (string) - String into which message values should be interpolated.
* secs (number or nil) - Timestamp (in seconds since epoch) to use for time
  substitutions. If nil, system time will be used.

*Return*
* string - Original string value with any interpolated message values.
--]]

local date          = require "os".date
local floor         = require "math".floor
local string        = require "string"
local read_message  = read_message
local tostring      = tostring
local type          = type

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module.

local function interpolate_match(header_fields, match, secs)
    -- First see if it's a message header.
    local fp = header_fields[match]
    if fp then return fp() end

    -- Second check for a dynamic field.
    local fname = string.format("Fields[%s]", match)
    local fval = read_message(fname)
    if type(fval) == "boolean" then
        return tostring(fval)
    elseif fval then
        return fval
    end
    -- Finally try to use it as a strftime format string.
    fval = date(match, secs)
    if fval ~= match then  -- Only return it if a substitution happened.
        return fval
    end
end

--[[ Public Interface --]]

header_fields = {
    Uuid        = function() return get_uuid(read_message("Uuid")) end,
    Timestamp   = function() return get_timestamp(read_message("Timestamp")) end,
    Type        = function() return read_message("Type") end,
    Logger      = function() return read_message("Logger") end,
    Severity    = function() return read_message("Severity") end,
    Payload     = function() return read_message("Payload") end,
    EnvVersion  = function() return read_message("EnvVersion") end,
    Pid         = function() return read_message("Pid") end,
    Hostname    = function() return read_message("Hostname") end
}


function get_uuid(uuid)
    return string.format("%X%X%X%X-%X%X-%X%X-%X%X-%X%X%X%X%X", string.byte(uuid, 1, 16))
end


function get_timestamp(ns)
    local time_t = floor(ns / 1e9)
    local frac = ns - time_t * 1e9
    local ds = date("!%Y-%m-%dT%H:%M:%S", time_t)
    return string.format("%s.%09dZ", ds, frac)
end


function interpolate(value, secs)
    return string.gsub(value, "%%{(.-)}", function(match) return interpolate_match(header_fields, match, secs) end)
end

return M
