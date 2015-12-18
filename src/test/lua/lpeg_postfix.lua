-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require 'string'
require 'table'
local pf = require 'postfix'

-- From https://github.com/whyscream/postfix-grok-patterns
-- 2015-05-26 8fdd562d159845b97bf8d9c273e5357f3a143a94
-- Original under BSD-3-clause license
local tests = {
  ["anvil_0001.yaml"] = {
    "POSTFIX_ANVIL",
    "statistics: max cache size 1 at Oct 26 18:13:50",
    {
      postfix_anvil_cache_size = 1,
      postfix_anvil_timestamp = "Oct 26 18:13:50"
    }
  },
  ["anvil_0002.yaml"] = {
    "POSTFIX_ANVIL",
    "statistics: max connection count 1 for (smtpd:61.238.241.86) at Oct 26 18:13:50",
    {
      postfix_anvil_conn_count = 1,
      postfix_anvil_timestamp = "Oct 26 18:13:50",
      postfix_client_ip = "61.238.241.86",
      postfix_service = "smtpd"
    }
  },
  ["anvil_0003.yaml"] = {
    "POSTFIX_ANVIL",
    "statistics: max connection rate 1/60s for (smtpd:61.238.241.86) at Oct 26 18:13:50",
    {
      postfix_anvil_conn_period = "60s",
      postfix_anvil_conn_rate = 1,
      postfix_anvil_timestamp = "Oct 26 18:13:50",
      postfix_client_ip = "61.238.241.86",
      postfix_service = "smtpd"
    }
  },
  ["anvil_0004.yaml"] = {
    "POSTFIX_ANVIL",
    "statistics: max connection count 1 for (smtpd:2604:8d00:0:1::3) at Oct 26 17:46:59",
    {
      postfix_anvil_conn_count = 1,
      postfix_anvil_timestamp = "Oct 26 17:46:59",
      postfix_client_ip = "2604:8d00:0:1::3",
      postfix_service = "smtpd"
    }
  },
  ["anvil_0005.yaml"] = {
    "POSTFIX_ANVIL",
    "statistics: max connection rate 1/60s for (smtpd:2604:8d00:0:1::3) at Oct 26 17:46:59",
    {
      postfix_anvil_conn_period = "60s",
      postfix_anvil_conn_rate = 1,
      postfix_anvil_timestamp = "Oct 26 17:46:59",
      postfix_client_ip = "2604:8d00:0:1::3",
      postfix_service = "smtpd"
    }
  },
  ["anvil_0006.yaml"] = {
    "POSTFIX_ANVIL",
    "statistics: max connection count 1 for (127.0.0.1:2525:127.0.0.1) at Oct 26 19:46:00",
    {
      postfix_anvil_conn_count = 1,
      postfix_anvil_timestamp = "Oct 26 19:46:00",
      postfix_client_ip = "127.0.0.1",
      postfix_service = "127.0.0.1:2525"
    }
  },
  ["anvil_0007.yaml"] = {
    "POSTFIX_ANVIL",
    "statistics: max connection rate 1/60s for (127.0.0.1:2525:127.0.0.1) at Oct 26 18:13:50",
    {
      postfix_anvil_conn_period = "60s",
      postfix_anvil_conn_rate = 1,
      postfix_anvil_timestamp = "Oct 26 18:13:50",
      postfix_client_ip = "127.0.0.1",
      postfix_service = "127.0.0.1:2525"
    }
  },
  ["bounce_0001.yaml"] = {
    "POSTFIX_BOUNCE",
    "17CCA8044: sender non-delivery notification: ADD2F8052",
    {
      postfix_bounce_queueid = "ADD2F8052",
      postfix_queueid = "17CCA8044"
    }
  },
  ["bounce_0002.yaml"] = {
    "POSTFIX_BOUNCE",
    "65AFB8B5: sender delivery status notification: 1DF35917",
    {
      postfix_bounce_queueid = "1DF35917",
      postfix_queueid = "65AFB8B5"
    }
  },
  ["bounce_0003.yaml"] = {
    "POSTFIX_BOUNCE",
    "264FE1A18: sender delay notification: 0A87A1A08",
    {
      postfix_bounce_queueid = "0A87A1A08",
      postfix_queueid = "264FE1A18"
    }
  },
  ["cleanup_0001.yaml"] = {
    "POSTFIX_CLEANUP",
    "0F5383D: message-id=<544D2523.30609@rhsoft.net>",
    {
      postfix_keyvalue_data = "message-id=<544D2523.30609@rhsoft.net>",
      postfix_queueid = "0F5383D"
    }
  },
  ["cleanup_0002.yaml"] = {
    "POSTFIX_CLEANUP",
    "4104E3D: message-id=<>",
    {
      postfix_keyvalue_data = "message-id=<>",
      postfix_queueid = "4104E3D"
    }
  },
  ["cleanup_0003.yaml"] = {
    "POSTFIX_CLEANUP",
    "4104E3D: milter-header-redirect: header X-Spam-Status: Yes, score=39.3 required=5.0 tests=ADVANCE_FE from 061238241086.static.ctinets.com[61.238.241.86]; from=<wumt5@cchfdc.com> to=<voorzitter@example.com> proto=ESMTP helo=<ecsolved.com>: tom@example.com",
    {
      postfix_keyvalue_data = "from=<wumt5@cchfdc.com> to=<voorzitter@example.com> proto=ESMTP helo=<ecsolved.com>",
      postfix_milter_data = "tom@example.com",
      postfix_milter_message = "header X-Spam-Status: Yes, score=39.3 required=5.0 tests=ADVANCE_FE from 061238241086.static.ctinets.com[61.238.241.86]",
      postfix_milter_result = "header-redirect",
      postfix_queueid = "4104E3D"
    }
  },
  ["cleanup_0004.yaml"] = {
    "POSTFIX_CLEANUP",
    "4104E3D: milter-reject: END-OF-MESSAGE from 061238241086.static.ctinets.com[61.238.241.86]: 5.7.1 Blocked by SpamAssassin; from=<wumt5@cchfdc.com> to=<voorzitter@example.com> proto=ESMTP helo=<ecsolved.com>",
    {
      postfix_keyvalue_data = "from=<wumt5@cchfdc.com> to=<voorzitter@example.com> proto=ESMTP helo=<ecsolved.com>",
      postfix_milter_message = "END-OF-MESSAGE from 061238241086.static.ctinets.com[61.238.241.86]: 5.7.1 Blocked by SpamAssassin",
      postfix_milter_result = "reject",
      postfix_queueid = "4104E3D"
    }
  },
  ["cleanup_0005.yaml"] = {
    "POSTFIX_CLEANUP",
    "warning: dict_ldap_get_values[1]: DN uid=mguiraud,ou=people,dc=neotion,dc=com not found, skipping ",
    {
      postfix_warning = "dict_ldap_get_values[1]: DN uid=mguiraud,ou=people,dc=neotion,dc=com not found, skipping "
    }
  },
  ["delays_0001.yaml"] = {
    "POSTFIX_DELAYS",
    "0.2/0.02/0.04/3.3",
    {
      postfix_delay_before_qmgr = 0.20000000000000001,
      postfix_delay_conn_setup = 0.040000000000000001,
      postfix_delay_in_qmgr = 0.02,
      postfix_delay_transmission = 3.2999999999999998
    }
  },
  ["discard_0001.yaml"] = {
    "POSTFIX_DISCARD",
    "25F2E5E061: to=<george.desantis@test.example.com>, relay=none, delay=0.05, delays=0.05/0/0/0, dsn=2.0.0, status=sent (test.example.com)",
    {
      postfix_keyvalue_data = "to=<george.desantis@test.example.com>, relay=none, delay=0.05, delays=0.05/0/0/0, dsn=2.0.0,",
      postfix_queueid = "25F2E5E061",
      postfix_status = "sent"
    }
  },
  ["dnsblog_0001.yaml"] = {
    "POSTFIX_DNSBLOG",
    "addr 61.238.241.86 listed by domain psbl.surriel.com as 127.0.0.2",
    {
      postfix_client_ip = "61.238.241.86",
      postfix_dnsbl_domain = "psbl.surriel.com",
      postfix_dnsbl_result = "127.0.0.2"
    }
  },
  ["dnsblog_0002.yaml"] = {
    "POSTFIX_DNSBLOG",
    "addr 2607:f8b0:4003:c01::23a listed by domain list.dnswl.org as 127.0.5.1",
    {
      postfix_client_ip = "2607:f8b0:4003:c01::23a",
      postfix_dnsbl_domain = "list.dnswl.org",
      postfix_dnsbl_result = "127.0.5.1"
    }
  },
  ["local_0001.yaml"] = {
    "POSTFIX_LOCAL",
    "2A22C263F6: to=user@hostname.example.com, orig_to=root@localhost, relay=local, delay=0.07, delays=0.04/0/0/0.03, dsn=2.0.0, status=sent (delivered to command: procmail -a \"$EXTENSION\")",
    {
      postfix_keyvalue_data = "to=user@hostname.example.com, orig_to=root@localhost, relay=local, delay=0.07, delays=0.04/0/0/0.03, dsn=2.0.0, status=sent (delivered to command: procmail -a \"$EXTENSION\")",
      postfix_queueid = "2A22C263F6"
    }
  },
  ["local_0002.yaml"] = {
    "POSTFIX_LOCAL",
    "892A0205B6: to=ghdsgfhdslfh@localhost, relay=local, delay=0.05, delays=0.02/0/0/0.02, dsn=5.1.1, status=bounced (unknown user: \"ghdsgfhdslfh\")",
    {
      postfix_keyvalue_data = "to=ghdsgfhdslfh@localhost, relay=local, delay=0.05, delays=0.02/0/0/0.02, dsn=5.1.1, status=bounced (unknown user: \"ghdsgfhdslfh\")",
      postfix_queueid = "892A0205B6"
    }
  },
  ["master_0001.yaml"] = {
    "POSTFIX_MASTER",
    "daemon started -- version 2.11.0, configuration /etc/postfix",
    {
      postfix_config_path = "/etc/postfix",
      postfix_version = "2.11.0"
    }
  },
  ["master_0002.yaml"] = {
    "POSTFIX_MASTER",
    "terminating on signal 15",
    {
      postfix_termination_signal = 15
    }
  },
  ["master_0003.yaml"] = {
    "POSTFIX_MASTER",
    "reload -- version 2.11.0, configuration /etc/postfix",
    {
      postfix_config_path = "/etc/postfix",
      postfix_version = "2.11.0"
    }
  },
  ["pickup_0001.yaml"] = {
    "POSTFIX_PICKUP",
    "D2FAE20586: uid=0 from=<fail2ban>",
    {
      postfix_queueid = "D2FAE20586",
      -- from postfix_keyvalue_data = "uid=0 from=<fail2ban>",
      postfix_uid = 0,
      postfix_from = 'fail2ban'
    }
  },
  ["pipe_0001.yaml"] = {
    "POSTFIX_PIPE",
    "0F5383D: to=<tom@example.com>, relay=dovecot, delay=4.3, delays=4.1/0.01/0/0.15, dsn=2.0.0, status=sent (delivered via dovecot service)",
    {
      postfix_keyvalue_data = "to=<tom@example.com>, relay=dovecot, delay=4.3, delays=4.1/0.01/0/0.15, dsn=2.0.0, status=sent",
      postfix_pipe_service = "dovecot",
      postfix_queueid = "0F5383D"
    }
  },
  ["pipe_0002.yaml"] = {
    "POSTFIX_PIPE",
    "153053D: to=<tom@example.com>, orig_to=<admin@example.com>, relay=dovecot, delay=3.4, delays=3.3/0.03/0/0.12, dsn=2.0.0, status=sent (delivered via dovecot service)",
    {
      postfix_keyvalue_data = "to=<tom@example.com>, orig_to=<admin@example.com>, relay=dovecot, delay=3.4, delays=3.3/0.03/0/0.12, dsn=2.0.0, status=sent",
      postfix_pipe_service = "dovecot",
      postfix_queueid = "153053D"
    }
  },
  ["pipe_0003.yaml"] = {
    "POSTFIX_PIPE",
    "95ECE24E0: to=<tom@example.com>, relay=dovecot, delay=0.12, delays=0.03/0/0/0.08, dsn=5.4.6, status=bounced (mail forwarding loop for tom@example.com)",
    {
      postfix_keyvalue_data = "to=<tom@example.com>, relay=dovecot, delay=0.12, delays=0.03/0/0/0.08, dsn=5.4.6, status=bounced",
      postfix_queueid = "95ECE24E0",
      postfix_to = "tom@example.com"
    }
  },
  ["postdrop_0001.yaml"] = {
    "POSTFIX_POSTDROP",
    "warning: uid=0: File too large",
    {
      postfix_warning = "uid=0: File too large"
    }
  },
  ["postscreen_0001.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "cache btree:/var/lib/postfix-in/postscreen_cache full cleanup: retained=242 dropped=7 entries",
    {
      postfix_postscreen_cache_dropped = 7,
      postfix_postscreen_cache_retained = 242
    }
  },
  ["postscreen_0002.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "CONNECT from [61.238.241.86]:53024 to [89.105.204.244]:25",
    {
      postfix_client_ip = "61.238.241.86",
      postfix_client_port = 53024,
      postfix_server_ip = "89.105.204.244",
      postfix_server_port = 25
    }
  },
  ["postscreen_0003.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "DISCONNECT [216.81.72.72]:43421",
    {
      postfix_client_ip = "216.81.72.72",
      postfix_client_port = 43421
    }
  },
  ["postscreen_0004.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "PASS OLD [61.238.241.86]:53024",
    {
      postfix_client_ip = "61.238.241.86",
      postfix_client_port = 53024,
      postfix_postscreen_access = "PASS OLD"
    }
  },
  ["postscreen_0005.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "PASS OLD [2604:8d00:0:1::3]:52237",
    {
      postfix_client_ip = "2604:8d00:0:1::3",
      postfix_client_port = 52237,
      postfix_postscreen_access = "PASS OLD"
    }
  },
  ["postscreen_0006.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "PASS NEW [2607:f8b0:4003:c01::23a]:36051",
    {
      postfix_client_ip = "2607:f8b0:4003:c01::23a",
      postfix_client_port = 36051,
      postfix_postscreen_access = "PASS NEW"
    }
  },
  ["postscreen_0007.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "WHITELISTED [61.238.241.86]:53024",
    {
      postfix_client_ip = "61.238.241.86",
      postfix_client_port = 53024,
      postfix_postscreen_access = "WHITELISTED"
    }
  },
  ["postscreen_0008.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "BLACKLISTED [61.238.241.86]:53024",
    {
      postfix_client_ip = "61.238.241.86",
      postfix_client_port = 53024,
      postfix_postscreen_access = "BLACKLISTED"
    }
  },
  ["postscreen_0009.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "WHITELIST VETO [61.238.241.86]:53024",
    {
      postfix_client_ip = "61.238.241.86",
      postfix_client_port = 53024,
      postfix_postscreen_access = "WHITELIST VETO"
    }
  },
  ["postscreen_0010.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "PREGREET 6 after 1.2 from [216.81.72.72]:43421: RSET\r\n",
    {
      postfix_client_ip = "216.81.72.72",
      postfix_client_port = 43421,
      postfix_postscreen_violation = "PREGREET",
      postfix_postscreen_violation_time = 1.2
    }
  },
  ["postscreen_0011.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "DNSBL rank 3 for [111.73.45.149]:1038",
    {
      postfix_client_ip = "111.73.45.149",
      postfix_client_port = 1038,
      postfix_postscreen_dnsbl_rank = 3,
      postfix_postscreen_violation = "DNSBL"
    }
  },
  ["postscreen_0012.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "HANGUP after 63 from [111.73.45.149]:1038 in tests after SMTP handshake",
    {
      postfix_client_ip = "111.73.45.149",
      postfix_client_port = 1038,
      postfix_postscreen_violation = "HANGUP",
      postfix_postscreen_violation_time = 63
    }
  },
  ["postscreen_0013.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "HANGUP after 0 from [66.55.85.58]:54017 in tests before SMTP handshake",
    {
      postfix_client_ip = "66.55.85.58",
      postfix_client_port = 54017,
      postfix_postscreen_violation = "HANGUP",
      postfix_postscreen_violation_time = 0
    }
  },
  ["postscreen_0014.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND TIME LIMIT from [182.246.250.243]:51799 after CONNECT",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51799,
      postfix_postscreen_violation = "COMMAND TIME LIMIT",
      postfix_smtp_stage = "CONNECT"
    }
  },
  ["postscreen_0015.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND TIME LIMIT from [182.246.250.243]:51799 after HELO",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51799,
      postfix_postscreen_violation = "COMMAND TIME LIMIT",
      postfix_smtp_stage = "HELO"
    }
  },
  ["postscreen_0016.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND TIME LIMIT from [182.246.250.243]:51799 after EHLO",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51799,
      postfix_postscreen_violation = "COMMAND TIME LIMIT",
      postfix_smtp_stage = "EHLO"
    }
  },
  ["postscreen_0017.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND TIME LIMIT from [182.246.250.243]:51799 after AUTH",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51799,
      postfix_postscreen_violation = "COMMAND TIME LIMIT",
      postfix_smtp_stage = "AUTH"
    }
  },
  ["postscreen_0018.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND TIME LIMIT from [182.246.250.243]:51799 after MAIL",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51799,
      postfix_postscreen_violation = "COMMAND TIME LIMIT",
      postfix_smtp_stage = "MAIL"
    }
  },
  ["postscreen_0019.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND TIME LIMIT from [182.246.250.243]:51796 after RCPT",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51796,
      postfix_postscreen_violation = "COMMAND TIME LIMIT",
      postfix_smtp_stage = "RCPT"
    }
  },
  ["postscreen_0020.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND COUNT LIMIT from [182.246.250.243]:51796 after RCPT",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51796,
      postfix_postscreen_violation = "COMMAND COUNT LIMIT",
      postfix_smtp_stage = "RCPT"
    }
  },
  ["postscreen_0021.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND LENGTH LIMIT from [182.246.250.243]:51796 after RCPT",
    {
      postfix_client_ip = "182.246.250.243",
      postfix_client_port = 51796,
      postfix_postscreen_violation = "COMMAND LENGTH LIMIT",
      postfix_smtp_stage = "RCPT"
    }
  },
  ["postscreen_0022.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "NON-SMTP COMMAND from [93.174.93.51]:45284 after CONNECT: GET http://ipv4scan.com/hello/check.txt HTTP/1.1",
    {
      postfix_client_ip = "93.174.93.51",
      postfix_client_port = 45284,
      postfix_postscreen_violation = "NON-SMTP COMMAND",
      postfix_smtp_stage = "CONNECT"
    }
  },
  ["postscreen_0023.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "NOQUEUE: reject: CONNECT from [1.2.3.4]:1337: too many connections",
    {
      postfix_client_ip = "1.2.3.4",
      postfix_client_port = 1337,
      postfix_postscreen_toobusy_data = "too many connections"
    }
  },
  ["postscreen_0024.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "NOQUEUE: reject: CONNECT from [5.6.7.8]:1337: all server ports busy",
    {
      postfix_client_ip = "5.6.7.8",
      postfix_client_port = 1337,
      postfix_postscreen_toobusy_data = "all server ports busy"
    }
  },
  ["postscreen_0025.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "COMMAND PIPELINING from [88.208.233.4]:42606 after .: \n",
    {
      postfix_client_ip = "88.208.233.4",
      postfix_client_port = 42606,
      postfix_postscreen_violation = "COMMAND PIPELINING",
      postfix_smtp_stage = "."
    }
  },
  ["postscreen_0026.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "BARE NEWLINE from [88.208.233.4]:42606 after .",
    {
      postfix_client_ip = "88.208.233.4",
      postfix_client_port = 42606,
      postfix_postscreen_violation = "BARE NEWLINE",
      postfix_smtp_stage = "."
    }
  },
  ["postscreen_0027.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "NOQUEUE: reject: RCPT from [182.98.255.184]:2413: 550 5.5.1 Protocol error; from=<lvtowxfdp@yahoo.com>, to=<tom@example.com>, proto=SMTP, helo=<mx32.usaindiamunish.net>",
    {
      postfix_action = "reject",
      postfix_client_ip = "182.98.255.184",
      postfix_client_port = 2413,
      postfix_keyvalue_data = "from=<lvtowxfdp@yahoo.com>, to=<tom@example.com>, proto=SMTP, helo=<mx32.usaindiamunish.net>",
      postfix_smtp_stage = "RCPT",
      postfix_status_code = 550,
      postfix_status_code_enhanced = "5.5.1",
      postfix_status_message = "Protocol error"
    }
  },
  ["postscreen_0028.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "NOQUEUE: reject: RCPT from [27.157.200.233]:4984: 550 5.7.1 Service unavailable; client [27.157.200.233] blocked using zen.spamhaus.org; from=<hcdvbnm@qhhn.com>, to=<4ECEA00F.9040306@example.com>, proto=ESMTP, helo=<qhhn.com>",
    {
      postfix_action = "reject",
      postfix_client_ip = "27.157.200.233",
      postfix_client_port = 4984,
      postfix_keyvalue_data = "from=<hcdvbnm@qhhn.com>, to=<4ECEA00F.9040306@example.com>, proto=ESMTP, helo=<qhhn.com>",
      postfix_smtp_stage = "RCPT",
      postfix_status_code = 550,
      postfix_status_code_enhanced = "5.7.1",
      postfix_status_data = "27.157.200.233",
      postfix_status_message = "blocked using zen.spamhaus.org"
    }
  },
  ["postscreen_0029.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "warning: getpeername: Transport endpoint is not connected -- dropping this connection",
    {
      postfix_warning = "getpeername: Transport endpoint is not connected -- dropping this connection"
    }
  },
  ["qmgr_0001.yaml"] = {
    "POSTFIX_QMGR",
    "0F5383D: removed",
    {
      postfix_queueid = "0F5383D"
    }
  },
  ["qmgr_0002.yaml"] = {
    "POSTFIX_QMGR",
    "0F5383D: from=<owner-postfix-users@postfix.org>, size=5360, nrcpt=1 (queue active)",
    {
      postfix_keyvalue_data = "from=<owner-postfix-users@postfix.org>, size=5360, nrcpt=1",
      postfix_queueid = "0F5383D"
    }
  },
  ["qmgr_0003.yaml"] = {
    "POSTFIX_QMGR",
    "warning: bounce_queue_lifetime is larger than maximal_queue_lifetime - adjusting bounce_queue_lifetime",
    {
      postfix_warning = "bounce_queue_lifetime is larger than maximal_queue_lifetime - adjusting bounce_queue_lifetime"
    }
  },
  ["relay_info_0001.yaml"] = {
    "POSTFIX_RELAY_INFO",
    "1.example.com.si[private/dovecot-lmtp]",
    {
      postfix_relay_hostname = "1.example.com.si",
      postfix_relay_service = "private/dovecot-lmtp"
    }
  },
  ["scache_0001.yaml"] = {
    "POSTFIX_SCACHE",
    "statistics: domain lookup hits=0 miss=1 success=0%",
    {
      postfix_scache_hits = 0,
      postfix_scache_miss = 1,
      postfix_scache_success = 0
    }
  },
  ["scache_0002.yaml"] = {
    "POSTFIX_SCACHE",
    "statistics: start interval Dec  6 21:20:35",
    {
      postfix_scache_timestamp = "Dec  6 21:20:35"
    }
  },
  ["scache_0003.yaml"] = {
    "POSTFIX_SCACHE",
    "statistics: address lookup hits=0 miss=1 success=0%",
    {
      postfix_scache_hits = 0,
      postfix_scache_miss = 1,
      postfix_scache_success = 0
    }
  },
  ["scache_0004.yaml"] = {
    "POSTFIX_SCACHE",
    "statistics: max simultaneous domains=1 addresses=1 connection=1",
    {
      postfix_scache_addresses = 1,
      postfix_scache_connection = 1,
      postfix_scache_domains = 1
    }
  },
  ["sendmail_0001.yaml"] = {
    "POSTFIX_SENDMAIL",
    "fatal: root(0): message file too big",
    {
      postfix_warning = "root(0): message file too big"
    }
  },
  ["smtp_0001.yaml"] = {
    "POSTFIX_SMTP",
    "7EE668039: to=<admin@example.com>, relay=127.0.0.1[127.0.0.1]:2525, delay=3.6, delays=0.2/0.02/0.04/3.3, dsn=2.0.0, status=sent (250 2.0.0 Ok: queued as 153053D)",
    {
      postfix_keyvalue_data = "to=<admin@example.com>, relay=127.0.0.1[127.0.0.1]:2525, delay=3.6, delays=0.2/0.02/0.04/3.3, dsn=2.0.0,",
      postfix_queueid = "7EE668039",
      postfix_smtp_response = "250 2.0.0 Ok: queued as 153053D",
      postfix_status = "sent"
    }
  },
  ["smtp_0002.yaml"] = {
    "POSTFIX_SMTP",
    "0D9633D: to=<jos@example.com>, orig_to=<bestuur@example.com>, relay=mail1.example.net[10.74.103.11]:25, delay=11, delays=5.4/0.06/5.1/0.03, dsn=2.0.0, status=sent (250 2.0.0 Ok: queued as F12DF84004)",
    {
      postfix_keyvalue_data = "to=<jos@example.com>, orig_to=<bestuur@example.com>, relay=mail1.example.net[10.74.103.11]:25, delay=11, delays=5.4/0.06/5.1/0.03, dsn=2.0.0,",
      postfix_queueid = "0D9633D",
      postfix_smtp_response = "250 2.0.0 Ok: queued as F12DF84004",
      postfix_status = "sent"
    }
  },
  ["smtp_0003.yaml"] = {
    "POSTFIX_SMTP",
    "Untrusted TLS connection established to mx4.hotmail.com[65.55.92.136]:25: TLSv1.2 with cipher ECDHE-RSA-AES256-SHA384 (256/256 bits)",
    {
      postfix_relay_hostname = "mx4.hotmail.com",
      postfix_relay_ip = "65.55.92.136",
      postfix_relay_port = 25,
      postfix_tls_cipher = "ECDHE-RSA-AES256-SHA384",
      postfix_tls_cipher_size = "256/256",
      postfix_tls_version = "TLSv1.2"
    }
  },
  ["smtp_0004.yaml"] = {
    "POSTFIX_SMTP",
    "Untrusted TLS connection established to 127.0.0.1[127.0.0.1]:2525: TLSv1.2 with cipher AECDH-AES256-SHA (256/256 bits)",
    {
      postfix_relay_hostname = "127.0.0.1",
      postfix_relay_ip = "127.0.0.1",
      postfix_relay_port = 2525,
      postfix_tls_cipher = "AECDH-AES256-SHA",
      postfix_tls_cipher_size = "256/256",
      postfix_tls_version = "TLSv1.2"
    }
  },
  ["smtp_0005.yaml"] = {
    "POSTFIX_SMTP",
    "connect to intern.nl[194.109.6.98]:25: Connection timed out",
    {
      postfix_relay_hostname = "intern.nl",
      postfix_relay_ip = "194.109.6.98",
      postfix_relay_port = 25
    }
  },
  ["smtp_0006.yaml"] = {
    "POSTFIX_SMTP",
    "B99FE3D: lost connection with mx3.hotmail.com[65.55.37.72] while receiving the initial server greeting",
    {
      postfix_queueid = "B99FE3D",
      postfix_relay_hostname = "mx3.hotmail.com",
      postfix_relay_ip = "65.55.37.72"
    }
  },
  ["smtp_0007.yaml"] = {
    "POSTFIX_SMTP",
    "connect to mail2.crabruzzo.it[2.115.207.21]:25: No route to host",
    {
      postfix_relay_hostname = "mail2.crabruzzo.it",
      postfix_relay_ip = "2.115.207.21",
      postfix_relay_port = 25
    }
  },
  ["smtp_0015.yaml"] = {
    "POSTFIX_SMTP",
    "Trusted TLS connection established to gmail-smtp-in.l.google.com[74.125.136.26]:25: TLSv1.2 with cipher ECDHE-RSA-AES128-GCM-SHA256 (128/128 bits)",
    {
      postfix_relay_hostname = "gmail-smtp-in.l.google.com",
      postfix_relay_ip = "74.125.136.26",
      postfix_relay_port = 25,
      postfix_tls_cipher = "ECDHE-RSA-AES128-GCM-SHA256",
      postfix_tls_cipher_size = "128/128",
      postfix_tls_version = "TLSv1.2"
    }
  },
  ["smtp_0016.yaml"] = {
    "POSTFIX_SMTP",
    "Verified TLS connection established to mail.sys4.de[2001:1578:400:111::7]:25: TLSv1 with cipher ECDHE-RSA-AES256-SHA (256/256 bits)",
    {
      postfix_relay_hostname = "mail.sys4.de",
      postfix_relay_ip = "2001:1578:400:111::7",
      postfix_relay_port = 25,
      postfix_tls_cipher = "ECDHE-RSA-AES256-SHA",
      postfix_tls_cipher_size = "256/256",
      postfix_tls_version = "TLSv1"
    }
  },
  ["smtp_0017.yaml"] = {
    "POSTFIX_SMTP",
    "AD6E41CDC05C: to=<admin@example.com>, relay=sub1.example.com[private/dovecot-lmtp], delay=0.15, delays=0.04/0/0.02/0.08, dsn=2.0.0, status=sent (250 2.0.0 <admin@example.com> qYyyBMsl9FRuZQAAA15QOA Saved)",
    {
      postfix_keyvalue_data = "to=<admin@example.com>, relay=sub1.example.com[private/dovecot-lmtp], delay=0.15, delays=0.04/0/0.02/0.08, dsn=2.0.0,",
      postfix_queueid = "AD6E41CDC05C",
      postfix_smtp_response = "250 2.0.0 <admin@example.com> qYyyBMsl9FRuZQAAA15QOA Saved",
      postfix_status = "sent"
    }
  },
  ["smtp_0018.yaml"] = {
    "POSTFIX_SMTP",
    "4E74613FE76: to=<redacted@gmail.com>, relay=gmail-smtp-in.l.google.com[2607:f8b0:400e:c04::1b]:25, delay=1.9, delays=0.05/0/1.1/0.71, dsn=5.7.1, status=bounced (host gmail-smtp-in.l.google.com[2607:f8b0:400e:c04::1b] said: 550-5.7.1 [2607:4100:3:25:100::2      12] Our system has detected that this 550-5.7.1 message is likely unsolicited mail. To reduce the amount of spam sent 550-5.7.1 to Gmail, this message has been blocked. Please visit 550-5.7.1 http://support.google.com/mail/bin/answer.py?hl=en&answer=188131 for 550 5.7.1 more information. qc12si8475009pab.211 - gsmtp (in reply to end of DATA command))",
    {
      postfix_keyvalue_data = "to=<redacted@gmail.com>, relay=gmail-smtp-in.l.google.com[2607:f8b0:400e:c04::1b]:25, delay=1.9, delays=0.05/0/1.1/0.71, dsn=5.7.1,",
      postfix_queueid = "4E74613FE76",
      postfix_smtp_response = "host gmail-smtp-in.l.google.com[2607:f8b0:400e:c04::1b] said: 550-5.7.1 [2607:4100:3:25:100::2      12] Our system has detected that this 550-5.7.1 message is likely unsolicited mail. To reduce the amount of spam sent 550-5.7.1 to Gmail, this message has been blocked. Please visit 550-5.7.1 http://support.google.com/mail/bin/answer.py?hl=en&answer=188131 for 550 5.7.1 more information. qc12si8475009pab.211 - gsmtp (in reply to end of DATA command)",
      postfix_status = "bounced"
    }
  },
  ["smtp_0019.yaml"] = {
    "POSTFIX_SMTP",
    "warning: problem talking to service private/scache: Connection timed out",
    {
      postfix_warning = "problem talking to service private/scache: Connection timed out"
    }
  },
  ["smtpd_0001.yaml"] = {
    "POSTFIX_SMTPD",
    "connect from 061238241086.static.ctinets.com[61.238.241.86]",
    {
      postfix_client_hostname = "061238241086.static.ctinets.com",
      postfix_client_ip = "61.238.241.86"
    }
  },
  ["smtpd_0002.yaml"] = {
    "POSTFIX_SMTPD",
    "disconnect from 061238241086.static.ctinets.com[61.238.241.86]",
    {
      postfix_client_hostname = "061238241086.static.ctinets.com",
      postfix_client_ip = "61.238.241.86"
    }
  },
  ["smtpd_0003.yaml"] = {
    "POSTFIX_SMTPD",
    "4104E3D: client=061238241086.static.ctinets.com[61.238.241.86]",
    {
      postfix_keyvalue_data = "client=061238241086.static.ctinets.com[61.238.241.86]",
      postfix_queueid = "4104E3D"
    }
  },
  ["smtpd_0004.yaml"] = {
    "POSTFIX_SMTPD",
    "NOQUEUE: reject: RCPT from 061238241086.static.ctinets.com[61.238.241.86]: 550 5.1.1 <toernooicommissie@example.com>: Recipient address rejected: User unknown in virtual mailbox table; from=<wumt5@cchfdc.com> to=<toernooicommissie@example.com> proto=ESMTP helo=<ecsolved.com>",
    {
      postfix_action = "reject",
      postfix_client_hostname = "061238241086.static.ctinets.com",
      postfix_client_ip = "61.238.241.86",
      postfix_keyvalue_data = "from=<wumt5@cchfdc.com> to=<toernooicommissie@example.com> proto=ESMTP helo=<ecsolved.com>",
      postfix_smtp_stage = "RCPT",
      postfix_status_code = 550,
      postfix_status_code_enhanced = "5.1.1",
      postfix_status_data = "toernooicommissie@example.com",
      postfix_status_message = "Recipient address rejected: User unknown in virtual mailbox table"
    }
  },
  ["smtpd_0005.yaml"] = {
    "POSTFIX_SMTPD",
    "lost connection after STARTTLS from unknown[72.13.58.7]",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "72.13.58.7",
      postfix_smtp_stage = "STARTTLS",
      postfix_smtpd_lostconn_data = "lost connection"
    }
  },
  ["smtpd_0006.yaml"] = {
    "POSTFIX_SMTPD",
    "warning: hostname exemple.com does not resolve to address 185.14.29.32",
    {
      postfix_warning = "hostname exemple.com does not resolve to address 185.14.29.32"
    }
  },
  ["smtpd_0007.yaml"] = {
    "POSTFIX_SMTPD",
    "timeout after CONNECT from unknown[177.227.18.3]",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "177.227.18.3",
      postfix_smtp_stage = "CONNECT",
      postfix_smtpd_lostconn_data = "timeout"
    }
  },
  ["smtpd_0008.yaml"] = {
    "POSTFIX_POSTSCREEN",
    "NOQUEUE: reject: RCPT from unknown[2001:980:cfb1:1:82f:f74e:a45c:3033]: 504 5.5.2 <aap@henk>: Sender address rejected: need fully-qualified address; from=<aap@henk> to=<user@example.com> proto=SMTP helo=<test>",
    {
      postfix_action = "reject",
      postfix_client_hostname = "unknown",
      postfix_client_ip = "2001:980:cfb1:1:82f:f74e:a45c:3033",
      postfix_keyvalue_data = "from=<aap@henk> to=<user@example.com> proto=SMTP helo=<test>",
      postfix_smtp_stage = "RCPT",
      postfix_status_code = 504,
      postfix_status_code_enhanced = "5.5.2",
      postfix_status_data = "aap@henk",
      postfix_status_message = "Sender address rejected: need fully-qualified address"
    }
  },
  ["smtpd_0009.yaml"] = {
    "POSTFIX_SMTPD",
    "NOQUEUE: reject: RCPT from news.zihan-promo.com[192.36.205.58]: 554 5.7.1 Service unavailable; Helo command [news.zihan-promo.com] blocked using dbl.spamhaus.org; http://www.spamhaus.org/query/dbl?domain=zihan-promo.com; from=<mark.alexander@zihan-promo.com> to=<tom@example.com> proto=ESMTP helo=<news.zihan-promo.com>",
    {
      postfix_action = "reject",
      postfix_client_hostname = "news.zihan-promo.com",
      postfix_client_ip = "192.36.205.58",
      postfix_keyvalue_data = "from=<mark.alexander@zihan-promo.com> to=<tom@example.com> proto=ESMTP helo=<news.zihan-promo.com>",
      postfix_smtp_stage = "RCPT",
      postfix_status_code = 554,
      postfix_status_code_enhanced = "5.7.1",
      postfix_status_data = "news.zihan-promo.com",
      postfix_status_message = "blocked using dbl.spamhaus.org; http://www.spamhaus.org/query/dbl?domain=zihan-promo.com"
    }
  },
  ["smtpd_0010.yaml"] = {
    "POSTFIX_SMTPD",
    "Anonymous TLS connection established from julie.example.com[10.163.89.202]: TLSv1.2 with cipher AECDH-AES256-SHA (256/256 bits)",
    {
      postfix_client_hostname = "julie.example.com",
      postfix_client_ip = "10.163.89.202",
      postfix_tls_cipher = "AECDH-AES256-SHA",
      postfix_tls_cipher_size = "256/256",
      postfix_tls_version = "TLSv1.2"
    }
  },
  ["smtpd_0011.yaml"] = {
    "POSTFIX_SMTPD",
    "lost connection after DATA (7774180 bytes) from unknown[72.13.58.7]",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "72.13.58.7",
      postfix_smtp_stage = "DATA",
      postfix_smtpd_lostconn_data = "lost connection"
    }
  },
  ["smtpd_0012.yaml"] = {
    "POSTFIX_SMTPD",
    "lost connection after UNKNOWN from unknown[2001:456:cfb1:1:f5d7:dead:beef:cafe]",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "2001:456:cfb1:1:f5d7:dead:beef:cafe",
      postfix_smtp_stage = "UNKNOWN",
      postfix_smtpd_lostconn_data = "lost connection"
    }
  },
  ["smtpd_0013.yaml"] = {
    "POSTFIX_SMTPD",
    "lost connection after RSET from unknown[72.13.58.7]",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "72.13.58.7",
      postfix_smtp_stage = "RSET",
      postfix_smtpd_lostconn_data = "lost connection"
    }
  },
  ["smtpd_0014.yaml"] = {
    "POSTFIX_SMTPD",
    "timeout after END-OF-MESSAGE from unknown[72.13.58.7]",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "72.13.58.7",
      postfix_smtp_stage = "END-OF-MESSAGE",
      postfix_smtpd_lostconn_data = "timeout"
    }
  },
  ["smtpd_0015.yaml"] = {
    "POSTFIX_SMTPD",
    "improper command pipelining after EHLO from unknown[2001:968:9999:20:415c:cd2:da8e:d0cf]: MAIL FROM:<>\nRCPT TO:<tom@example.net>\nDATA\nSubject: pipeline test\n\nThis is a test\n.\nQUIT\n\r\n",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "2001:968:9999:20:415c:cd2:da8e:d0cf",
      postfix_smtp_stage = "EHLO"
    }
  },
  ["smtpd_0016.yaml"] = {
    "POSTFIX_SMTPD",
    "lost connection after DATA (0 bytes) from unknown[72.13.58.7]",
    {
      postfix_client_hostname = "unknown",
      postfix_client_ip = "72.13.58.7",
      postfix_smtp_stage = "DATA",
      postfix_smtpd_lostconn_data = "lost connection"
    }
  },
  ["smtpd_0017.yaml"] = {
    "POSTFIX_SMTPD",
    "NOQUEUE: reject: VRFY from unknown[2001:968:9999:20:88b:9b7d:2a54:2bd2]: 454 4.7.1 <aap@henk.com>: Relay access denied; to=<aap@henk.com> proto=SMTP helo=<me>",
    {
      postfix_action = "reject",
      postfix_client_hostname = "unknown",
      postfix_client_ip = "2001:968:9999:20:88b:9b7d:2a54:2bd2",
      postfix_keyvalue_data = "to=<aap@henk.com> proto=SMTP helo=<me>",
      postfix_smtp_stage = "VRFY",
      postfix_status_code = 454,
      postfix_status_code_enhanced = "4.7.1",
      postfix_status_data = "aap@henk.com",
      postfix_status_message = "Relay access denied"
    }
  },
  ["smtpd_0018.yaml"] = {
    "POSTFIX_SMTPD",
    "NOQUEUE: reject: VRFY from unknown[2001:968:9999:20:88b:9b7d:2a54:2bd2]: 550 5.1.1 <does-not-exist@example.com>: Recipient address rejected: User unknown in virtual mailbox table; to=<does-not-exist@example.com> proto=SMTP helo=<me>",
    {
      postfix_action = "reject",
      postfix_client_hostname = "unknown",
      postfix_client_ip = "2001:968:9999:20:88b:9b7d:2a54:2bd2",
      postfix_keyvalue_data = "to=<does-not-exist@example.com> proto=SMTP helo=<me>",
      postfix_smtp_stage = "VRFY",
      postfix_status_code = 550,
      postfix_status_code_enhanced = "5.1.1",
      postfix_status_data = "does-not-exist@example.com",
      postfix_status_message = "Recipient address rejected: User unknown in virtual mailbox table"
    }
  },
  ["smtpd_0019.yaml"] = {
    "POSTFIX_SMTPD",
    "SSL_accept error from unknown[72.13.58.7]: lost connection",
    {
      postfix_action = "SSL_accept error",
      postfix_client_hostname = "unknown",
      postfix_client_ip = "72.13.58.7",
      postfix_smtpd_lostconn_data = "lost connection"
    }
  },
  ["smtpd_0020.yaml"] = {
    "POSTFIX_SMTPD",
    "proxy-accept: END-OF-MESSAGE: 250 2.0.0 from MTA(smtp:[127.0.0.1]:10013): 250 2.0.0 Ok: queued as DF66E940333; from=<user1@test.example.com> to=<user2@example2.com> proto=ESMTP helo=<[127.0.0.1]>",
    {
      postfix_keyvalue_data = "from=<user1@test.example.com> to=<user2@example2.com> proto=ESMTP helo=<[127.0.0.1]>",
      -- postfix_proxy_message = "250 2.0.0 from MTA(smtp:[127.0.0.1]:10013): 250 2.0.0 Ok: queued as DF66E940333", -- FIXME
      postfix_proxy_result = "accept",
      postfix_proxy_smtp_stage = "END-OF-MESSAGE",
      postfix_proxy_status_code = 250,
      postfix_proxy_status_code_enhanced = "2.0.0"
    }
  },
  ["smtpd_0021.yaml"] = {
    "POSTFIX_SMTPD",
    "proxy-reject: END-OF-MESSAGE: ; from=<user1@test.example.com> to=<user2@example2.com> proto=ESMTP helo=<[192.168.0.16]>",
    {
      postfix_keyvalue_data = "from=<user1@test.example.com> to=<user2@example2.com> proto=ESMTP helo=<[192.168.0.16]>",
      -- postfix_proxy_message = "", -- FIXME
      postfix_proxy_result = "reject",
      postfix_proxy_smtp_stage = "END-OF-MESSAGE"
    }
  },
  ["smtpd_0022.yaml"] = {
    "POSTFIX_SMTPD",
    "proxy-reject: END-OF-MESSAGE: 554 5.7.0 Reject, contact postmaster@example.com, id=31619-02 - spam; from=<> to=<user2@example2.com> proto=SMTP helo=<17.33.12.53>",
    {
      postfix_keyvalue_data = "from=<> to=<user2@example2.com> proto=SMTP helo=<17.33.12.53>",
      -- postfix_proxy_message = "554 5.7.0 Reject, contact postmaster@example.com, id=31619-02 - spam", -- FIXME
      postfix_proxy_result = "reject",
      postfix_proxy_smtp_stage = "END-OF-MESSAGE",
      postfix_proxy_status_code = 554,
      postfix_proxy_status_code_enhanced = "5.7.0"
    }
  },
  ["smtpd_0023.yaml"] = {
    "POSTFIX_SMTPD",
    "SSL_accept error from unknown[72.13.58.7]: Connection timed out",
    {
      postfix_action = "SSL_accept error",
      postfix_client_hostname = "unknown",
      postfix_client_ip = "72.13.58.7",
      postfix_smtpd_lostconn_data = "Connection timed out"
    }
  },
  ["tlsmgr_0001.yaml"] = {
    "POSTFIX_TLSMGR",
    "warning: request to update table btree:/var/spool/postfix/smtpd_scache in non-postfix directory /var/spool/postfix",
    {
      postfix_warning = "request to update table btree:/var/spool/postfix/smtpd_scache in non-postfix directory /var/spool/postfix"
    }
  },
  ["tlsproxy_0001.yaml"] = {
    "POSTFIX_TLSPROXY",
    "DISCONNECT [82.118.17.142]:51637",
    {
      postfix_client_ip = "82.118.17.142",
      postfix_client_port = 51637
    }
  },
  ["tlsproxy_0002.yaml"] = {
    "POSTFIX_TLSPROXY",
    "CONNECT from [82.118.17.142]:51637",
    {
      postfix_client_ip = "82.118.17.142",
      postfix_client_port = 51637
    }
  },
  ["trivial_rewrite_0001.yaml"] = {
    "POSTFIX_TRIVIAL_REWRITE",
    "warning: virtual_alias_domains lookup failure",
    {
      postfix_warning = "virtual_alias_domains lookup failure"
    }
  },
  ["trivial_rewrite_0002.yaml"] = {
    "POSTFIX_TRIVIAL_REWRITE",
    "warning: dict_ldap_lookup: Search error -1: Can't contact LDAP server ",
    {
      postfix_warning = "dict_ldap_lookup: Search error -1: Can't contact LDAP server "
    }
  },
  -- testsuite additions, not in postfix-grok-patterns
  ["keyvalue_data_0001.yaml"] = {
    "POSTFIX_KEYVALUE_DATA",
    "from=<>, to=<tom@example.com>, relay=dovecot, delay=4.3, delays=4.1/0.01/0/0.15, dsn=2.0.0, status=sent",
    {
      from = '',
      to = 'tom@example.com',
      relay = 'dovecot',
      delay = '4.3',
      delays = '4.1/0.01/0/0.15',
      dsn = '2.0.0',
      status='sent'
    }
  }
}

function process_one(filename, programmname, message, expect, extract_keyvalue_data)
   local ret = pf.postfix_match(programmname, message, extract_keyvalue_data)
   if not ret then
     error(string.format("%s: failed match", filename))
   elseif type(ret) ~= 'table' then
     error(string.format("%s: incorrect type %s: %s", filename, type(ret), ret))
   else
     for k,v in pairs(expect) do
       if not ret[k] then
         error(string.format("%s: missing key '%s'", filename, k))
       else
         if type(ret[k]) == 'table' then
           if ret[k]['value'] ~= v then
             error(string.format("%s: data mismatch for '%s': '%s' != '%s'", filename, k, ret[k]['value'], v))
           end
         elseif ret[k] ~= v then
           error(string.format("%s: data mismatch for '%s': '%s' != '%s'", filename, k, ret[k], v))
         end
       end
     end
   end
end


function process()
  for filename, test in pairs(tests) do
    local pattern = test[1]
    local data = test[2]
    local results = test[3]
    local programmname = string.gsub(string.lower(pattern), '_', '/', 1)
    process_one(filename, programmname, data, results)
  end
  -- test with extract_keyvalue_data
  local message = '25F2E5E061: to=<george.desantis@test.example.com>, relay=none, delay=0.05, delays=0.05/0/0/0, dsn=2.0.0, status=sent (test.example.com)'
  local expect = {
    postfix_queueid = "25F2E5E061",
    postfix_to = 'george.desantis@test.example.com',
    postfix_relay = 'none',
    postfix_delay = 0.05,
    postfix_delay_before_qmgr = 0.05,
    postfix_delay_in_qmgr = 0,
    postfix_delay_conn_setup = 0,
    postfix_delay_transmission = 0,
    postfix_dsn = '2.0.0',
    postfix_status = 'sent'
   }
  process_one("discard_0001.yaml+extract_keyvalue_data", 'mta-in/discard', message, expect, true)
  return 0
end
