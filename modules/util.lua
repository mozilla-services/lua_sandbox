-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Imports
local pairs = pairs
local type = type
local string = require "string"
local cjson = require "cjson"

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

-- Flattens a Lua table so it can be encoded as a protobuf fields object.
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

-- Effectively removes all array values up to the provided index from an array
-- by copying end values to the array head and setting now unused entries at
-- the end of the array to `nil`.
function behead_array(idx, array)
    if idx <= 1 then return end
    local array_len = #array
    local start_nil_idx = 1 -- If idx > #array we zero it out completely.
    if idx <= array_len then
        -- Copy values to lower indexes.
        local difference = idx - 1
        for i = idx, array_len do
            array[i-difference] = array[i]
        end
        start_nil_idx = array_len - difference + 1
    end
    -- Empty out the end of the array.
    for i = start_nil_idx, array_len do
        array[i] = nil
    end
end

return M
