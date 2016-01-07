-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local syslog = require("syslog")

local function traditional_file_format()
    local grammar = syslog.build_rsyslog_grammar('%TIMESTAMP% %HOSTNAME% %syslogtag%%msg:::sp-if-no-1st-sp%%msg:::drop-last-lf%\n')
    local log = 'Feb  9 14:17:01 trink-x230 CRON[20758]: (root) CMD (   cd / && run-parts --report /etc/cron.hourly)\n'
    local fields = grammar:match(log)
    assert(fields.msg == "(root) CMD (   cd / && run-parts --report /etc/cron.hourly)", fields.msg)
    local ts = os.time{year = os.date("%Y"), month = 2, day = 9, hour = 14, min = 17, sec = 01} * 1e9
    assert(fields.timestamp == ts, fields.timestamp)
    assert(fields.syslogtag.pid == 20758, fields.syslogtag.pid)
    assert(fields.syslogtag.programname == "CRON", fields.syslogtag.programname)
    assert(fields.hostname == "trink-x230", fields.hostname)
end

local function file_format()
    local grammar = syslog.build_rsyslog_grammar('%TIMESTAMP:::date-rfc3339% %HOSTNAME% %syslogtag%%msg:::sp-if-no-1st-sp%%msg:::drop-last-lf%\n')
    local log = '2014-02-10T08:21:59.111537-08:00 trink-x230 kernel: Kernel logging (proc) stopped.\n'
    local fields = grammar:match(log)
    assert(fields.msg == "Kernel logging (proc) stopped.", fields.msg)
    assert(fields.timestamp == 1392049319111537000, fields.timestamp)
    assert(fields.syslogtag.programname == "kernel", fields.syslogtag.programname)
    assert(fields.hostname == "trink-x230", fields.hostname)
end

local function forward_format()
    local grammar = syslog.build_rsyslog_grammar('<%PRI%>%TIMESTAMP:::date-rfc3339% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%')
    local log = '<6>2014-02-10T08:33:15.407935-08:00 trink-x230 kernel: imklog 5.8.6, log source = /proc/kmsg started.'
    local fields = grammar:match(log)
    assert(fields.msg == "imklog 5.8.6, log source = /proc/kmsg started.", fields.msg)
    assert(fields.timestamp == 1392049995407935000, fields.timestamp)
    assert(fields.syslogtag.programname == "kernel", fields.syslogtag.programname)
    assert(fields.pri.severity == 6, fields.pri.severity)
    assert(fields.pri.facility == 0, fields.pri.facility)
    assert(fields.hostname == "trink-x230", fields.hostname)
end

local function traditional_forward_format()
    local grammar = syslog.build_rsyslog_grammar('<%PRI%>%TIMESTAMP% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%')
    local log = '<6>Feb 10 08:38:47 trink-x230 kernel: imklog 5.8.6, log source = /proc/kmsg started.'
    local fields = grammar:match(log)
    assert(fields.msg == "imklog 5.8.6, log source = /proc/kmsg started.", fields.msg)
    local ts = os.time{year = os.date("%Y"), month = 2, day = 10, hour = 8, min = 38, sec = 47} * 1e9
    assert(fields.timestamp == ts, fields.timestamp)
    assert(fields.syslogtag.programname == "kernel", fields.syslogtag.programname)
    assert(fields.pri.severity == 6, fields.pri.severity)
    assert(fields.pri.facility == 0, fields.pri.facility)
    assert(fields.hostname == "trink-x230", fields.hostname)
end

local function kitchen_sink()
    local grammar = syslog.build_rsyslog_grammar("%HOSTNAME% %SOURCE% %FROMHOST% %FROMHOST-IP% %SYSLOGTAG% %PROGRAMNAME% %PRI% %PRI-TEXT% %IUT% %SYSLOGFACILITY% %SYSLOGFACILITY-TEXT% %SYSLOGPRIORITY% %SYSLOGPRIORITY-TEXT% %TIMEGENERATED% %TIMEREPORTED:::date-mysql% %TIMESTAMP:::date-rfc3339% %PROTOCOL-VERSION% %STRUCTURED-DATA% %APP-NAME% %PROCID% %MSGID% %$NOW% %$YEAR% %$MONTH% %$DAY% %$HOUR% %$HHOUR% %$QHOUR% %$MINUTE% %MSG%\n")
    local log = 'trink-x230 trink-x230 trink-x230 127.0.0.1 kernel: kernel 6 kern.info<6> 1 0 kern 6 info Feb 10 09:20:53 20140210092053 2014-02-10T09:20:53.559934-08:00 0 - kernel  - 2014-02-10 2014 02 10 09 00 01 20 imklog 5.8.6, log source = /proc/kmsg started.\n'
    local fields = grammar:match(log)
    assert(fields.hostname == "trink-x230", fields.hostname)
    assert(fields.source == "trink-x230", fields.source)
    assert(fields.fromhost == "trink-x230", fields.fromhost)
    assert(fields["fromhost-ip"] == "127.0.0.1", fields["fromhost-ip"])
    assert(fields.syslogtag.programname == "kernel", fields.syslogtag.programname)
    assert(fields.programname == "kernel", fields.programname)
    assert(fields.pri.severity == 6, fields.pri.severity)
    assert(fields.pri.facility == 0, fields.pri.facility)
    assert(fields["pri-text"] == 0, fields["pri-text"])
    assert(fields.iut == "1", fields.iut)
    assert(fields.syslogfacility == 0, fields.syslogfacility)
    assert(fields["syslogfacility-text"] == 0, fields["syslogfacility-text"])
    assert(fields.syslogpriority == 6, fields.syslogpriority)
    assert(fields["syslogpriority-text"] == 6, fields["syslogpriority-text"])
    local ts = os.time{year = os.date("%Y"), month = 2, day = 10, hour = 9, min = 20, sec = 53} * 1e9
    assert(fields.timegenerated == ts, fields.timegenerated)
    -- time reported is mapped to timestamp
    assert(fields.timestamp == 1392052853559934000, fields.timestamp)
    assert(fields["protocol-version"] == "0", fields["protocol-version"])
    assert(type(fields["structured-data"]) == "table", type(fields["structured-data"]))
    assert(fields["app-name"] == "kernel", fields["app-name"])
    assert(fields.procid == "", fields.procid)
    assert(fields.msgid == "-", fields.msgid)
    assert(fields["$now"] == "2014-02-10", fields["$now"])
    assert(fields["$year"] == "2014")
    assert(fields["$month"] == "02")
    assert(fields["$day"] == "10")
    assert(fields["$hour"] == "09")
    assert(fields["$hhour"] == "00")
    assert(fields["$qhour"] == "01")
    assert(fields["$minute"] == "20")
    assert(fields.msg == "imklog 5.8.6, log source = /proc/kmsg started.", fields.msg)
end

local function no_colon_in_tag()
    local grammar = syslog.build_rsyslog_grammar('<%PRI%>%TIMESTAMP:::date-rfc3339% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%')
    local log = '<166>2014-06-26T23:13:23-07:00 example.com     at java.net.SocketInputStream.socketRead0(Native Method)'
    local fields = grammar:match(log)
    assert(fields.msg == "at java.net.SocketInputStream.socketRead0(Native Method)", fields.msg)
    assert(fields.timestamp == 1403849603000000000, fields.timestamp)
    assert(fields.syslogtag.programname == "", fields.syslogtag.programname)
    assert(fields.pri.severity == 6, fields.pri.severity)
    assert(fields.pri.facility == 20, fields.pri.facility)
    assert(fields.hostname == "example.com", fields.hostname)
end

local function structured_data()
    local grammar = syslog.build_rsyslog_grammar("%STRUCTURED-DATA%")
    local log = '[origin@123 name1="first value" name2="sec\\ond\\\\v[a\\]l\\"ue"]'
    local fields = grammar:match(log)
    local sd = fields["structured-data"]
    assert(sd["_name1"] == "first value", sd["_name1"])
    assert(sd["_name2"] == "sec\\ond\\v[a]l\"ue", sd["_name2"])
    assert(sd["id"] == "origin@123", sd["id"])

    log = '[origin@123 name="value\\\\"]'
    fields = grammar:match(log)
    sd = fields["structured-data"]
    assert(sd["_name"] == "value\\", sd["_name"])
    assert(sd["id"] == "origin@123", sd["id"])

    log = '[origin@123 name="value\\\""]'
    fields = grammar:match(log)
    sd = fields["structured-data"]
    assert(sd["_name"] == 'value"', sd["_name"])
    assert(sd["id"] == "origin@123", sd["id"])

    log = '[origin@123 name=""]'
    fields = grammar:match(log)
    sd = fields["structured-data"]
    assert(sd["_name"] == "", sd["_name"])
    assert(sd["id"] == "origin@123", sd["id"])
end

function process()
    traditional_file_format()
    file_format()
    forward_format()
    traditional_forward_format()
    kitchen_sink()
    no_colon_in_tag()
    structured_data()

    return 0
end

