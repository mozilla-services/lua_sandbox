-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
## Elasticsearch Encoder for Unified Telemetry Messages

### Module Configuration Table

[Common Options](../elasticsearch.lua#8)
```lua
-- Array of Heka message field names that should be passed to Elasticsearch.
fields = {"Payload", "Fields[docType]"} -- required
```
### Sample Output
```json
{"index":{"_index":"mylogger-2014.06.05","_type":"mytype-host.domain.com"}}
{"Payload":"data","docType":"main"}
```
--]]

-- Imports
local cjson         = require "cjson"
local string        = require "string"
local os            = require "os"
local math          = require "math"
local mi            = require "heka.msg_interpolate"
local es            = require "heka.elasticsearch"
local hj            = require "heka_json"
local ipairs        = ipairs
local pcall         = pcall
local read_message  = read_message
local cfg           = es.load_encoder_cfg()

if not cfg.fields or type(cfg.fields) ~= "table" or #cfg.fields == 0 then
    error("fields must be specified")
end

local M = {}
setfenv(1, M) -- Remove external access to contain everything in the module

local info_fields = {
    "sessionId"
  , "subsessionId"
  , "previousSessionId"
  , "previousSubsessionId"
  , "subsessionCounter"
  , "profileSubsessionCounter"
  , "sessionStartDate"
  , "subsessionStartDate"
  , "subsessionLength"
  , "sessionLength"
}

local environment_fields = {
    "telemetryEnabled"
}

local static_fields = {}
local dynamic_fields = {}

local function key(str)
    return str:match("^Fields%[(.+)%]$") or error("invalid field name: " .. str)
end

for i, field in ipairs(cfg.fields) do
    local fp = mi.header_fields[field]
    if fp then
        static_fields[#static_fields+1] = {field, fp}
    else
        dynamic_fields[#dynamic_fields+1] = {field, key(field)}
    end
end

function encode()
    local ns
    if cfg.es_index_from_timestamp then ns = read_message("Timestamp") end

    local idx_json = es.bulkapi_index_json(cfg.index, cfg.type_name, cfg.id, ns)

    local tbl = {}
    for i, field in ipairs(static_fields) do
        tbl[field[1]] = field[2]()
    end

    for i, field in ipairs(dynamic_fields) do
        local full_name = field[1]
        local short_name = field[2]
        local z = 0
        local v = read_message(full_name, nil, z)
        while v do
            if z == 0 then
                tbl[short_name] = v
            elseif z == 1 then
                tbl[short_name] = {tbl[short_name], v}
            elseif z > 1 then
                tbl[short_name][z+1] = v
            end
            z = z + 1
            v = read_message(full_name, nil, z)
        end
    end

    local ok, doc = pcall(hj.parse_message, "Fields[payload.info]")
    if ok then
        for i, k in ipairs(info_fields) do
            tbl[k] = doc:value(doc:find(k))
        end
    end

    ok, doc = pcall(hj.parse_message, "Fields[environment.settings]")
    if ok then
        for i, k in ipairs(environment_fields) do
            tbl[k] = doc:value(doc:find(k))
        end
    end

    ok, doc = pcall(hj.parse_message, "Payload")
    if ok then
        tbl.architecture = doc:value(doc:find("application", "architecture"))
    end

    if tbl.creationTimestamp then
        if ns then
            tbl.Latency = math.floor((ns - tbl.creationTimestamp) / 1e9)
        end
        tbl.creationTimestamp = os.date("!%Y-%m-%dT%H:%M:%SZ", tbl.creationTimestamp / 1e9)
    end

    return string.format("%s\n%s\n", idx_json, cjson.encode(tbl))
end

return M
