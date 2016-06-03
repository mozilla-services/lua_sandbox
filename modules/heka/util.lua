-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Heka Sandbox Utility Module

## Functions

### table_to_fields

Flattens a Lua table so it can be encoded as a protobuf fields object.

*Arguments*
- hash (table) - table to flatten (not modified)
- fields (table) - table to receive the flattened output
- parent (string) - key prefix
- separator (string) - key separator (default = ".") i.e. 'foo.bar'
- max_depth (number) - maximum nesting before converting the remainder of the
  structure to a JSON string

*Return*
- none - in-place modification of `fields`
--]]

-- Imports
local pairs = pairs
local type = type
local string = require "string"
local cjson = require "cjson"

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

function table_to_fields(t, fields, parent, char, max_depth)
    if type(char) ~= "string" then
        char = "."
    end

    for k,v in pairs(t) do
        if parent then
            full_key = string.format("%s%s%s", parent, char, k)
        else
            full_key = k
        end

        if type(v) == "table" then
            local _, sep_count = string.gsub(full_key, char, "")
            local depth = sep_count + 1

            if type(max_depth) == "number" and depth >= max_depth then
                fields[full_key] = cjson.encode(v)
            else
                table_to_fields(v, fields, full_key, char, max_depth)
            end
        else
            if type(v) ~= "userdata" then
                fields[full_key] = v
            end
        end
    end
end

return M
