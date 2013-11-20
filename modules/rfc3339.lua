-- sample input
---------------
-- 1999-05-05T23:23:59.217-07:00

-- table output
---------------
-- hour=23 (string)
-- min=23 (string)
-- year=1999 (string)
-- month=05 (string)
-- day=05 (string)
-- sec=59 (string)
---- conditional table members
-- sec_frac=0.217 (number)
-- offset_sign=- (string)
-- offset_hour=7 (number)
-- offset_min=0 (number)

lpeg.locale(lpeg)

local date_fullyear = lpeg.Cg(lpeg.digit * lpeg.digit * lpeg.digit * lpeg.digit, "year")
local date_month = lpeg.Cg(lpeg.P"0" * lpeg.R"19"
                           + "1" * lpeg.R"02", "month")
local date_mday = lpeg.Cg(lpeg.P"0" * lpeg.R"19"
                          + lpeg.R"12" * lpeg.R"09"
                          + "3" * lpeg.R"01", "day")

local time_hour = lpeg.Cg(lpeg.R"01" * lpeg.R"09"
                          + "2" * lpeg.R"03", "hour")
local time_minute = lpeg.Cg(lpeg.R"05" * lpeg.R"09", "min")
local time_second = lpeg.Cg(lpeg.R"05" * lpeg.R"09", "sec")
local time_secfrac = lpeg.Cg(lpeg.P"." * lpeg.digit^1 / tonumber, "sec_frac")
local time_numoffset = lpeg.Cg(lpeg.P"+" + "-", "offset_sign") * 
lpeg.Cg(time_hour / tonumber, "offset_hour") * ":" * 
lpeg.Cg(time_minute / tonumber, "offset_min")
local time_offset = lpeg.P"Z" + time_numoffset

local partial_time = time_hour * ":" * time_minute * ":" * time_second * time_secfrac^-1
local full_date = date_fullyear * "-"  * date_month * "-" * date_mday
local full_time = partial_time * time_offset

local rfc3339 =  lpeg.Ct(full_date * "T" * full_time)

return rfc3339


