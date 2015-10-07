-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local clf = require "common_log_format"
require "string"

local combined_log = '127.0.0.1 - - [10/Feb/2014:08:46:41 -0800] "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0"'

local combined_result = {
    remote_addr     = {value = "127.0.0.1", representation = "ipv4"},
    remote_user     = "-",
    time            = 1392050801000000000,
    request         = "GET / HTTP/1.1",
    status          = 304,
    body_bytes_sent = {value = 0, representation = "B"},
    http_referer    = "-",
    http_user_agent = "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0"
}

local function verify_result(fields, expected)
    if not fields then
        error("no result", 2)
    end

    for k, v in pairs(expected) do
        local f = fields[k]
        if not f then
            error(string.format("key: %s is not found in the result", k), 2)
        end

        if type(v) == "table" then
            if f.value ~= v.value then
                error(string.format("key: %s value: %s received:%s",
                                    k,
                                    tostring(v.value),
                                    tostring(f.value)),
                       2)
            end
            if f.representation ~= v.representation then
                error(string.format("key: %s representation: %s received: %s",
                                    k,
                                    tostring(v.representation),
                                    tostring(f.representation)),
                       2)
            end
        else
            if f ~= v then
                error(string.format("key: %s expected: %s received: %s",
                                    k,
                                    tostring(v),
                                    tostring(f)),
                      2)
            end
        end
    end
end

local function nginx_formats()
    local tests = { -- format, input, result
     {'"$arg_generic"'          ,'"test \\"item\\""'            ,'test \\"item\\"'}
    ,{"$binary_remote_addr"     ,"aaaa"                         ,"aaaa"}
    ,{"$body_bytes_sent"        ,"23"                           ,{23,"B"}}
    ,{"$bytes_sent"             ,"24"                           ,{24,"B"}}
    ,{"$connection"             ,"3"                            ,3}
    ,{"$connection_requests"    ,"0"                            ,0}
    ,{"$content_length"         ,"123"                          ,{123,"B"}}
    ,{"$host"                   ,"example.com"                  ,{"example.com","hostname"}}
    ,{"$hostname"               ,"::1"                          ,{"::1","ipv6"}}
    ,{"$https"                  ,"on"                           ,"on"}
    ,{"$https"                  ,""                             ,""}
    ,{"$is_args"                ,"?"                            ,"?"}
    ,{"$is_args"                ,""                             ,""}
    ,{"$msec"                   ,"1391794831.755"               ,1391794831755000000}
    ,{"$nginx_version"          ,"1.1.1"                        ,"1.1.1"}
    ,{"$pid"                    ,"99"                           ,99}
    ,{"$pipe"                   ,"p"                            ,"p"}
    ,{"$pipe"                   ,"."                            ,"."}
    ,{"$proxy_protocol_addr"    ,"127.0.0.1"                    ,{"127.0.0.1","ipv4"}}
    ,{"$remote_addr"            ,"127.0.0.1"                    ,{"127.0.0.1","ipv4"}}
    ,{"$remote_port"            ,"5435"                         ,5435}
    ,{"$request_completion"     ,"OK"                           ,"OK"}
    ,{"$request_completion"     ,""                             ,""}
    ,{"$request_length"         ,"23"                           ,{23,"B"}}
    ,{"$request_method"         ,"GET"                          ,"GET"}
    ,{"$request_time"           ,"1.123"                        ,{1.123,"s"}}
    ,{"$scheme"                 ,"http"                         ,"http"}
    ,{"$scheme"                 ,"https"                        ,"https"}
    ,{"$server_addr"            ,"127.0.0.1"                    ,{"127.0.0.1","ipv4"}}
    ,{"$server_name"            ,"example.com"                  ,{"example.com","hostname"}}
    ,{"$server_port"            ,"5435"                         ,5435}
    ,{"$server_protocol"        ,"HTTP/1.0"                     ,"HTTP/1.0"}
    ,{"$status"                 ,"200"                          ,200}
    ,{"$tcpinfo_rtt"            ,"200"                          ,200}
    ,{"$tcpinfo_rttvar"         ,"200"                          ,200}
    ,{"$tcpinfo_snd_cwnd"       ,"200"                          ,200}
    ,{"$tcpinfo_rcv_space"      ,"200"                          ,200}
    ,{"$time_iso8601"           ,"2014-02-10T08:46:41-08:00"    ,1392050801000000000}
    ,{"$time_local"             ,"10/Feb/2014:08:46:41 -0800"   ,1392050801000000000}
    }

    for i, v in ipairs(tests) do
        local grammar = clf.build_nginx_grammar(v[1])
        local fields = grammar:match(v[2])
        if not fields then
            error(string.format("test: %s failed to match: %s", v[1], v[2]))
        end

        local key
        if i == 1 then
            key = "arg_generic"
        else
            key = string.sub(v[1], 2)
        end
        if key == "msec" or key == "time_iso8601" or key == "time_local" then
            key = "time"
        end


        if type(v[3]) == "table" then
            if fields[key].value ~= v[3][1] then
                error(string.format("test: %s expected value: %s received: %s", v[1], v[3][1], fields[key].value))
            end
            if fields[key].representation ~= v[3][2] then
                error(string.format("test: %s expected representation: %s received: %s", v[1], v[3][2], fields[key].representation))
            end
        else
            if fields[key] ~= v[3] then
                error(string.format("test: %s expected value: %s received: %s", v[1], v[3], fields[key]))
            end
        end
    end
end

local function nginx_combined()
    local grammar = clf.build_nginx_grammar('$remote_addr - $remote_user [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
    local fields = grammar:match(combined_log)
    verify_result(fields, combined_result)
end

local function nginx_msec()
    local grammar = clf.build_nginx_grammar('$msec $remote_addr "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
    local log = '1391794831.755 127.0.0.1 "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0" "-" 0.000'
    local fields = grammar:match(log)
    local result = {
        remote_addr     = {value = "127.0.0.1", representation = "ipv4"},
        time            = 1391794831755000000,
        request         = "GET / HTTP/1.1",
        status          = 304,
        body_bytes_sent = {value = 0, representation = "B"},
        http_referer    = "-",
        http_user_agent = "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0"
    }
    verify_result(fields, result)
end

local function user_agent_normalization()
    local user_agents = {
         "Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1667.0 Safari/537.36"
        ,"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)"
        ,"Mozilla/5.0 (Windows NT 6.1; rv:29.0) Gecko/20100101 Firefox/29.0"
        ,"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:29.0) Gecko/20100101 Firefox/29.0"
        ,"Mozilla/5.0 (Mobile; rv:29.0) Gecko/20100101 Firefox/29.0"
        ,"curl/7.29.0"
        ,"Apache-HttpClient/UNAVAILABLE (java 1.5)"
        ,"Mozilla/5.0 (iPad; U; CPU OS 3_2_1 like Mac OS X; en-us) AppleWebKit/531.21.10 (KHTML, like Gecko) Mobile/7B405"
        ,"Googlebot/2.1 (+http://www.google.com/bot.html)"
        ,""
        ,"-"
        ,"Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0; MSIE; )" -- https://bugzilla.mozilla.org/show_bug.cgi?id=1009280
        ,"Firefox/29.0 FxSync/1.31.0.20140327113732.desktop" -- https://github.com/mozilla-services/puppet-config/issues/312
        ,"Mozilla/5.0 (iPad; CPU OS 6_0 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10A5355d Safari/8536.25"
        ,"Opera/9.80 (J2ME/MIDP; Opera Mini/9.80 (S60; SymbOS; Opera Mobi/23.348; U; en) Presto/2.5.25 Version/10.54"
        ,"Opera/12.02 (Android 4.1; Linux; Opera Mobi/ADR-1111101157; U; en-US) Presto/2.9.201 Version/12.02"
        ,"Opera/9.80 (Windows NT 6.0) Presto/2.12.388 Version/12.14"
    }
    local results = {
         {"Chrome"      , 32    , "Windows 8"}
        ,{"MSIE"        , 10    , "Windows 7"}
        ,{"Firefox"     , 29    , "Windows 7"}
        ,{"Firefox"     , 29    , "Macintosh"}
        ,{"Firefox"     , 29    , "FirefoxOS"}
        ,{nil           , nil   , nil}
        ,{nil           , nil   , nil}
        ,{nil           , nil   , "iPad"}
        ,{nil           , nil   , nil}
        ,{nil           , nil   , nil}
        ,{nil           , nil   , nil}
        ,{"MSIE"        , nil   , "Windows 7"}
        ,{"Firefox"     , 29    , nil}
        ,{"Safari"      , 8536  , "iPad"}
        ,{"Opera Mini"  , 10    , nil}
        ,{"Opera Mobi"  , 12    , "Android"}
        ,{"Opera"       , 12    , "Windows Vista"}
    }

    for i, v in ipairs(user_agents) do
        local browser, version, os = clf.normalize_user_agent(v)
        assert(results[i][1] == browser and results[i][2] == version and results[i][3] == os,
               string.format("user_agent: %d browser: %s version: %s os: %s expected browser: %s version: %s os: %s",
                             i, tostring(browser), tostring(version), tostring(os),
                             tostring(results[i][1]), tostring(results[i][2]), tostring(results[i][3])))
    end
end

local function apache_formats()
    local tests = { -- format, key, input, result
     {"%a", "remote_addr"          ,"127.0.0.1"                    ,{"127.0.0.1","ipv4"}}
    ,{"%A", "server_addr"          ,"127.0.0.1"                    ,{"127.0.0.1","ipv4"}}
    ,{"%B", "body_bytes_sent"      ,"23"                           ,{23,"B"}}
    ,{"%b", "body_bytes_sent"      ,"23"                           ,{23,"B"}}
    ,{"%b", "body_bytes_sent"      ,"-"                            ,{0,"B"}}
    ,{"%D", "request_time"         ,"123"                          ,{123,"us"}}
    ,{"%f", "request_filename"     ,"test.txt"                     ,"test.txt"}
    ,{"%h", "remote_addr"          ,"example.com"                  ,{"example.com","hostname"}}
    ,{"%H", "server_protocol"      ,"HTTP/1.0"                     ,"HTTP/1.0"}
    ,{"%k", "connection_requests"  ,"0"                            ,0}
    ,{"%l", "remote_user"          ,"waldo"                        ,"waldo"}
    ,{"%L", "request_log_id"       ,"???"                          ,"???"}
    ,{"%m", "request_method"       ,"GET"                          ,"GET"}
    ,{"%p", "server_port"          ,"5435"                         ,5435}
    ,{"%P", "pid"                  ,"99"                           ,99}
    ,{"%q", "query_string"         ,"query"                        ,"query"}
    ,{"%r", "request"              ,"request"                      ,"request"}
    ,{"%R", "request_handler"      ,"handler"                      ,"handler"}
    ,{"%s", "status"               ,"200"                          ,200}
    ,{"%t", "time"                 ,"[10/Feb/2014:08:46:41 -0800]" ,1392050801000000000}
    ,{"%T", "request_time"          ,"1"                            ,{1,"s"}}
    ,{"%u", "remote_user"          ,"remote"                       ,"remote"}
    ,{"%U", "uri"                  ,"uri"                          ,"uri"}
    ,{"%v", "server_name"          ,"example.com"                  ,{"example.com","hostname"}}
    ,{"%V", "server_name"          ,"example.com"                  ,{"example.com","hostname"}}
    ,{'\\"%V\\"', "server_name"    ,'"example.com"'                ,{"example.com","hostname"}}
    ,{"%X", "connection_status"    ,"X"                            ,"X"}
    ,{"%X", "connection_status"    ,"+"                            ,"+"}
    ,{"%X", "connection_status"    ,"-"                            ,"-"}
    ,{"%I", "request_length"       ,"23"                           ,{23,"B"}}
    ,{"%O", "response_length"      ,"23"                           ,{23,"B"}}
    ,{"%S", "total_length"         ,"23"                           ,{23,"B"}}

    ,{"%>s"                     ,"status"               ,"200"                          ,200}
    ,{"%{c}a"                   ,"remote_addr"          ,"127.0.0.1"                    ,{"127.0.0.1","ipv4"}}
    ,{"%{Test-item}C"           ,"cookie_Test-item"     ,"item"                         ,"item"}
    ,{"%{Test-item}e"           ,"env_Test-item"        ,"item"                         ,"item"}
    ,{'"%{User-agent}i"'        ,"http_user_agent"      ,'"Mozilla/5.0 (comment)"'      ,"Mozilla/5.0 (comment)"}
    ,{'"%400,501{User-agent}i"' ,"http_user_agent"      ,'"Mozilla/5.0 (comment)"'      ,"Mozilla/5.0 (comment)"}
    ,{'"%!400,501{User-agent}i"',"http_user_agent"      ,'"Mozilla/5.0 (comment)"'      ,"Mozilla/5.0 (comment)"}
    ,{"%{Test-item}n"           ,"module_Test-item"     ,"item"                         ,"item"}
    ,{"%{Test-item}o"           ,"sent_http_test_item"  ,"item"                         ,"item"}
    ,{"%{canonical}p"           ,"server_port"          ,"99"                           ,99}
    ,{"%{local}p"               ,"server_port"          ,"99"                           ,99}
    ,{"%{remote}p"              ,"remote_port"          ,"99"                           ,99}
    ,{"%{pid}P"                 ,"pid"                  ,"99"                           ,99}
    ,{"%{tid}P"                 ,"tid"                  ,"99"                           ,99}
    ,{"%{hextid}P"              ,"tid"                  ,"63"                           ,99}
    ,{"%{sec}t"                 ,"time"                 ,"1392050801"                   ,1392050801000000000}
    ,{"%{begin:sec}t"           ,"time"                 ,"1392050801"                   ,1392050801000000000}
    ,{"%{end:sec}t"             ,"time"                 ,"1392050801"                   ,1392050801000000000}
    ,{"%{msec}t"                ,"time"                 ,"1392050801000"                ,1392050801000000000}
    ,{"%{usec}t"                ,"time"                 ,"1392050801000000"             ,1392050801000000000}
    ,{"%{msec_frac}t"           ,"sec_frac"             ,"010"                          ,0.010}
    ,{"%{usec_frac}t"           ,"sec_frac"             ,"010000"                       ,0.010}
    ,{"%{%d/%b/%Y:%H:%M:%S %z}t","time"                 ,"10/Feb/2014:08:46:41 -0800"   ,1392050801000000000}
    ,{"%{%s}t"                  ,"time"                 ,"1392050801"                   ,1392050801000000000}
    ,{'"%r"'                    , "request"             ,'"test \\"item\\""'            ,'test \\"item\\"'}
    ,{'"%{Test-item}o"'         ,"sent_http_test_item"  ,'"test \\"item\\""'            ,'test \\"item\\"'}
    }

    if os.date("%c"):find("^%d") then -- windows
        tests[55][3] = "10/Feb/2014:08:46:41 PST"
    end

    for i, v in ipairs(tests) do
        local grammar = clf.build_apache_grammar(v[1])
        local fields = grammar:match(v[3])
        if not fields then
            error(string.format("test: %s failed to match: %s", v[1], v[3]))
        end
        local key = v[2]

        if type(v[4]) == "table" then
            if fields[key].value ~= v[4][1] then
                error(string.format("test: %s expected value: %s received: %s", v[1], v[4][1], fields[key].value))
            end
            if fields[key].representation ~= v[4][2] then
                error(string.format("test: %s expected representation: %s received: %s", v[1], v[4][2], fields[key].representation))
            end
        else
            if fields[key] ~= v[4] then
                error(string.format("test: %s expected value: %s received: %s", v[1], v[4], fields[key]))
            end
        end
    end
end

local function apache_custom()
    -- test each of the unique grammars
    local grammar = clf.build_apache_grammar('%% %a %B %b %D %f %k %p %P %s %t %T %v %X %I %O %S %A')
    local log = '% 127.0.0.1 235 235 204 test.txt 2 80 1234 404 [20/Mar/2014:08:56:26 -0700] 0 example.com + 311 498 809 ::1'
    local fields = grammar:match(log)
    local result = {
        server_addr         = {value = "::1", representation = "ipv6"},
        pid                 = 1234,
        total_length        = {value = 809, representation = "B"},
        server_port         = 80,
        request_length      = {value = 311, representation = "B"},
        connection_requests = 2,
        connection_status   = "+",
        body_bytes_sent     = {value = 235, representation = "B"},
        remote_addr         = {value = "127.0.0.1", representation = "ipv4"},
        time                = 1395330986000000000,
        response_length     = {value = 498, representation = "B"},
        request_time        = {value = 0, representation = "s"},
        request_filename    = "test.txt",
        status              = 404,
        server_name         = {value = "example.com", representation = "hostname"}
    }
    verify_result(fields, result)
end

local function apache_clf()
    local grammar = clf.build_apache_grammar('%h %l %u %t \"%r\" %>s %b')
    local log = '127.0.0.1 - - [10/Feb/2014:08:46:41 -0800] "GET / HTTP/1.1" 304 0'
    local fields = grammar:match(log)
    local result = {
        remote_addr     = {value = "127.0.0.1", representation = "ipv4"},
        remote_user     = "-",
        body_bytes_sent = {value = 0, representation = "B"},
        time            = 1392050801000000000,
        request         = "GET / HTTP/1.1",
        status          = 304,
    }
    verify_result(fields, result)
end

local function apache_vhost_combined()
    local grammar = clf.build_apache_grammar('%v:%p %h %l %u %t \"%r\" %>s %O \"%{Referer}i\" \"%{User-Agent}i\"')
    local log = '127.0.1.1:80 127.0.0.1 - - [20/Mar/2014:12:38:34 -0700] "GET / HTTP/1.1" 404 492 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:27.0) Gecko/20100101 Firefox/27.0"'
    local fields = grammar:match(log)
    local result = {
        remote_user         = "-",
        server_port         = 80,
        http_referer        = "-",
        remote_addr         = {value = "127.0.0.1", representation = "ipv4"},
        time                = 1395344314000000000,
        response_length     = {value = 492, representation = "B"},
        request             = "GET / HTTP/1.1",
        http_user_agent     = "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:27.0) Gecko/20100101 Firefox/27.0",
        status              = 404,
        server_name         = {value = "127.0.1.1", representation = "ipv4"}
    }
    verify_result(fields, result)
end

local function apache_combined()
    local grammar = clf.build_apache_grammar('%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\"')
    local fields = grammar:match(combined_log)
    verify_result(fields, combined_result)
end

local function apache_referer()
    local grammar = clf.build_apache_grammar('%{Referer}i -> %U')
    local log = '- -> /'
    local fields = grammar:match(log)
    local result = {
        uri = "/",
        http_referer = "-"
    }
    verify_result(fields, result)
end

local function nginx_error()
    local log = '2014/03/01 11:29:39 [notice] 16842#0: using inherited sockets from "6;"'
    local fields = clf.nginx_error_grammar:match(log)
    local result = {
        Pid         = 16842,
        Payload     = 'using inherited sockets from "6;"',
        Severity    = 5,
        Timestamp   = 1393673379000000000
    }
    verify_result(fields, result)
    assert(fields.Fields.tid == 0, "expected a thread id of 0")
end

local function nginx_error_connection()
    -- optional connection
    local log = '2014/03/01 11:29:39 [notice] 16842#0: *8878 using inherited sockets from "6;"'
    local fields = clf.nginx_error_grammar:match(log)
    local result = {
        Pid         = 16842,
        Payload     = 'using inherited sockets from "6;"',
        Severity    = 5,
        Timestamp   = 1393673379000000000
    }
    verify_result(fields, result)
    assert(fields.Fields.tid == 0, "invalid tid")
    assert(fields.Fields.connection == 8878, "invalid connection")
end

local function nginx_upstream()
    local addrs = {"192.168.1.1:80", "192.168.1.2:80", "unix:/tmp/sock", "192.168.10.1:80", "192.168.10.2:80"}
    local lengths = {1, 2, 3, 4, 5}
    local times = {1.1, 1.2, 1.3, 1.4, "-"}
    local statuses = {200, 201, 202, 203, "-"}
    local uscs = "HIT"
    local header = "header field"

    local grammar = clf.build_nginx_grammar('$upstream_addr $upstream_cache_status $upstream_response_length $upstream_response_time $upstream_status "$upstream_http_test" $upstream_cache_last_modified')
    local log = string.format('%s, %s, %s : %s, %s %s %d, %d, %d : %d, %d %g, %g, %g : %g, %s %d, %d, %d : %d, %s "%s" Mon, 28 Sep 1970 06:00:00 GMT',
                              addrs[1], addrs[2], addrs[3], addrs[4], addrs[5], uscs,
                              lengths[1], lengths[2], lengths[3], lengths[4], lengths[5],
                              times[1], times[2], times[3], times[4], times[5],
                              statuses[1], statuses[2], statuses[3], statuses[4], statuses[5], header)
    local fields = grammar:match(log)
    if not fields then error(string.format("failed match: %s", log)) end

    if #fields.upstream_addr ~= #addrs then
        error(string.format("#upstream_addr = %d", #fields.upstream_addr))
    end
    for i, v in ipairs(addrs) do
        if fields.upstream_addr[i] ~= v then
            error(string.format("expected value: '%s' received: '%s'", v, fields.upstream_addr[i]))
        end
    end

    if #fields.upstream_response_length.value ~= #lengths then
        error(string.format("#upstream_response_length = %d", #fields.upstream_response_length.value))
    end
    if fields.upstream_response_length.representation ~= "B" then
        error(string.format("upstream_response_length representation = '%s'", fields.upstream_response_length.representation))
    end
    for i, v in ipairs(lengths) do
        if fields.upstream_response_length.value[i] ~= v then
            error(string.format("expected value: %d received: %d", v, fields.upstream_response_length.value[i]))
        end
    end

    if #fields.upstream_response_time.value ~= #times then
        error(string.format("#upstream_response_time = %d", #fields.upstream_response_time.value))
    end
    if fields.upstream_response_time.representation ~= "s" then
        error(string.format("upstream_response_time representation = '%s'", fields.upstream_response_time.representation))
    end
    for i, v in ipairs(times) do
        if v == "-" then v = 0 end
        if fields.upstream_response_time.value[i] ~= v then
            error(string.format("expected value: %g received: %g", v, fields.upstream_response_time.value[i]))
        end
    end

    if #fields.upstream_status ~= #statuses then
        error(string.format("#upstream_status = %d", #fields.upstream_status))
    end
    for i, v in ipairs(statuses) do
        if v == "-" then v = 0 end
        if fields.upstream_status[i] ~= v then
            error(string.format("expected value: %d received: %d", v, fields.upstream_status[i]))
        end
    end

    if fields.upstream_cache_status ~= uscs then
        error(string.format("expected value: '%s' received: '%s'", uscs, fields.upstream_cache_status))
    end
    if fields.upstream_http_test ~= header then
        error(string.format("expected value: '%s' received: '%s'", header, fields.upstream_http_test))
    end
    local usclm = 2.33496e16
    if fields.upstream_cache_last_modified ~= usclm then
        error(string.format("expected value: '%s' received: '%s'", usclm, fields.upstream_cache_last_modified))
    end
end

local function nginx_upstream_defaults()
    local grammar = clf.build_nginx_grammar('$upstream_addr $upstream_cache_status $upstream_response_length $upstream_response_time $upstream_status "$upstream_http_test" $upstream_cache_last_modified')
    local log = '- - - - - "-" -'
    local fields = grammar:match(log)
    if not fields then error(string.format("failed match: %s", log)) end
end


function process()
    nginx_formats()
    nginx_combined()
    nginx_msec()
    user_agent_normalization()
    apache_formats()
    apache_custom()
    apache_clf()
    apache_vhost_combined()
    apache_combined()
    apache_referer()
    nginx_error()
    nginx_error_connection()
    nginx_upstream()
    nginx_upstream_defaults()

    return 0
end
