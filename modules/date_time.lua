-- Imports
local l = require "lpeg"
l.locale(l)
local os = require "os"
local string = require "string"
local tonumber = tonumber
local ipairs = ipairs
local error = error

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

--[[ Utility Functions --]]
-- Converts a time table into the number of nanoseconds since the UNIX epoch
function time_to_ns(t)
    if not t then return 0 end
    if t.time_t then return t.time_t * 1e9 end

    local offset = 0
    if t.offset_hour then
        offset = (t.offset_hour * 60 * 60) + (t.offset_min * 60)
        if t.offset_sign == "+" then offset = offset * -1 end
    end

    if t.period then
        t.hour = tonumber(t.hour)
        if t.period == "AM" then
            if t.hour == 12 then
                t.hour = 0
            end
        elseif t.period == "PM" then
            if t.hour < 12 then
                t.hour = t.hour + 12
            end
        end
    end

    local frac = 0
    if t.sec_frac then
        frac = t.sec_frac
    end

    local ost = os.time(t)
    if not ost then
        return 0
    end

    return (ost + frac + offset) * 1e9
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
      l.P"Jan" /"1"
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

date_mfull = l.Cg(
      l.P"January"      /"1"
    + l.P"February"     /"2"
    + l.P"March"        /"3"
    + l.P"April"        /"4"
    + l.P"May"          /"5"
    + l.P"June"         /"6"
    + l.P"July"         /"7"
    + l.P"August"       /"8"
    + l.P"September"    /"9"
    + l.P"October"      /"10"
    + l.P"November"     /"11"
    + l.P"December"     /"12"
    , "month")

date_mday = l.Cg(l.P"0" * l.R"19"
                    + l.R"12" * l.R"09"
                    + "3" * l.R"01", "day")

date_mday_sp = l.Cg(l.P" " * l.R"19"
                    + l.S"12" * l.digit
                    + "3" * l.S"01", "day")

time_hour = l.Cg(l.R"01" * l.digit
                    + "2" * l.R"03", "hour")

time_minute = l.Cg(l.R"05" * l.digit, "min")

time_second = l.Cg(l.R"05" * l.digit
                           + "60", "sec") -- include leap second

time_secfrac = l.Cg(l.P"." * l.digit^1 / tonumber, "sec_frac")

timezone =
      l.P"UTC" * l.Cg(l.Cc"+", "offset_sign") * l.Cg(l.Cc"00" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"PST" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"08" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"PDT" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"07" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"MST" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"07" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"MDT" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"06" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"CST" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"06" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"CDT" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"05" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"EST" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"05" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.P"EDT" * l.Cg(l.Cc"-", "offset_sign") * l.Cg(l.Cc"04" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")
    + l.alpha^-5 * l.Cg(l.Cc"+", "offset_sign") * l.Cg(l.Cc"00" / tonumber, "offset_hour") * l.Cg(l.Cc"00" / tonumber, "offset_min")

timezone_offset = l.Cg(l.S"+-", "offset_sign") * l.Cg(time_hour / tonumber, "offset_hour") * l.Cg(time_minute / tonumber, "offset_min")

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
local century = os.date("%Y"):sub(1,2)
local strftime_specifiers = {}
strftime_specifiers["a"] = l.P"Mon" + "Tue" + "Wed" + "Thu" + "Fri" + "Sat" + "Sun"
strftime_specifiers["A"] = l.P"Monday"  + "Tuesday"  + "Wednesday" + "Thursday"  + "Friday" + "Saturday"  + "Sunday"
strftime_specifiers["b"] = date_mabbr
strftime_specifiers["B"] = date_mfull
strftime_specifiers["C"] = l.digit * l.digit
strftime_specifiers["y"] = l.Cg((l.digit * l.digit) / function (yy) return century .. yy end, "year")
strftime_specifiers["d"] = date_mday
strftime_specifiers["D"] = date_month * "/" * date_mday * "/" * strftime_specifiers["y"]
strftime_specifiers["e"] = date_mday_sp
strftime_specifiers["F"] = date_fullyear * "-" * date_month * "-" * date_mday
strftime_specifiers["g"] = strftime_specifiers["y"]
strftime_specifiers["G"] = date_fullyear
strftime_specifiers["h"] = date_mabbr
strftime_specifiers["H"] = time_hour
strftime_specifiers["I"] = l.Cg(l.P"0" * l.digit
                                + "1" * l.R"02", "hour")
strftime_specifiers["j"] = l.P"00" * l.R"19"
                                + "0" * l.digit * l.digit
                                + l.S"12" * l.digit * l.digit
                                + "3" * l.R"05" * l.digit
                                + "36" * l.R"06"
strftime_specifiers["k"] = l.Cg(l.S" 1" * l.digit
                                + "2" * l.R"03", "hour")
strftime_specifiers["l"] = l.Cg(l.space * l.digit
                                + "1" * l.R"02", "hour")
strftime_specifiers["m"] = date_month
strftime_specifiers["M"] = time_minute
strftime_specifiers["n"] = l.P"\n"
strftime_specifiers["p"] = l.Cg(l.P"AM" + "PM", "period")
strftime_specifiers["r"] = strftime_specifiers["I"] * ":" * time_minute * ":" * time_second * " " * strftime_specifiers["p"]
strftime_specifiers["R"] = time_hour * ":" * time_minute
strftime_specifiers["s"] = l.Cg(l.digit^1 / tonumber, "time_t")
strftime_specifiers["S"] = l.Cg(l.R"05" * l.digit
                           + "6" * l.S"01", "sec")
strftime_specifiers["t"] = l.P"\t"
strftime_specifiers["T"] = time_hour * ":" * time_minute * ":" * time_second
strftime_specifiers["u"] = l.R"17"
strftime_specifiers["U"] = l.R"04" * l.digit
                            + l.P"5" * l.R"03"
strftime_specifiers["V"] = strftime_specifiers["U"]
strftime_specifiers["w"] = l.R"06"
strftime_specifiers["W"] = strftime_specifiers["U"]
strftime_specifiers["x"] = strftime_specifiers["D"]
strftime_specifiers["X"] = strftime_specifiers["T"]
strftime_specifiers["Y"] = date_fullyear
strftime_specifiers["z"] = timezone_offset
strftime_specifiers["Z"] = timezone
strftime_specifiers["%"] = l.P"%"
if os.date("%c"):find("^%d") then -- windows
    strftime_specifiers["c"] = strftime_specifiers["D"] * " " * strftime_specifiers["T"]
    strftime_specifiers["z"] = timezone -- todo depending on the registry this could be the full name and not the abbreviation
else
    strftime_specifiers["c"] = strftime_specifiers["a"] * " " * date_mabbr * " " * date_mday_sp * " " * strftime_specifiers["T"] * " " * date_fullyear
end

local function strftime_lookup_grammar(var)
    local g = strftime_specifiers[var]
    if not g then
        error("unknown strftime specifier: " .. var)
    end
   return g
end

-- Returns an LPeg grammar based on the strftime format.
function build_strftime_grammar(format)
    local ws = l.space / function () return l.space end
    local variable = l.P"%" * ((l.alnum + "%") / strftime_lookup_grammar)
    local literal = (l.P(1) - (ws + variable))^1 / function (lit) return l.P(lit) end
    local item = ws + variable + literal

    local p = l.Ct(item * (item)^0)
    local t = p:match(format)
    if not t then
        error("could not parse the strftime format")
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


--[[ Common Log Format Grammars --]]
-- strftime format %d/%b/%Y:%H:%M:%S %z.
clf_timestamp = l.Ct(date_mday * "/" * date_mabbr * "/" * date_fullyear * ":" * strftime_specifiers["T"] * " " * timezone_offset)


--[[ RFC3164 Grammars --]]
local sp = l.P" "
-- since we consume multiple spaces this will also handle date-rfc3164-buggyday
rfc3164_timestamp = l.Ct(date_mabbr * sp^1 * l.Cg(l.digit^-2, "day") * sp^1 * strftime_specifiers["T"] * l.Cg(l.Cc("") / function() return os.date("%Y") end, "year"))


--[[ Database Grammars --]]
mysql_timestamp = l.Ct(date_fullyear * date_month * date_mday * time_hour * time_minute * time_second)

pgsql_timestamp = l.Ct(rfc3339_full_date * sp * strftime_specifiers["T"] )

return M
