-- Imports
local l = lpeg
l.locale(l)
local os = os
local string = string
local tonumber = tonumber

-- Verify TZ
local offset = "([+-])(%d%d)(%d%d)"
local sign, hour, min  = os.date("%z"):match(offset)
if not sign or tonumber(hour) ~= 0 or tonumber(min) ~= 0 then 
    error("TZ must be set to UTC") 
end

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

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

local date_fullyear = l.Cg(l.digit * l.digit * l.digit * l.digit, "year")
local date_month = l.Cg(l.P"0" * l.R"19"
                           + "1" * l.R"02", "month")
local date_mday = l.Cg(l.P"0" * l.R"19"
                          + l.R"12" * l.R"09"
                          + "3" * l.R"01", "day")

local time_hour = l.Cg(l.R"01" * l.digit
                          + "2" * l.R"03", "hour")
local time_minute = l.Cg(l.R"05" * l.digit, "min")
local time_second = l.Cg(l.R"05" * l.digit 
                            + "60", "sec")
local time_secfrac = l.Cg(l.P"." * l.digit^1 / tonumber, "sec_frac")
local time_numoffset = l.Cg(l.S"+-", "offset_sign") * 
l.Cg(time_hour / tonumber, "offset_hour") * ":" * 
l.Cg(time_minute / tonumber, "offset_min")
local time_offset = l.S"Zz" + time_numoffset

local partial_time = time_hour * ":" * time_minute * ":" * time_second * time_secfrac^-1
local full_date = date_fullyear * "-"  * date_month * "-" * date_mday
local full_time = partial_time * time_offset

grammar =  l.Ct(full_date * l.S"Tt " * full_time)

-- Utility function to convert the table output into the number of nanoseconds since the UNIX epoch
function time_ns(t)
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

return M
