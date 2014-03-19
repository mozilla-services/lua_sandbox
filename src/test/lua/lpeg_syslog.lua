-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local syslog = require("syslog")

function process(tc)
    if tc == 0 then
        -- TraditionalFileFormat
        local grammar = syslog.build_rsyslog_grammar('%TIMESTAMP% %HOSTNAME% %syslogtag%%msg:::sp-if-no-1st-sp%%msg:::drop-last-lf%\n')
        local log = 'Feb  9 14:17:01 trink-x230 CRON[20758]: (root) CMD (   cd / && run-parts --report /etc/cron.hourly)\n'
        local fields = grammar:match(log)
        fields.timestamp = fields.timestamp/1e6
        output(fields)
    elseif tc == 1 then
        -- FileFormat
        local grammar = syslog.build_rsyslog_grammar('%TIMESTAMP:::date-rfc3339% %HOSTNAME% %syslogtag%%msg:::sp-if-no-1st-sp%%msg:::drop-last-lf%\n')
        local log = '2014-02-10T08:21:59.111537-08:00 trink-x230 kernel: Kernel logging (proc) stopped.\n'
        local fields = grammar:match(log)
        fields.timestamp = fields.timestamp/1e6
        output(fields)
    elseif tc == 2 then
        -- ForwardFormat
        local grammar = syslog.build_rsyslog_grammar('<%PRI%>%TIMESTAMP:::date-rfc3339% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%')
        local log = '<6>2014-02-10T08:33:15.407935-08:00 trink-x230 kernel: imklog 5.8.6, log source = /proc/kmsg started.'
        local fields = grammar:match(log)
        fields.timestamp = fields.timestamp/1e6
        output(fields)
    elseif tc == 3 then
        -- TraditionalForwardFormat
        local grammar = syslog.build_rsyslog_grammar('<%PRI%>%TIMESTAMP% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%')
        local log = '<6>Feb 10 08:38:47 trink-x230 kernel: imklog 5.8.6, log source = /proc/kmsg started.'
        local fields = grammar:match(log)
        fields.timestamp = fields.timestamp/1e6
        output(fields)
    elseif tc == 4 then
        -- KitchenSink
        local grammar = syslog.build_rsyslog_grammar("%HOSTNAME% %SOURCE% %FROMHOST% %FROMHOST-IP% %SYSLOGTAG% %PROGRAMNAME% %PRI% %PRI-TEXT% %IUT% %SYSLOGFACILITY% %SYSLOGFACILITY-TEXT% %SYSLOGPRIORITY% %SYSLOGPRIORITY-TEXT% %TIMEGENERATED% %TIMEREPORTED:::date-mysql% %TIMESTAMP:::date-rfc3339% %PROTOCOL-VERSION% %STRUCTURED-DATA% %APP-NAME% %PROCID% %MSGID% %$NOW% %$YEAR% %$MONTH% %$DAY% %$HOUR% %$HHOUR% %$QHOUR% %$MINUTE% %MSG%\n")
        local log = 'trink-x230 trink-x230 trink-x230 127.0.0.1 kernel: kernel 6 kern.info<6> 1 0 kern 6 info Feb 10 09:20:53 20140210092053 2014-02-10T09:20:53.559934-08:00 0 - kernel  - imklog 2014-02-10 2014 02 10 09 00 01 20 imklog 5.8.6, log source = /proc/kmsg started.\n'
        local fields = grammar:match(log)
        fields.timestamp = fields.timestamp/1e6
        output(fields)
    end

    write()
    return 0
end
