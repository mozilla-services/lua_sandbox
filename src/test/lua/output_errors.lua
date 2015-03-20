-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "cjson"
require "circular_buffer"; require "bloom_filter"; require "cuckoo_filter"
require "lpeg"
function process(tc)
    if tc == 0 then -- error internal reference
        output({})
    elseif tc == 1 then -- error escape overflow
        local escape = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
        for i=1, 10 do
            escape = escape .. escape
        end
        output(escape)
    elseif tc == 2 then -- heka error mis-match field array
        local hm = {Timestamp = 1e9, Fields = {counts={2,"ten",4}}}
        write_message(hm)
    elseif tc == 3 then -- heka error nil field
        local hm = {Timestamp = 1e9, Fields = {counts={}}}
        write_message(hm)
    elseif tc == 4 then -- unsupported userdata
        write_output(lpeg.P"")
    elseif tc == 5 then -- overflow
        local cb = circular_buffer.new(1000, 1, 60);
        write_output(cb)
    elseif tc == 6 then -- invalid array type
        local hm = {Timestamp = 1e9, Fields = {counts={{1},{2}}}}
        write_message(hm)
    elseif tc == 7 then -- overflow cjson encode buffer
        local t = {}
        for i=1, 10 do
            t[#t+1] = "this is a test"
        end
        cjson.encode(t)
    elseif tc == 8 then -- invalid type
        write_message("string")
    elseif tc == 9 then -- invalid value_type integer
        local hm = {Timestamp = 1e9, Fields = {counts={value="s", value_type=2}}}
        write_message(hm)
    elseif tc == 10 then -- invalid value_type double
        local hm = {Timestamp = 1e9, Fields = {counts={value="s", value_type=3}}}
        write_message(hm)
    elseif tc == 11 then -- invalid value_type bool
        local hm = {Timestamp = 1e9, Fields = {counts={value="s", value_type=4}}}
        write_message(hm)
    elseif tc == 12 then -- invalid value_type string
        local hm = {Timestamp = 1e9, Fields = {counts={value=1, value_type=0}}}
        write_message(hm)
    elseif tc == 13 then -- invalid value_type bytes
        local hm = {Timestamp = 1e9, Fields = {counts={value=1, value_type=1}}}
        write_message(hm)
    elseif tc == 14 then -- invalid value_type bool
        local hm = {Timestamp = 1e9, Fields = {counts={value=1, value_type=4}}}
        write_message(hm)
    elseif tc == 15 then -- invalid value_type string
        local hm = {Timestamp = 1e9, Fields = {counts={value=true, value_type=0}}}
        write_message(hm)
    elseif tc == 16 then -- invalid value_type bytes
        local hm = {Timestamp = 1e9, Fields = {counts={value=true, value_type=1}}}
        write_message(hm)
    elseif tc == 17 then -- invalid value_type integer
        local hm = {Timestamp = 1e9, Fields = {counts={value=true, value_type=2}}}
        write_message(hm)
    elseif tc == 18 then -- invalid value_type double
        local hm = {Timestamp = 1e9, Fields = {counts={value=true, value_type=3}}}
        write_message(hm)
    elseif tc == 19 then -- bloom_filter doesn't implement lsb_output
        local bf = bloom_filter.new(10, 0.01)
        local hm = {Timestamp = 1e9, Fields = {bf = {value=bf, representation="bf"}}}
        write_message(hm)
    elseif tc == 20 then -- cuckoo_filter doesn't implement lsb_output
        local cf = cuckoo_filter.new(10)
        local hm = {Timestamp = 1e9, Fields = {cf = {value=cf, representation="cf"}}}
        write_message(hm)
    end
    return 0
end
