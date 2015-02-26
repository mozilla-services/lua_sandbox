-- imports

local l = require "lpeg"
local dt = require "date_time"

l.locale(l)

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local space  = l.space^1
local sep    = l.P"\n"
local start  = (l.P(1) - "[")^1
local level  = (l.P":" - "]")^1


--[[
2015-02-12T07:37:50.43210 [Thu Feb 12 07:37:50.432070 2015] [:error] [pid 27272:tid 140350035179264] [client 199.16.43.148] ModSecurity: Warning. Pattern match "\\\\W{4,}" at ARGS:data. [file "/etc/apache2/modsecurity-crs/activated_rules/modsecurity_crs_40_generic_attacks.conf"] [line "37"] [id "960024"] [rev "2"] [msg "Meta-Character Anomaly Detection Alert - Repetative Non-Word Characters"] [data "Matched Data: \\x22:[],\\x22 found within ARGS:data: {\\x22offset\\x22:0,\\x22payments\\x22:[],\\x22transactions\\x22:[{\\x22uuid\\x22:\\x22e9a3b02c-c3eb-3133-a16f-9136fe01b202\\x22,\\x22user_id\\x22:\\x22c858ba0a-e97d-44a6-8d94-c088858b5d4f\\x22,\\x22amounts\\x22:{\\x22amount\\x22:1650000,\\x22cleared\\x22:1650000,\\x22fees\\x22:0,\\x22cashback\\x22:0,\\x22base\\x22:1650000},\\x22times\\x22:{\\x22when_recorded\\x22:1423518569000,\\x22when_recorded_local\\x22:\\x222015-02-09 21:49:29.000\\x22,\\x22when_received\\x22:1423616789835,\\x22last_mo..."] [ver "OWASP_CRS/2.2.9"] [maturity "9"] [accuracy "8"] [hostname "bank.simple.com"] [uri "/transactions/export"] [unique_id "VNxYTgobA9wAAGqIu8YAAABm"]
--]]

-- Thu Feb 12 07:37:50.432070 2015
local modsec_time = time_hour * ":" * time_minute * ":" * time_second * time_secfrac
local modsec_timestamp = l.Ct(date_abbr * space * date_mabbr * date_mday * space * modsec_time * space * date_fullyear) / dt.time_to_ns

local modsec_logline = start                             -- Ignore the syslog-ng timestamp
                     * "[" * modsec_timestamp * "]"      -- [Thu Feb 12 07:37:50.432070 2015]
                     * space
                     * "[" * level * "]"                 -- [:error]
