-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "kafka"

--[[
# Heka Kafka Consumer Input

## Sample Configuration
```lua
filename                = "heka_kafka.lua"
output_limit            = 8 * 1024 * 1024
brokerlist              = "localhost:9092" -- see https://github.com/edenhill/librdkafka/blob/master/src/rdkafka.h#L2205

-- in balanced consumer group mode a consumer can only subscribe on topics, not topics:partitions.
-- The partition syntax is only used for manual assignments (without balanced consumer groups).
topics                  = {"test"}
ticker_interval         = 60

-- https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md#global-configuration-properties
consumer_conf = {
    ["group.id"] = "test_group", -- must always be provided (a single consumer is considered a group of one
    -- in that case make this a unique identifier)
    ["message.max.bytes"] = output_limit,
}

-- https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md#topic-configuration-properties
topic_conf = {
    -- ["auto.commit.enable"] = true, -- cannot be overridden
    -- ["offset.store.method"] = "broker, -- cannot be overridden
}
```
--]]
local brokerlist    = read_config("brokerlist") or error("brokerlist must be set")
local topics        = read_config("topics") or error("topics must be set")
local consumer_conf = read_config("consumer_conf")
local topic_conf    = read_config("topic_conf")

local consumer = kafka.consumer(brokerlist, topics, consumer_conf, topic_conf)

local err_msg = {
    Logger  = read_config("Logger"),
    Type    = "error",
    Payload = nil,
}

function process_message()
    while true do
        local msg, topic, partition, key = consumer:receive()
        if msg then
            local ok, err = pcall(inject_message, msg)
            if not ok then
                err_msg.Payload = err
                pcall(inject_message, err_msg)
            end
        end
    end
    return 0 -- unreachable but here for consistency
end
