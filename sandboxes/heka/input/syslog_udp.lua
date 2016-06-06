-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Syslog UDP Input

## Sample Configuration
```lua
filename            = "syslog_udp.lua"
instruction_limit   = 0

-- address (string) - IP address or * for all interfaces
-- address = "127.0.0.1"

-- port (integer) - IP port to listen on
-- port = 514

-- template (string) - The 'template' configuration string from rsyslog.conf
-- see http://rsyslog-5-8-6-doc.neocities.org/rsyslog_conf_templates.html
-- template = "<%PRI%>%TIMESTAMP% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%" -- RSYSLOG_TraditionalForwardFormat
```
--]]

local syslog = require "lpeg.syslog"
local socket = require "socket"

local address       = read_config("address") or "127.0.0.1"
local port          = read_config("port") or 514
local hostname_keep = read_config("hostname_keep")
local template      = read_config("template") or "<%PRI%>%TIMESTAMP% %HOSTNAME% %syslogtag:1:32%%msg:::sp-if-no-1st-sp%%msg%"
local grammar       = syslog.build_rsyslog_grammar(template)

local msg = {
Timestamp   = nil,
Type        = read_config("type"),
Hostname    = nil,
Payload     = nil,
Pid         = nil,
Severity    = nil,
Fields      = nil
}

local err_msg = {
    Logger  = read_config("Logger"),
    Type    = "error",
    Payload = nil,
}

local server = assert(socket.udp())
assert(server:setsockname(address, port))
server:settimeout(1)

function process_message()
    while true do
        local data, ip, port = server:receivefrom()
        if data then
            local fields = grammar:match(data)
            if fields then
                if fields.pri then
                    msg.Severity = fields.pri.severity
                    fields.syslogfacility = fields.pri.facility
                    fields.pri = nil
                else
                    msg.Severity = fields.syslogseverity or fields["syslogseverity-text"]
                    or fields.syslogpriority or fields["syslogpriority-text"]

                    fields.syslogseverity = nil
                    fields["syslogseverity-text"] = nil
                    fields.syslogpriority = nil
                    fields["syslogpriority-text"] = nil
                end

                if fields.syslogtag then
                    fields.programname = fields.syslogtag.programname
                    msg.Pid = fields.syslogtag.pid
                    fields.syslogtag = nil
                end

                msg.Hostname = fields.hostname or fields.source
                fields.hostname = nil
                fields.source = nil

                msg.Payload = fields.msg
                fields.msg = nil

                fields.sender_ip = ip
                fields.sender_port = {value = port, value_type = 2}

                msg.Fields = fields
                pcall(inject_message, msg)
            end
        elseif ip ~= "timeout" then
            err_msg.Payload = ip
            pcall(inject_message, err_msg)
        end
    end
    return 0
end
