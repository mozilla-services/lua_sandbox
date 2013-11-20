-- sample input
---------------
-- debug

-- output
---------------
-- 7 (string)


local rfc5424_severity =
lpeg.Cs(lpeg.P"debug"               /"7")
+ lpeg.Cs(lpeg.P"info"              /"6")
+ lpeg.Cs(lpeg.P"notice"            /"5")
+ lpeg.Cs((lpeg.P"warning" + "warn")/"4")
+ lpeg.Cs((lpeg.P"error" + "err")   /"3")
+ lpeg.Cs(lpeg.P"crit"              /"2")
+ lpeg.Cs(lpeg.P"alert"             /"1")
+ lpeg.Cs((lpeg.P"emerg" + "panic") /"0")

return rfc5424_severity
