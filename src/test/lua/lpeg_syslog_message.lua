-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local syslog_message = require("syslog_message")

local function CRON()
    local grammar = syslog_message.get_prog_grammar('CRON')
    local log
    local fields

    log = '(root) CMD ( cd / && run-parts --report /etc/cron.hourly)'
    fields = grammar:match(log)
    assert(fields.cron_username == 'root', fields.cron_username)
    assert(fields.cron_event == 'CMD', fields.cron_event)
    assert(fields.cron_detail == ' cd / && run-parts --report /etc/cron.hourly', fields.cron_detail)

end

local function crontab()
    local grammar = syslog_message.get_prog_grammar('crontab')
    local log
    local fields

    log = '(root) LIST (root)'
    fields = grammar:match(log)
    assert(fields.cron_username == 'root', fields.cron_username)
    assert(fields.cron_event == 'LIST', fields.cron_event)
    assert(fields.cron_detail == 'root', fields.cron_detail)
end

local function dhclient()
    local grammar = syslog_message.get_prog_grammar('dhclient')
    local log
    local fields

    log = 'DHCPDISCOVER on eth0 to 10.20.30.42 port 67 interval 18000'
    fields = grammar:match(log)
    assert(fields.dhcp_type == 'DHCPDISCOVER', fields.dhcp_type)
    assert(fields.dhcp_client_interface == 'eth0', fields.dhcdhcp_client_interfacep_source)
    assert(fields.dhcp_server_addr.value == '10.20.30.42', fields.dhcp_server_addr)
    assert(fields.dhcp_server_addr.representation == 'ipv4', fields.dhcp_server_addr)
    assert(fields.dhcp_server_port == 67, fields.dhcp_server_port)
    assert(fields.dhcp_client_interval_seconds == 18000, fields.dhcp_client_interval_seconds)
    

    log = 'DHCPREQUEST on eth0 to 10.20.30.42 port 67'
    fields = grammar:match(log)
    assert(fields.dhcp_type == 'DHCPREQUEST', fields.dhcp_type)
    assert(fields.dhcp_client_interface == 'eth0', fields.dhcdhcp_client_interfacep_source)
    assert(fields.dhcp_server_addr.value == '10.20.30.42', fields.dhcp_server_addr)
    assert(fields.dhcp_server_addr.representation == 'ipv4', fields.dhcp_server_addr)
    assert(fields.dhcp_server_port == 67, fields.dhcp_server_port)

    log = 'DHCPACK from 10.20.30.42'
    fields = grammar:match(log)
    assert(fields.dhcp_type == 'DHCPACK', fields.dhcp_type)
    assert(fields.dhcp_server_addr.value == '10.20.30.42', fields.dhcp_server_addr)
    assert(fields.dhcp_server_addr.representation == 'ipv4', fields.dhcp_server_addr)

    log = 'bound to 10.20.30.40 -- renewal in 20346 seconds.'
    fields = grammar:match(log)
    assert(fields.dhcp_client_addr.value == '10.20.30.40', fields.dhcp_client_addr)
    assert(fields.dhcp_client_addr.representation == 'ipv4', fields.dhcp_client_addr)
    assert(fields.dhcp_client_renewal_seconds == 20346, fields.dhcp_client_renewal_seconds)
end

local function dhcpd()
    local grammar = syslog_message.get_prog_grammar('dhcpd')
    local log
    local fields

    log = 'DHCPINFORM from 192.168.0.45 via 192.168.0.1'
    fields = grammar:match(log)
    assert(fields.dhcp_type == 'DHCPINFORM', fields.dhcp_type)
    assert(fields.dhcp_client_addr.value == '192.168.0.45', fields.dhcp_client_addr)
    assert(fields.dhcp_client_addr.representation == 'ipv4', fields.dhcp_client_addr)
    assert(fields.dhcp_source == '192.168.0.1', fields.dhcp_source)

    log = 'DHCPDISCOVER from aa:bb:cc:dd:ee:ff via 10.2.3.4: unknown network segment'
    fields = grammar:match(log)
    assert(fields.dhcp_type == 'DHCPDISCOVER', fields.dhcp_type)
    assert(fields.dhcp_client_hw_addr == 'aa:bb:cc:dd:ee:ff', fields.dhcp_client_hw_addr)
    assert(fields.dhcp_source == '10.2.3.4', fields.dhcp_source)
    assert(fields.dhcp_message == 'unknown network segment', fields.dhcp_message)

    log = 'DHCPACK to 192.168.2.3 (aa:bb:cc:dd:ee:ff) via vlan42'
    fields = grammar:match(log)
    assert(fields.dhcp_type == 'DHCPACK', fields.dhcp_type)
    assert(fields.dhcp_client_hw_addr == 'aa:bb:cc:dd:ee:ff', fields.dhcp_client_hw_addr)
    assert(fields.dhcp_source == 'vlan42', fields.dhcp_source)
end

local function login()
    local grammar = syslog_message.get_prog_grammar('login')
    local log
    local fields

    log = "FAILED LOGIN (1) on '/dev/tty1' FOR 'root', Authentication failure"
    fields = grammar:match(log)
    assert(fields.failcount == 1, fields.failcount)
    assert(fields.tty == '/dev/tty1', fields.tty)
    assert(fields.user == 'root', fields.user)
    assert(fields.pam_error == 'Authentication failure', fields.pam_error)
end

local function named()
    local grammar = syslog_message.get_prog_grammar('named')
    local log
    local fields

    log = "lame server resolving '40.30.20.10.in-addr.arpa' (in '30.20.10.in-addr.arpa'?): 10.11.12.13#53"
    fields = grammar:match(log)
    assert(fields.dns_error == 'lame server', fields.dns_error)
    assert(fields.dns_name == '40.30.20.10.in-addr.arpa', fields.dns_name)
    assert(fields.dns_domain == '30.20.10.in-addr.arpa', fields.dns_domain)
    assert(fields.dns_addr.value == '10.11.12.13', fields.dns_addr.value)
    assert(fields.dns_addr.representation == 'ipv4', fields.dns_addr.representation)
    assert(fields.dns_port == 53, fields.dns_port)

    log = "host unreachable resolving 'ipv6.example.org/AAAA/IN': 2001:503:231d::2:30#53"
    fields = grammar:match(log)
    assert(fields.dns_error == 'host unreachable', fields.dns_error)
    assert(fields.dns_name == 'ipv6.example.org', fields.dns_name)
    assert(fields.dns_type == 'AAAA', fields.dns_type)
    assert(fields.dns_class == 'IN', fields.dns_class)
    assert(fields.dns_addr.value == '2001:503:231d::2:30', fields.dns_addr.value)
    assert(fields.dns_addr.representation == 'ipv6', fields.dns_addr.representation)
    assert(fields.dns_port == 53, fields.dns_port)

    log = "DNS format error from 134.170.107.24#53 resolving cid-ff58b408a75804a8.users.storage.live.com/AAAA for client 10.2.3.4#60466: Name storage.live.com (SOA) not subdomain of zone users.storage.live.com -- invalid response"
    fields = grammar:match(log)
    assert(fields.dns_error == 'DNS format error', fields.dns_error)
    assert(fields.dns_addr.value == '134.170.107.24', fields.dns_addr.value)
    assert(fields.dns_addr.representation == 'ipv4', fields.dns_addr.representation)
    assert(fields.dns_port == 53, fields.dns_port)
    assert(fields.dns_name == 'cid-ff58b408a75804a8.users.storage.live.com', fields.dns_name)
    assert(fields.dns_type == 'AAAA', fields.dns_type)
    assert(fields.dns_client_addr.value == '10.2.3.4', fields.dns_client_addr.value)
    assert(fields.dns_client_addr.representation == 'ipv4', fields.dns_client_addr.representation)
    assert(fields.dns_client_port == 60466, fields.dns_client_port)
    assert(fields.dns_message == "Name storage.live.com (SOA) not subdomain of zone users.storage.live.com -- invalid response", fields.dns_message)

    log = 'DNS format error from 184.105.66.196#53 resolving ns-os1.qq.com/AAAA: Name qq.com (SOA) not subdomain of zone ns-os1.qq.com -- invalid response'
    fields = grammar:match(log)
    assert(fields.dns_error == 'DNS format error', fields.dns_error)
    assert(fields.dns_addr.value == '184.105.66.196', fields.dns_addr.value)
    assert(fields.dns_addr.representation == 'ipv4', fields.dns_addr.representation)
    assert(fields.dns_port == 53, fields.dns_port)
    assert(fields.dns_name == 'ns-os1.qq.com', fields.dns_name)
    assert(fields.dns_type == 'AAAA', fields.dns_type)
    assert(fields.dns_message == "Name qq.com (SOA) not subdomain of zone ns-os1.qq.com -- invalid response", fields.dns_message)

    log = "client 10.8.6.1#17069/key trusty (pc.example.org): view internal: transfer of 'pc.example.org/IN': IXFR started: TSIG trusty (serial 12 -> 14)"
    fields = grammar:match(log)
    assert(fields.dns_client_addr.value == '10.8.6.1', fields.dns_client_addr.value)
    assert(fields.dns_client_addr.representation == 'ipv4', fields.dns_client_addr.representation)
    assert(fields.dns_client_port == 17069, fields.dns_client_port)
    assert(fields.dns_client_signer == 'trusty', fields.dns_client_signer)
    assert(fields.dns_name == 'pc.example.org', fields.dns_name)
    --assert(fields.dns_view == 'internal', fields.dns_view)
    assert(fields.dns_message == "transfer of 'pc.example.org/IN': IXFR started: TSIG trusty (serial 12 -> 14)", fields.dns_message)

    log = "success resolving 'ns1.example.com/AAAA' (in 'example.com'?) after disabling EDNS"
    fields = grammar:match(log)
    assert(fields.dns_name == 'ns1.example.com', fields.dns_name)
    assert(fields.dns_type == 'AAAA', fields.dns_type)
    assert(fields.dns_domain == 'example.com', fields.dns_domain)
    assert(fields.dns_message == "disabling EDNS", fields.dns_message)

    log = "zone 12.11.10.in-addr.arpa/IN/internal: sending notifies (serial 42)"
    fields = grammar:match(log)
    assert(fields.dns_domain == '12.11.10.in-addr.arpa', fields.dns_domain)
    assert(fields.dns_class == 'IN', fields.dns_class)
    assert(fields.dns_view == 'internal', fields.dns_view)
    assert(fields.dns_message == "sending notifies", fields.dns_message)
    assert(fields.dns_serial == 42, fields.dns_serial)

    log = "clients-per-query decreased to 22"
    fields = grammar:match(log)
    assert(fields.dns_clients_per_query == 22, fields.dns_clients_per_query)
end

local function puppet_agent()
    local grammar = syslog_message.get_prog_grammar('puppet-agent')
    local log
    local fields

    log = '(/Stage[main]/Nantes::Profile::Heka_base/Exec[setcap CAP_NET_BIND_SERVICE=+eip /usr/bin/hekad]/returns) executed successfully'
    fields = grammar:match(log)
    assert(fields.puppet_resource_path == '/Stage[main]/Nantes::Profile::Heka_base/Exec[setcap CAP_NET_BIND_SERVICE=+eip /usr/bin/hekad]', fields.puppet_resource_path)
    assert(fields.puppet_parameter == 'returns', fields.puppet_parameter)
    assert(fields.puppet_change == 'executed successfully', fields.puppet_change)

    log = "(/Stage[main]/Logstash::Service/Logstash::Service::Init[logstash]/Service[logstash]/ensure) ensure changed 'stopped' to 'running'"
    fields = grammar:match(log)
    assert(fields.puppet_resource_path == '/Stage[main]/Logstash::Service/Logstash::Service::Init[logstash]/Service[logstash]', fields.puppet_resource_path)
    assert(fields.puppet_parameter == 'ensure', fields.puppet_parameter)
    assert(fields.puppet_old_value == 'stopped', fields.puppet_old_value)
    assert(fields.puppet_new_value == 'running', fields.puppet_new_value)

    log = '(/Stage[main]/Elasticsearch::Config/File[/var/log/elasticsearch/test.log]/owner) current_value root, should be elasticsearch (noop)'
    fields = grammar:match(log)
    assert(fields.puppet_resource_path == '/Stage[main]/Elasticsearch::Config/File[/var/log/elasticsearch/test.log]', fields.puppet_resource_path)
    assert(fields.puppet_parameter == 'owner', fields.puppet_parameter)
    assert(fields.puppet_current_value == 'root', fields.puppet_current_value)
    assert(fields.puppet_should_value == 'elasticsearch', fields.puppet_should_value)
    assert(fields.puppet_noop == true, fields.puppet_noop)

    log = '(/Stage[main]/Nantes::Profile::Heka_base/File[/etc/heka/conf.d/10_local_syslog.toml]/content)'
    fields = grammar:match(log)
    assert(fields.puppet_resource_path == '/Stage[main]/Nantes::Profile::Heka_base/File[/etc/heka/conf.d/10_local_syslog.toml]', fields.puppet_resource_path)
    assert(fields.puppet_parameter == 'content', fields.puppet_parameter)

    log = '(/Stage[main]/Nantes::Profile::Heka_base/File[/etc/heka/conf.d/10_net_syslog.toml]) Scheduling refresh of Service[heka]'
    fields = grammar:match(log)
    assert(fields.puppet_resource_path == '/Stage[main]/Nantes::Profile::Heka_base/File[/etc/heka/conf.d/10_net_syslog.toml]', fields.puppet_resource_path)
    assert(fields.puppet_callback == 'refresh', fields.puppet_callback)
    assert(fields.puppet_callback_target == 'Service[heka]', fields.puppet_callback_target)

    log = "(Class[Logstash::Service]) Would have triggered 'refresh' from 1 events"
    fields = grammar:match(log)
    assert(fields.puppet_resource_path == 'Class[Logstash::Service]', fields.puppet_resource_path)
    assert(fields.puppet_msg == 'Would have triggered', fields.puppet_msg)
    assert(fields.puppet_callback == 'refresh', fields.puppet_callback)
    assert(fields.puppet_events_count == 1, fields.puppet_events_count)

    log = '(/Stage[main]/Nantes::Profile::Heka_base/File[/etc/heka/conf.d/10_net_syslog.toml]) Filebucketed /etc/heka/conf.d/10_net_syslog.toml to main with sum 70358a826b06f61f36bdc6aecaa3db14'
    fields = grammar:match(log)
    assert(fields.puppet_resource_path == '/Stage[main]/Nantes::Profile::Heka_base/File[/etc/heka/conf.d/10_net_syslog.toml]', fields.puppet_resource_path)
    assert(fields.puppet_file_path == '/etc/heka/conf.d/10_net_syslog.toml', fields.puppet_file_path)
    assert(fields.puppet_bucket == 'main', fields.puppet_bucket)
    assert(fields.puppet_file_sum == '70358a826b06f61f36bdc6aecaa3db14', fields.puppet_file_sum)

    log = 'Finished catalog run in 7.11 seconds'
    fields = grammar:match(log)
    assert(fields.puppet_msg == 'Finished catalog run', fields.puppet_msg)
    assert(fields.puppet_benchmark_seconds == 7.11, fields.puppet_benchmark_seconds)
end

local function sshd()
    local grammar = syslog_message.get_prog_grammar('sshd')
    local log
    local fields

    log = 'Accepted publickey for sathieu from 10.11.12.13 port 4242 ssh2'
    fields = grammar:match(log)
    assert(fields.sshd_authmsg == 'Accepted', fields.sshd_authmsg)
    assert(fields.sshd_method == 'publickey', fields.sshd_method)
    assert(fields.remote_user == 'sathieu', fields.remote_user)
    assert(fields.remote_addr.value == '10.11.12.13', fields.remote_addr)
    assert(fields.remote_port == 4242, fields.remote_port)

    log = "Failed password for invalid user administrator from 10.20.30.40 port 4242 ssh2"
    fields = grammar:match(log)
    assert(fields.sshd_authmsg == 'Failed', fields.sshd_authmsg)
    assert(fields.sshd_method == 'password', fields.sshd_method)
    assert(fields.remote_user == 'administrator', fields.remote_user)
    assert(fields.remote_addr.value == '10.20.30.40', fields.remote_addr)
    assert(fields.remote_port == 4242, fields.remote_port)

    log = "Received disconnect from 10.2.3.4: 11: The user disconnected the application [preauth]"
    fields = grammar:match(log)
    assert(fields.remote_addr.value == '10.2.3.4', fields.remote_addr)
    assert(fields.disconnect_reason == 11, fields.disconnect_reason)
    assert(fields.disconnect_msg == 'The user disconnected the application [preauth]', fields.disconnect_msg)
end

local function sudo()
    local grammar = syslog_message.get_prog_grammar('sudo')
    local log
    local fields

    log = "usrnagios : TTY=unknown ; PWD=/home/usrnagios ; USER=root ; COMMAND=/usr/bin/ctdb -Y status"
    fields = grammar:match(log)
    assert(fields.sudo_message == 'usrnagios', fields.sudo_message)
    assert(fields.sudo_tty == 'unknown', fields.sudo_tty)
    assert(fields.sudo_pwd == '/home/usrnagios', fields.sudo_pwd)
    assert(fields.sudo_user == 'root', fields.sudo_user)
    assert(fields.sudo_command == '/usr/bin/ctdb -Y status', fields.sudo_command)
end

local function systemd_logind()
    local grammar = syslog_message.get_prog_grammar('systemd-logind')
    local log
    local fields

    log = "New session 42 of user sathieu."
    fields = grammar:match(log)
    assert(fields.sd_message == 'SESSION_START', fields.sd_message)
    assert(fields.session_id == 42, fields.session_id)
    assert(fields.user_id == 'sathieu', fields.user_id)

    log = "Removed session 42."
    fields = grammar:match(log)
    assert(fields.sd_message == 'SESSION_STOP', fields.sd_message)
    assert(fields.session_id == 42, fields.session_id)
end

local function pam()
    local grammar = syslog_message.get_wildcard_grammar('PAM')
    local log
    local fields

    log = "pam_unix(login:session): session opened for user sathieu by LOGIN(uid=1000)"
    fields = grammar:match(log)
    assert(fields.pam_module == 'pam_unix', fields.pam_module)
    assert(fields.pam_service == 'login', fields.pam_service)
    assert(fields.pam_type == 'session', fields.pam_type)
    assert(fields.pam_action == 'session opened', fields.pam_action)
    assert(fields.user_name == 'sathieu', fields.user_name)
    assert(fields.uid == 1000, fields.uid)

    log = "pam_unix(login:auth): authentication failure; logname=LOGIN uid=0 euid=0 tty=/dev/tty1 ruser= rhost=  user=root"
    fields = grammar:match(log)
    assert(fields.pam_module == 'pam_unix', fields.pam_module)
    assert(fields.pam_service == 'login', fields.pam_service)
    assert(fields.pam_type == 'auth', fields.pam_type)
    assert(fields.pam_action == 'authentication failure', fields.pam_action)
    assert(fields.logname == 'LOGIN', fields.logname)
    assert(fields.uid == 0, fields.uid)
    assert(fields.euid == 0, fields.euid)
    assert(fields.tty == '/dev/tty1', fields.tty)
    assert(fields.ruser == '', fields.ruser)
    assert(fields.rhost == '', fields.rhost)
    assert(fields.user == 'root', fields.user)

    log = "pam_unix(login:session): session closed for user sathieu"
    fields = grammar:match(log)
    assert(fields.pam_module == 'pam_unix', fields.pam_module)
    assert(fields.pam_service == 'login', fields.pam_service)
    assert(fields.pam_type == 'session', fields.pam_type)
    assert(fields.pam_action == 'session closed', fields.pam_action)
    assert(fields.user_name == 'sathieu', fields.user_name)
end

function process()
    CRON()
    crontab()
    dhclient()
    dhcpd()
    login()
    named()
    puppet_agent()
    sshd()
    sudo()
    systemd_logind()

    -- wildcard grammars
    pam()

    return 0
end

