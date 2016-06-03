-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Elasticsearch Utility Functions

## Module Configuration Table (common options)
```lua
-- Boolean flag, if true then any time interpolation (often used to generate the
-- ElasticSeach index) will use the timestamp from the processed message rather
-- than the system time.
es_index_from_timestamp == false -- optional, default shown

-- String to use as the `_index` key's value in the  generated JSON.
-- Supports field interpolation as described below.
index = "heka-%{%Y.%m.%d}" -- optional, default shown

-- String to use as the `_type` key's value in the generated JSON.
-- Supports field interpolation as described below.
type_name = "message" -- optional, default shown

-- String to use as the `_id` key's value in the generated JSON.
-- Supports field interpolation as described below.
id = nil -- optional, default shown

```

## Functions

### bulkapi_index_json

Returns a simple JSON 'index' structure satisfying the [ElasticSearch BulkAPI](http://www.elasticsearch.org/guide/en/elasticsearch/reference/current/docs-bulk.html)

*Arguments*
* index (string or nil) - Used as the `_index` key's value in the generated JSON
  or nil to omit the key. Supports field interpolation as described below.
* type_name (string or nil) - Used as the `_type` key's value in the generated
  JSON or nil to omit the key. Supports field interpolation as described below.
* id (string or nil) - Used as the `_id` key's value in the generated JSON or
  nil to omit the key. Supports field interpolation as described below.
* ns (number or nil) - Nanosecond timestamp to use for any strftime field
  interpolation into the above fields. Current system time will be used if nil.

*Return*
* JSON - String suitable for use as ElasticSearch BulkAPI index directive.

*See*
[Field Interpolation](msg_interpolate.html)
--]]

local cjson         = require "cjson"
local mi            = require "heka.msg_interpolate"
local string        = require "string"
local assert        = assert
local type          = type
local read_message  = read_message
local read_config   = read_config

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module.

local result_inner = {
    _index = nil,
    _type = nil,
    _id = nil
}

--[[ Public Interface --]]

function bulkapi_index_json(index, type_name, id, ns)
    local secs
    if ns then
        secs = ns / 1e9
    end
    if index then
        result_inner._index = string.lower(mi.interpolate(index, secs))
    else
        result_inner._index = nil
    end
    if type_name then
        result_inner._type = mi.interpolate(type_name, secs)
    else
        result_inner._type = nil
    end
    if id then
        result_inner._id = mi.interpolate(id, secs)
    else
        result_inner._id = nil
    end
    return cjson.encode({index = result_inner})
end


function load_encoder_cfg()
    local cfg = read_config("encoder_cfg")

    if cfg.es_index_from_timestamp == nil then
        cfg.es_index_from_timestamp = false
    else
        assert(type(cfg.es_index_from_timestamp) == "boolean",
               "es_index_from_timestamp must be nil or boolean")
    end

    if cfg.index == nil then
        cfg.index = "heka-%{%Y.%m.%d}"
    else
        assert(type(cfg.index) == "string", "index must be nil or a string")
    end

    if cfg.type_name == nil then
        cfg.type_name = "message"
    else
        assert(type(cfg.type_name) == "string", "type_name must be nil or a string")
    end

    return cfg
end

return M
