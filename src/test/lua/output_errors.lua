-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "circular_buffer"
require "lpeg"

function process(tc)
    if tc == 0 then -- error internal reference
        local a = {x = {1,2,3}, y = {2}}
        a.ir = a.x
        output(a)
    elseif tc == 1 then -- error circular reference
        local a = {x = 1, y = 2}
        a.self = a
        output(a)
    elseif tc == 2 then -- error escape overflow
        local escape = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
        for i=1, 10 do
            escape = escape .. escape
        end
        output({escape = escape})
    elseif tc == 3 then -- heka error mis-match field array
        local hm = {Timestamp = 1e9, Fields = {counts={2,"ten",4}}}
        write(hm)
    elseif tc == 4 then -- heka error nil field
        local hm = {Timestamp = 1e9, Fields = {counts={}}}
        write(hm)
    elseif tc == 5 then -- unsupported userdata
        write(lpeg.P"")
    elseif tc == 6 then -- overflow
        local cb = circular_buffer.new(1000, 1, 60);
        write(cb)
    end
    return 0
end
