-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local clf = require "common_log_format"
require "string"

function process(tc)
    if tc == 0 then
        -- common log format
        local grammar = clf.build_nginx_grammar('$remote_addr - $remote_user [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        local log = '127.0.0.1 - - [10/Feb/2014:08:46:41 -0800] "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0"'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 1 then
        -- custom format to test the msec changes
        local grammar = clf.build_nginx_grammar('$msec $remote_addr "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        local log = '1391794831.755 127.0.0.1 "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0" "-" 0.000'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 2 then
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
    end

    write()
    return 0
end
