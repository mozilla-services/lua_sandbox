-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"
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
    end
    return 0
end
