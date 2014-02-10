-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "lpeg"
require "os"
require "string"

local rfc3339 = require("rfc3339")
local rfc5424 = require("rfc5424")
local clf = require "common_log_format"

function process(tc)
    if tc == 0 then
        output(lpeg.match(rfc3339.grammar, "1999-05-05T23:23:59.217-07:00"))
    elseif tc == 1 then
        output(string.format("%d %d %d %d %d %d %d %d %d %d %d",
                             lpeg.match(rfc5424.severity, "debug"),
                             lpeg.match(rfc5424.severity, "info"),
                             lpeg.match(rfc5424.severity, "notice"),
                             lpeg.match(rfc5424.severity, "warning"),
                             lpeg.match(rfc5424.severity, "warn"),
                             lpeg.match(rfc5424.severity, "error"),
                             lpeg.match(rfc5424.severity, "err"),
                             lpeg.match(rfc5424.severity, "crit"),
                             lpeg.match(rfc5424.severity, "alert"),
                             lpeg.match(rfc5424.severity, "emerg"),
                             lpeg.match(rfc5424.severity, "panic")
                          ))
    elseif tc == 2 then
        local t = lpeg.match(rfc3339.grammar, "1999-05-05T23:23:59.217-07:00")
        output(rfc3339.time_ns(t))
    elseif tc == 3 then
        output(lpeg.match(rfc3339.grammar, "1985-04-12T23:20:50.52Z"))
    elseif tc == 4 then
        output(lpeg.match(rfc3339.grammar, "1996-12-19T16:39:57-08:00"))
    elseif tc == 5 then
        output(lpeg.match(rfc3339.grammar, "1990-12-31T23:59:60Z"))
    elseif tc == 6 then
        output(lpeg.match(rfc3339.grammar, "1990-12-31T15:59:60-08:00"))
    elseif tc == 7 then
        output(lpeg.match(rfc3339.grammar, "1937-01-01T12:00:27.87+00:20"))
    elseif tc == 8 then
        -- common log format
        local grammar = clf.build_nginx_grammar('$remote_addr - $remote_user [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        if not grammar then return -1 end
        local log = '127.0.0.1 - - [10/Feb/2014:08:46:41 -0800] "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0"'
        local fields = grammar:match(log)
        output(fields)
    elseif tc == 9 then
        -- custom format to test the msec changes
        local grammar = clf.build_nginx_grammar('$msec $remote_addr "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        if not grammar then return -1 end
        local log = '1391794831.755 127.0.0.1 "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0" "-" 0.000'
        local fields = grammar:match(log)
        output(fields)
    elseif tc == 10 then
        -- TraditionalFileFormat
        local grammar = rfc5424.build_rsyslog_grammar('%TIMESTAMP% %HOSTNAME% %syslogtag%%msg:::sp-if-no-1st-sp%%msg:::drop-last-lf%\n')
        if not grammar then return -1 end
        local log = 'Feb  9 14:17:01 trink-x230 CRON[20758]: (root) CMD (   cd / && run-parts --report /etc/cron.hourly)\n'
        local fields = grammar:match(log)
        output(fields)
    elseif tc == 11 then
        -- FileFormat
        local grammar = rfc5424.build_rsyslog_grammar('%TIMESTAMP:::date-rfc3339% %HOSTNAME% %syslogtag%%msg:::sp-if-no-1st-sp%%msg:::drop-last-lf%\n')
        if not grammar then return -1 end
        local log = '2014-02-10T08:21:59.111537-08:00 trink-x230 kernel: Kernel logging (proc) stopped.\n'
        local fields = grammar:match(log)
        output(fields)
    elseif tc == 12 then
        -- ForwardFormat
        local grammar = rfc5424.build_rsyslog_grammar('<%PRI%>%TIMESTAMP:::date-rfc3339% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%')
        if not grammar then return -1 end
        local log = '<6>2014-02-10T08:33:15.407935-08:00 trink-x230 kernel: imklog 5.8.6, log source = /proc/kmsg started.'
        local fields = grammar:match(log)
        output(fields)
    elseif tc == 13 then
        -- TraditionalForwardFormat
        local grammar = rfc5424.build_rsyslog_grammar('<%PRI%>%TIMESTAMP% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%')
        if not grammar then return -1 end
        local log = '<6>Feb 10 08:38:47 trink-x230 kernel: imklog 5.8.6, log source = /proc/kmsg started.'
        local fields = grammar:match(log)
        output(fields)
    elseif tc == 14 then
        -- KitchenSink
        local grammar = rfc5424.build_rsyslog_grammar("%HOSTNAME% %SOURCE% %FROMHOST% %FROMHOST-IP% %SYSLOGTAG% %PROGRAMNAME% %PRI% %PRI-TEXT% %IUT% %SYSLOGFACILITY% %SYSLOGFACILITY-TEXT% %SYSLOGPRIORITY% %SYSLOGPRIORITY-TEXT% %TIMEGENERATED% %TIMEREPORTED:::date-mysql% %TIMESTAMP:::date-rfc3339% %PROTOCOL-VERSION% %STRUCTURED-DATA% %APP-NAME% %PROCID% %MSGID% %$NOW% %$YEAR% %$MONTH% %$DAY% %$HOUR% %$HHOUR% %$QHOUR% %$MINUTE% %MSG%\n")
        if not grammar then return -1 end
        local log = 'trink-x230 trink-x230 trink-x230 127.0.0.1 kernel: kernel 6 kern.info<6> 1 0 kern 6 info Feb 10 09:20:53 20140210092053 2014-02-10T09:20:53.559934-08:00 0 - kernel  - imklog 2014-02-10 2014 02 10 09 00 01 20 imklog 5.8.6, log source = /proc/kmsg started.\n'
        local fields = grammar:match(log)
        output(fields)
    end

    write()
    return 0
end
