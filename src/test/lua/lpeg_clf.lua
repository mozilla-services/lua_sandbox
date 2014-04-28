-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local clf = require "common_log_format"
require "string"

function process(tc)
    if tc == 0 then
        local tests = { -- format, input, result
             {'"$arg_generic"'          ,'"test item"'                  ,"test item"}
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
            output("ok")
    elseif tc == 1 then
        -- common log format
        local grammar = clf.build_nginx_grammar('$remote_addr - $remote_user [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        local log = '127.0.0.1 - - [10/Feb/2014:08:46:41 -0800] "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0"'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 2 then
        -- custom format to test the msec changes
        local grammar = clf.build_nginx_grammar('$msec $remote_addr "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        local log = '1391794831.755 127.0.0.1 "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0" "-" 0.000'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 3 then
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
        }
        local results = {
             {"Chrome"  , 32    , "Windows 8"}
            ,{"MSIE"    , 10    , "Windows 7"}
            ,{"Firefox" , 29    , "Windows 7"}
            ,{"Firefox" , 29    , "Macintosh"}
            ,{"Firefox" , 29    , "FirefoxOS"}
            ,{nil       , nil   , nil}
            ,{nil       , nil   , nil}
            ,{nil       , nil   , "iPad"}
            ,{nil       , nil   , nil}
            ,{nil       , nil   , nil}
            ,{nil       , nil   , nil}
        }

        for i, v in ipairs(user_agents) do
            local browser, version, os = clf.normalize_user_agent(v)
            if results[i][1] ~= browser or results[i][2] ~= version  or results[i][3] ~= os then
                output("user_agent: ", i, " browser: ", browser, " version: ", version, "  os: ", os,
                       " expected browser: ", results[i][1], " version: ", results[i][2], " os: ", results[i][3], "\n")
            end
        end
        output("ok")
    elseif tc == 4 then
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
            ,{"%T",  "request_time"          ,"1"                            ,{1,"s"}}
            ,{"%u", "remote_user"          ,"remote"                       ,"remote"}
            ,{"%U", "uri"                  ,"uri"                          ,"uri"}
            ,{"%v", "server_name"          ,"example.com"                  ,{"example.com","hostname"}}
            ,{"%V", "server_name"          ,"example.com"                  ,{"example.com","hostname"}}
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
            }

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
            output("ok")
    elseif tc == 5 then
        -- test each of the unique grammars
        local grammar = clf.build_apache_grammar('%% %a %B %b %D %f %k %p %P %s %t %T %v %X %I %O %S %A')
        local log = '% 127.0.0.1 235 235 204 test.txt 2 80 1234 404 [20/Mar/2014:08:56:26 -0700] 0 example.com + 311 498 809 ::1'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 6 then
        -- common log format
        local grammar = clf.build_apache_grammar('%h %l %u %t \"%r\" %>s %O')
        local log = '127.0.0.1 - - [10/Feb/2014:08:46:41 -0800] "GET / HTTP/1.1" 304 0'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 7 then
        -- vhost_combined log format
        local grammar = clf.build_apache_grammar('%v:%p %h %l %u %t \"%r\" %>s %O \"%{Referer}i\" \"%{User-Agent}i\"')
        local log = '127.0.1.1:80 127.0.0.1 - - [20/Mar/2014:12:38:34 -0700] "GET / HTTP/1.1" 404 492 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:27.0) Gecko/20100101 Firefox/27.0"'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 8 then
        -- combined log format
        local grammar = clf.build_apache_grammar('%h %l %u %t \"%r\" %>s %O \"%{Referer}i\" \"%{User-Agent}i\"')
        local log = '127.0.0.1 - - [20/Mar/2014:12:39:57 -0700] "GET / HTTP/1.1" 404 492 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:27.0) Gecko/20100101 Firefox/27.0"'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 9 then
        -- referer log format
        local grammar = clf.build_apache_grammar('%{Referer}i -> %U')
        local log = '- -> /'
        local fields = grammar:match(log)
        output(fields)
    elseif tc == 10 then
        local log = '2014/03/01 11:29:39 [notice] 16842#0: using inherited sockets from "6;"'
        local fields = clf.nginx_error_grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 11 then
        -- optional connection
        local log = '2014/03/01 11:29:39 [notice] 16842#0: 8878 using inherited sockets from "6;"'
        local fields = clf.nginx_error_grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    end

    write()
    return 0
end
