-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local pairs = pairs
local type = type
local string = require "string"

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

-- flattens a Lua table so it can be encoded as a protobuf fields object
function table_to_fields(t, fields, parent)
    for k,v in pairs(t) do
        if parent then
            full_key = string.format("%s.%s", parent, k)
        else
            full_key = k
        end
        if type(v) == "table" then
            table_to_fields(v, fields, full_key)
        else
            if type(v) ~= "userdata" then
                fields[full_key] = v
            end
        end
    end
end

return M
