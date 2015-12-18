-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Copyright 2015 Mathieu Parent <math.parent@gmail.com>

local string = require 'string'
local ip = require 'ip_address'
local l = require 'lpeg'
l.locale(l)
local ipairs = ipairs
local pairs = pairs
local rawset = rawset
local tonumber = tonumber
local type = type

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

-- Basic types
local notspace = l.P(1) - l.space
local integer = l.digit^1 / tonumber
local number = (l.digit^1 * (l.P'.' * l.digit^1)^-1) / tonumber

-- keyvalue_data
local kv_key = l.C(l.alpha^1)
local kv_pair_end = (l.P', ' * l.alpha^1 * l.P'=') + (l.P',' * -1) + -1
local kv_value = l.P'<' * l.C((l.P(1) - (l.P'>' * kv_pair_end))^0) * l.P'>'
                        + l.C((l.P(1) - kv_pair_end)^0)
local kv_sep = l.P', ' + (l.P',' * -1) + -1
local kv_pair = l.Cg(kv_key * '=' * kv_value) * kv_sep^-1
local kv_grammar = l.Cf(l.Ct'' * kv_pair^0, rawset)

-- Hostname and IPs
local ipv46         = l.Ct(l.Cg(ip.v4, 'value') * l.Cg(l.Cc'ipv4', 'representation'))
                    + l.Ct(l.Cg(ip.v6, 'value') * l.Cg(l.Cc'ipv6', 'representation'))
local hostname      = l.Ct(l.Cg((l.P(1) - l.S'[] ')^1, 'value') * l.Cg(l.Cc'hostname', 'representation'))
local host          = ipv46 + hostname
local port          = l.Ct(l.Cg(integer, 'value') * l.Cg(l.Cc'2', 'value_type'))

-- Postfix grammar
-- Inspired by https://github.com/whyscream/postfix-grok-patterns
-- 2015-05-26 8fdd562d159845b97bf8d9c273e5357f3a143a94
-- Original under BSD-3-clause license
-- Adaptation 2015 Mathieu Parent <math.parent@gmail.com>
local postfix_queueid = l.xdigit ^ 6 + l.alnum ^ 15 + l.P'NOQUEUE'
local postfix_queueid_cg = l.Cg(postfix_queueid, 'postfix_queueid')

local postfix_client_hostname_cg = l.Cg(host, 'postfix_client_hostname')
local postfix_client_ip_cg       = l.Cg(ipv46, 'postfix_client_ip')
local postfix_client_port_cg     = l.Cg(port, 'postfix_client_port')
local postfix_client_info_cg     = postfix_client_hostname_cg^-1
                                 * l.P'[' * postfix_client_ip_cg * l.P']'
                                 * (l.P':' * postfix_client_port_cg)^-1
local postfix_client_info_ct     = l.Ct(postfix_client_info_cg)

local postfix_relay_hostname_cg = l.Cg(host, 'postfix_relay_hostname')
local postfix_relay_ip_cg       = l.Cg(ipv46, 'postfix_relay_ip')
local postfix_relay_service_cg  = l.Cg((l.P(1)-l.P']')^1, 'postfix_relay_service')
local postfix_relay_port_cg     = l.Cg(port, 'postfix_relay_port')
local postfix_relay_info_cg     = postfix_relay_hostname_cg^-1
                                * l.P'[' * (postfix_relay_ip_cg + postfix_relay_service_cg) * l.P']'
                                * (l.P':' * postfix_relay_port_cg)^-1
local postfix_relay_info_ct     = l.Ct(postfix_relay_info_cg)

local postfix_smtp_stage = l.P'CONNECT'
                         + l.P'HELO'
                         + l.P'EHLO'
                         + l.P'STARTTLS'
                         + l.P'AUTH'
                         + l.P'MAIL'
                         + l.P'RCPT'
                         + l.P'DATA'
                         + l.P'RSET'
                         + l.P'UNKNOWN'
                         + l.P'END-OF-MESSAGE'
                         + l.P'VRFY'
                         + l.P'.'
local postfix_smtp_stage_cg = l.Cg(postfix_smtp_stage, 'postfix_smtp_stage')
local postfix_proxy_smtp_stage_cg = l.Cg(postfix_smtp_stage, 'postfix_proxy_smtp_stage')

local postfix_action = l.P'reject'
                     + l.P'defer'
                     + l.P'accept'
                     + l.P'header-redirect'
local postfix_action_cg = l.Cg(postfix_action, 'postfix_action')
local postfix_proxy_result_cg = l.Cg(postfix_action, 'postfix_proxy_result')

local postfix_status_code =( l.digit * l.digit * l.digit) / tonumber
local postfix_status_code_cg = l.Cg(postfix_status_code, 'postfix_status_code')
local postfix_status_code_enhanced = l.digit * l.P'.' * l.digit * l.P'.' * l.digit
local postfix_status_code_enhanced_cg = l.Cg(postfix_status_code_enhanced, 'postfix_status_code_enhanced')
local postfix_dnsbl_message_cg = l.P'Service unavailable; '
                               * (l.P(1) - l.P'[')^0
                               * l.P'['
                               * l.Cg((l.P(1) - l.P']')^0, 'postfix_status_data')
                               * l.P'] '
                               * l.Cg((l.P(1)-(l.P'; ' * l.alpha^1 * l.P'='))^1, 'postfix_status_message')
                               * l.P';'
local postfix_ps_access_action = l.P'DISCONNECT'
                               + l.P'BLACKLISTED'
                               + l.P'WHITELISTED'
                               + l.P'WHITELIST VETO'
                               + l.P'PASS NEW'
                               + l.P'PASS OLD'
local postfix_ps_violation_cg = l.Cg(l.P'BARE NEWLINE'
                              + l.P'COMMAND TIME LIMIT'
                              + l.P'COMMAND COUNT LIMIT'
                              + l.P'COMMAND LENGTH LIMIT'
                              + l.P'COMMAND PIPELINING'
                              + l.P'DNSBL'
                              + l.P'HANGUP'
                              + l.P'NON-SMTP COMMAND'
                              + l.P'PREGREET',
                              'postfix_postscreen_violation')
local postfix_time_unit = l.digit^1 * l.S'smhd'
local postfix_keyvalue_greedy_cg = postfix_queueid_cg
                                 * l.P': '
                                 * l.Cg(l.P(1)^1, 'postfix_keyvalue_data')
local postfix_warning_cg = (l.P'warning' + l.P'fatal') * ': ' * l.Cg(l.P(1)^1, 'postfix_warning')
local postfix_tlsconn_cg = (l.P'Anonymous' + l.P'Trusted' + l.P'Untrusted' + l.P'Verified')
                      * l.P' TLS connection established '
                      * ((l.P'to ' * postfix_relay_info_cg) + (l.P'from ' * postfix_client_info_cg)) * l.P': '
                      * l.Cg(notspace^0, 'postfix_tls_version')
                      * l.P' with cipher '
                      * l.Cg(notspace^0, 'postfix_tls_cipher')
                      * l.P' (' * l.Cg(notspace^0, 'postfix_tls_cipher_size') * l.P' bits)'
local postfix_delays_ct = l.Ct(l.Cg(number, 'postfix_delay_before_qmgr')
                        * l.P'/'
                        * l.Cg(number, 'postfix_delay_in_qmgr')
                        * l.P'/'
                        * l.Cg(number, 'postfix_delay_conn_setup')
                        * l.P'/'
                        * l.Cg(number, 'postfix_delay_transmission'))
local postfix_lostconn = l.P'lost connection'
                       + l.P'timeout'
                       + l.P'Connection timed out'
local postfix_smtpd_lostconn_data_cg = l.Cg(postfix_lostconn, 'postfix_smtpd_lostconn_data')
local postfix_proxy_message_cg = (l.Cg(postfix_status_code, 'postfix_proxy_status_code') * l.P' ')^-1 * (l.Cg(postfix_status_code_enhanced, 'postfix_proxy_status_code_enhanced') * l.P' ')^-1 * (l.P(1) - l.P';')^0

-- smtpd patterns
local postfix_smtpd_connect_cg = l.P'connect from '
                               * postfix_client_info_cg
local postfix_smtpd_disconnect_cg = l.P'disconnect from '
                                  * postfix_client_info_cg
local postfix_smtpd_lostconn_cg = (postfix_smtpd_lostconn_data_cg
                                * l.P' after '
                                * postfix_smtp_stage_cg
                                * (l.P' (' * l.digit^1 * l.P' bytes)') ^-1
                                * l.P' from '
                                * postfix_client_info_cg)
                                + (l.Cg((l.P(1) - l.P' from ')^1, 'postfix_action')
                                * l.P' from '
                                * postfix_client_info_cg
                                * l.P': '
                                * postfix_smtpd_lostconn_data_cg)
local postfix_smtpd_noqueue_cg = l.P'NOQUEUE: '
                               * postfix_action_cg
                               * l.P': '
                               * postfix_smtp_stage_cg
                               * l.P' from '
                               * postfix_client_info_cg
                               * l.P': '
                               * postfix_status_code_cg
                               * l.P' '
                               * postfix_status_code_enhanced_cg
                               * (l.P' <' * l.Cg((l.P(1) - l.P'>')^1, 'postfix_status_data') * l.P'>:')^-1
                               * l.P' '
                               * (postfix_dnsbl_message_cg + (l.Cg((l.P(1)-l.P';')^0, 'postfix_status_message') * l.P';'))
                               * l.P' '
                               * l.Cg(l.P(1)^1, 'postfix_keyvalue_data')
local postfix_smtpd_pipelining_cg = l.P'improper command pipelining after ' * postfix_smtp_stage_cg * l.P' from ' * postfix_client_info_cg * l.P':'
local postfix_smtpd_proxy_cg = l.P'proxy-'
                             * postfix_proxy_result_cg
                             * l.P': '
                             * postfix_proxy_smtp_stage_cg
                             * l.P': '
                             * postfix_proxy_message_cg -- FIXME this should fill postfix_proxy_message
                             * l.P'; '
                             * l.Cg(l.P(1)^1, 'postfix_keyvalue_data')

-- cleanup patterns
local postfix_cleanup_milter_cg = postfix_queueid_cg
                                * l.P': '
                                * l.P'milter-'
                                * l.Cg(postfix_action, 'postfix_milter_result')
                                * l.P': '
                                * l.Cg((l.P(1)-l.P';')^1, 'postfix_milter_message')
                                * l.P'; '
                                * l.Cg((l.P(1)-l.P':')^1, 'postfix_keyvalue_data')
                                *(l.P': ' * l.Cg(l.P(1)^1, 'postfix_milter_data'))^-1

-- qmgr patterns
local postfix_qmgr_removed_cg = postfix_queueid_cg * l.P': removed'
local postfix_qmgr_active_cg = postfix_queueid_cg
                             * l.P': '
                             * l.Cg((l.P(1) - l.P' (')^1, 'postfix_keyvalue_data')
                             * l.P' (queue active)'

-- pipe patterns
local postfix_pipe_delivered_cg = postfix_queueid_cg
                                * l.P': '
                                * l.Cg((l.P(1) - l.P' (')^1, 'postfix_keyvalue_data')
                                * l.P' (delivered via '
                                * l.Cg(l.alnum^1, 'postfix_pipe_service')
local postfix_pipe_forward_cg = postfix_queueid_cg
                              * l.P': '
                              * l.Cg((l.P(1) - l.P' (')^1, 'postfix_keyvalue_data')
                              * l.P' (mail forwarding loop for '
                              * l.Cg((l.P(1) - l.P')')^1, 'postfix_to')

-- postscreen patterns
local postfix_ps_connect_cg = l.P'CONNECT from '
                            * postfix_client_info_cg
                            * l.P' to ['
                            * l.Cg(ipv46, 'postfix_server_ip')
                            * l.P']:'
                            * l.Cg(number, 'postfix_server_port')
local postfix_ps_access_cg = l.Cg(postfix_ps_access_action, 'postfix_postscreen_access')
                           * l.P' '
                           * postfix_client_info_cg
local postfix_ps_noqueue_cg =postfix_smtpd_noqueue_cg
local postfix_ps_toobusy_cg = l.P'NOQUEUE: reject: CONNECT from '
                            * postfix_client_info_cg
                            * l.P': '
                            * l.Cg(l.P(1)^1, 'postfix_postscreen_toobusy_data')
local postfix_ps_dnsbl_cg = postfix_ps_violation_cg
                          * l.P' rank '
                          * l.Cg(integer, 'postfix_postscreen_dnsbl_rank')
                          * l.P' for '
                          * postfix_client_info_cg
local postfix_ps_cache_cg = l.P'cache '
                          * (l.P(1)-l.P' ')^1
                          * l.P' full cleanup: retained='
                          * l.Cg(integer, 'postfix_postscreen_cache_retained')
                          * l.P' dropped='
                          * l.Cg(integer, 'postfix_postscreen_cache_dropped')
                          * l.P' entries'
local postfix_ps_violations_cg = postfix_ps_violation_cg
                               * (l.P' ' * l.digit^1)^-1
                               * (l.P' after ' * l.Cg(number, 'postfix_postscreen_violation_time'))^-1
                               * l.P' from '
                               * postfix_client_info_cg
                               * (l.P' after ' * postfix_smtp_stage_cg)^-1

-- dnsblog patterns
local postfix_dnsblog_listing_cg = l.P'addr '
                                 * l.Cg(ipv46, 'postfix_client_ip')
                                 * l.P' listed by domain '
                                 * l.Cg(host, 'postfix_dnsbl_domain')
                                 * l.P' as '
                                 * l.Cg(ipv46, 'postfix_dnsbl_result')

-- tlsproxy patterns
local postfix_tlsproxy_conn_cg = (l.P'DISCONNECT'
                               + l.P'CONNECT')
                               * (l.P' from')^-1
                               * l.P' '
                               * postfix_client_info_cg

-- anvil patterns
local postfix_service_cg = (l.Cg((l.P(1)-l.S':')^1, 'postfix_service')
                         * l.P':'
                         * l.Cg(ipv46, 'postfix_client_ip'))
                         + (l.Cg((l.P(1)-l.S':')^1 * l.P':' * l.digit^1, 'postfix_service')
                         * l.P':'
                         * l.Cg(ipv46, 'postfix_client_ip'))

local postfix_anvil_conn_rate_cg = l.P'statistics: max connection rate '
                                 * l.Cg(integer, 'postfix_anvil_conn_rate')
                                 * l.P'/'
                                 * l.Cg(postfix_time_unit, 'postfix_anvil_conn_period')
                                 * l.P' for ('
                                 * postfix_service_cg
                                 * l.P') at '
                                 * l.Cg(l.P(1)^1, 'postfix_anvil_timestamp')
local postfix_anvil_conn_cache_cg = l.P'statistics: max cache size '
                                  * l.Cg(integer, 'postfix_anvil_cache_size')
                                  * l.P' at '
                                  * l.Cg(l.P(1)^1, 'postfix_anvil_timestamp')
local postfix_anvil_conn_count_cg = l.P'statistics: max connection count '
                                  * l.Cg(integer, 'postfix_anvil_conn_count')
                                  * l.P' for ('
                                  * postfix_service_cg
                                  * l.P') at '
                                  * l.Cg(l.P(1)^1, 'postfix_anvil_timestamp')

-- smtp patterns
local postfix_smtp_delivery_cg = postfix_queueid_cg
                               * l.P': '
                               * l.Cg((l.P(1) - l.P' status=')^1, 'postfix_keyvalue_data')
                               * l.P' status='
                               * l.Cg(l.alpha^1, 'postfix_status')
                               * (l.P' (' * l.Cg((l.P(1) - (l.P')' * -1))^1, 'postfix_smtp_response') * l.P')')^-1
local postfix_smtp_connerr_cg = l.P'connect to '
                              * postfix_relay_info_cg
                              * l.P': '
                              * (l.P'Connection timed out' + l.P'No route to host' + l.P'Connection refused')
local postfix_smtp_lostconn_cg = postfix_queueid_cg
                               * l.P': '
                               * postfix_lostconn
                               * l.P' with '
                               * postfix_relay_info_cg

-- master patterns
local postfix_master_start_cg = (l.P'daemon started' + l.P'reload')
                              * l.P' -- version '
                              * l.Cg((l.P(1)-l.P',')^1, 'postfix_version')
                              * l.P', configuration '
                              * l.Cg(l.P(1)^1, 'postfix_config_path')
local postfix_master_exit_cg = l.P'terminating on signal '
                             * l.Cg(integer, 'postfix_termination_signal')

-- bounce patterns
local postfix_bounce_notification_cg = postfix_queueid_cg
                                     * l.P': '
                                     * l.P'sender '
                                     * (l.P'non-delivery' + l.P'delivery status' + l.P'delay')
                                     * l.P' notification: '
                                     * l.Cg(postfix_queueid, 'postfix_bounce_queueid')

-- scache patterns
local postfix_scache_lookups_cg = l.P'statistics: '
                                * (l.P'address' + l.P'domain')
                                * l.P' lookup hits='
                                * l.Cg(integer, 'postfix_scache_hits')
                                * l.P' miss='
                                * l.Cg(integer,  'postfix_scache_miss')
                                * l.P' success='
                                * l.Cg(integer, 'postfix_scache_success')
local postfix_scache_simultaneous_cg = l.P'statistics: max simultaneous domains='
                                     * l.Cg(integer, 'postfix_scache_domains')
                                     * l.P' addresses='
                                     * l.Cg(integer, 'postfix_scache_addresses')
                                     * l.P' connection='
                                     * l.Cg(integer, 'postfix_scache_connection')
local postfix_scache_timestamp_cg = l.P'statistics: start interval '
                                  * l.Cg(l.P(1)^1, 'postfix_scache_timestamp')

-- aggregate all patterns
local postfix_patterns = {
  smtpd = l.Ct(postfix_smtpd_connect_cg
        + postfix_smtpd_disconnect_cg
        + postfix_smtpd_lostconn_cg
        + postfix_smtpd_noqueue_cg
        + postfix_smtpd_pipelining_cg
        + postfix_tlsconn_cg
        + postfix_warning_cg
        + postfix_smtpd_proxy_cg
        + postfix_keyvalue_greedy_cg),
  cleanup =  l.Ct(postfix_cleanup_milter_cg
          + postfix_warning_cg
          + postfix_keyvalue_greedy_cg),
  qmgr = l.Ct(postfix_qmgr_removed_cg
       + postfix_qmgr_active_cg
       + postfix_warning_cg),
  pipe = l.Ct(postfix_pipe_delivered_cg
       + postfix_pipe_forward_cg),
  postscreen = l.Ct(postfix_ps_connect_cg
             + postfix_ps_access_cg
             + postfix_ps_noqueue_cg
             + postfix_ps_toobusy_cg
             + postfix_ps_cache_cg
             + postfix_ps_dnsbl_cg
             + postfix_ps_violations_cg
             + postfix_warning_cg),
  dnsblog = l.Ct(postfix_dnsblog_listing_cg),
  anvil = l.Ct(postfix_anvil_conn_rate_cg
        + postfix_anvil_conn_cache_cg
        + postfix_anvil_conn_count_cg),
  smtp = l.Ct(postfix_smtp_delivery_cg
       + postfix_smtp_connerr_cg
       + postfix_smtp_lostconn_cg
       + postfix_tlsconn_cg
       + postfix_warning_cg),
  discard = l.Ct(postfix_queueid_cg
          * l.P': '
          * l.Cg((l.P(1) - l.P' status=')^1, 'postfix_keyvalue_data')
          * l.P' status='
          * l.Cg(l.alnum^1, 'postfix_status')),
  -- lmtp = smtp,
  pickup = l.Ct(postfix_queueid_cg
         * l.P': uid='
         * l.Cg((l.P(1) - l.P' from=')^1, 'postfix_uid')
         * l.P' from=<'
         * l.Cg((l.P(1) - l.P'>')^1, 'postfix_from')
         * (l.P'> orig_id='
           * l.Cg(l.P(1)^1, 'postfix_orig_id'))^-1
         ),
  tlsproxy = l.Ct(postfix_tlsproxy_conn_cg),
  master = l.Ct(postfix_master_start_cg
         + postfix_master_exit_cg),
  bounce = l.Ct(postfix_bounce_notification_cg),
  sendmail = l.Ct(postfix_warning_cg),
  postdrop = l.Ct(postfix_warning_cg),
  scache = l.Ct(postfix_scache_lookups_cg
         + postfix_scache_simultaneous_cg
         + postfix_scache_timestamp_cg),
  trivial_rewrite = l.Ct(postfix_warning_cg),
  tlsmgr = l.Ct(postfix_warning_cg),
  ['local'] = l.Ct(postfix_keyvalue_greedy_cg),
}
postfix_patterns['lmtp'] = postfix_patterns['smtp']

--local name = l.C(l.alpha^1) * space
--local value = l.C((l.P(1) - l.P',')^1)
--local sep = l.S(',;') * space
--local pair = l.Cg(name * '=' * space * value) * sep^-1
--local queueid = lpeg.Cf(lpeg.Ct'' * l.Cg(lpeg.Cc'queueid' * l.C(l.xdigit ^ 1)) * l.P': ', rawset)
--local postfix_grammar = l.Cf(queueid * pair^0, rawset)



local postfix_programname = (l.P(1) - l.P'/')^1
                          * '/'
                          * l.Cg(l.P(1)^1)

local integer_fields = {
  'postfix_anvil_cache_size',
  'postfix_anvil_conn_count',
  'postfix_anvil_conn_rate',
  'postfix_client_port',
  'postfix_nrcpt',
  'postfix_postscreen_cache_dropped',
  'postfix_postscreen_cache_retained',
  'postfix_postscreen_dnsbl_rank',
  'postfix_relay_port',
  'postfix_server_port',
  'postfix_size',
  'postfix_status_code',
  'postfix_termination_signal',
  'postfix_uid',
}
local float_fields = {
  'postfix_delay',
  --'postfix_delay_before_qmgr',
  --'postfix_delay_conn_setup',
  --'postfix_delay_in_qmgr',
  --'postfix_delay_transmission',
  --'postfix_postscreen_violation_time',
}


function postfix_match(programname, message, extract_keyvalue_data)
  local daemon_name = postfix_programname:match(programname)
  local ret = nil
  if not daemon_name then
    return nil
  end
  if postfix_patterns[daemon_name] then
    ret = postfix_patterns[daemon_name]:match(message)
  -- below are fake patterns used by tests
  elseif daemon_name == 'relay_info' then
    ret = postfix_relay_info_ct:match(message)
  elseif daemon_name == 'delays' then
    ret = postfix_delays_ct:match(message)
  elseif daemon_name == 'keyvalue_data' then
    ret = kv_grammar:match(message)
  end
  -- extract postfix_keyvalue_data
  if extract_keyvalue_data and ret and ret.postfix_keyvalue_data then
    local kv = kv_grammar:match(ret.postfix_keyvalue_data)
    if kv then
      ret.postfix_keyvalue_data = nil
      for k,v in pairs(kv) do
        ret[string.format('postfix_%s', k)] = v
      end
      if ret.postfix_client then
        local cl = postfix_client_info_ct:match(ret.postfix_client)
        if cl then
          ret.postfix_client = nil
          for k2,v2 in pairs(cl) do
            ret[k2] = v2
          end
        end
      end
      if ret.postfix_relay then
        local relay = postfix_relay_info_ct:match(ret.postfix_relay)
        if relay then
          ret.postfix_relay = nil
          for k2,v2 in pairs(relay) do
            ret[k2] = v2
          end
        end
      end
      if ret.postfix_delays then
        local delays = postfix_delays_ct:match(ret.postfix_delays)
        if delays then
          ret.postfix_delays = nil
          for k2,v2 in pairs(delays) do
            ret[k2] = v2
          end
        end
      end
    end
  end
  if ret then
    for i, v in ipairs(integer_fields) do
      if ret[v] then
        local value = ret[v]
        if type(value) ==  'table' then
          value = value.value
        end
        ret[v] = {
          value = tonumber(value),
          value_type = 2,
        }
      end
    end
    for i, v in ipairs(float_fields) do
      if ret[v] then
        local value = ret[v]
        if type(value) ==  'table' then
          value = value.value
        end
        ret[v] = tonumber(value)
      end
    end
  end
  return ret
end

return M
