-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local clf = require "common_log_format"

function process(tc)
    if tc == 0 then
        -- common log format
        local grammar = clf.build_nginx_grammar('$remote_addr - $remote_user [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        if not grammar then return -1 end
        local log = '127.0.0.1 - - [10/Feb/2014:08:46:41 -0800] "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0"'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    elseif tc == 1 then
        -- custom format to test the msec changes
        local grammar = clf.build_nginx_grammar('$msec $remote_addr "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"')
        if not grammar then return -1 end
        local log = '1391794831.755 127.0.0.1 "GET / HTTP/1.1" 304 0 "-" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:26.0) Gecko/20100101 Firefox/26.0" "-" 0.000'
        local fields = grammar:match(log)
        fields.time = fields.time/1e6
        output(fields)
    end

    write()
    return 0
end
