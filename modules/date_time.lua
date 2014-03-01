-- Imports
local l = require "lpeg"
l.locale(l)
local os = require "os"
local string = require "string"
local tonumber = tonumber

-- Verify TZ
local offset = "([+-])(%d%d)(%d%d)"
local tz = os.date("%z")
local sign, hour, min  = tz:match(offset)
if not(tz == "UTC" or (sign and tonumber(hour) == 0 and tonumber(min) == 0)) then
    error("TZ must be set to UTC")
end

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

--[[ Utility Functions --]]
-- Converts a time table into the number of nanoseconds since the UNIX epoch
function time_to_ns(t)
    if not t then return 0 end

    local offset = 0
    if t.offset_hour then
        offset = (t.offset_hour * 60 * 60) + (t.offset_min * 60)
        if t.offset_sign == "+" then offset = offset * -1 end
    end

    local frac = 0
    if t.sec_frac then
        frac = t.sec_frac
    end
    return (os.time(t) + frac + offset) * 1e9
end

--Converts a second value into the number of nanoseconds since the UNIX epoch
function seconds_to_ns(sec)
    return tonumber(sec) * 1e9
end


--[[ Generic Grammars --]]
date_fullyear = l.Cg(l.digit * l.digit * l.digit * l.digit, "year")

date_month = l.Cg(l.P"0" * l.R"19"
                     + "1" * l.R"02", "month")
date_mabbr = l.Cg(
    l.P"Jan"   /"1"
    + l.P"Feb" /"2"
    + l.P"Mar" /"3"
    + l.P"Apr" /"4"
    + l.P"May" /"5"
    + l.P"Jun" /"6"
    + l.P"Jul" /"7"
    + l.P"Aug" /"8"
    + l.P"Sep" /"9"
    + l.P"Oct" /"10"
    + l.P"Nov" /"11"
    + l.P"Dec" /"12"
    , "month")

date_mday = l.Cg(l.P"0" * l.R"19"
                    + l.R"12" * l.R"09"
                    + "3" * l.R"01", "day")

time_hour = l.Cg(l.R"01" * l.digit
                    + "2" * l.R"03", "hour")

time_minute = l.Cg(l.R"05" * l.digit, "min")

time_second = l.Cg(l.R"05" * l.digit
                           + "60", "sec") -- include leap second

time_secfrac = l.Cg(l.P"." * l.digit^1 / tonumber, "sec_frac")


--[[ RFC3339 grammar
sample input:  1999-05-05T23:23:59.217-07:00

output table:
hour=23 (string)
min=23 (string)
year=1999 (string)
month=05 (string)
day=05 (string)
sec=59 (string)
*** conditional table members ***
sec_frac=0.217 (number)
offset_sign=- (string)
offset_hour=7 (number)
offset_min=0 (number)
--]]
rfc3339_time_numoffset = l.Cg(l.S"+-", "offset_sign")
* l.Cg(time_hour / tonumber, "offset_hour")
* ":"
* l.Cg(time_minute / tonumber, "offset_min")

rfc3339_time_offset     = l.P"Z" + rfc3339_time_numoffset
rfc3339_partial_time    = time_hour * ":" * time_minute * ":" * time_second * time_secfrac^-1
rfc3339_full_date       = date_fullyear * "-"  * date_month * "-" * date_mday
rfc3339_full_time       = rfc3339_partial_time * rfc3339_time_offset
rfc3339                 = l.Ct(rfc3339_full_date * "T" * rfc3339_full_time)


--[[ strftime Grammars --]]
strftime_numoffset = l.Cg(l.S"+-", "offset_sign")
* l.Cg(time_hour / tonumber, "offset_hour")
* l.Cg(time_minute / tonumber, "offset_min")


--[[ Common Log Format Grammars --]]
-- strftime format %d/%b/%Y:%H:%M:%S %z.
clf_timestamp = l.Ct(date_mday * "/" * date_mabbr * "/" * date_fullyear * ":" * time_hour * ":" * time_minute * ":" * time_second * " " * strftime_numoffset)


--[[ RFC3164 Grammars --]]
local sp = l.P" "
-- since we consume multiple spaces this will also handle date-rfc3164-buggyday
rfc3164_timestamp = l.Ct(date_mabbr * sp^1 * l.Cg(l.digit^-2, "day") * sp^1 * time_hour * ":" * time_minute * ":" * time_second * l.Cg(l.Cc("") / function() return os.date("%Y") end, "year"))


--[[ Database Grammars --]]
mysql_timestamp = l.Ct(date_fullyear * date_month * date_mday * time_hour * time_minute * time_second)

pgsql_timestamp = l.Ct(rfc3339_full_date * sp * time_hour * ":" * time_minute * ":" * time_second)

return M
