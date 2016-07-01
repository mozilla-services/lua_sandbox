-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "kafka"

--[[
# Heka Kafka Producer Output

## Sample Configuration
```lua
filename               = "heka_kafka.lua"
message_matcher        = "TRUE"
output_limit           = 8 * 1024 * 1024
brokerlist              = "localhost:9092" -- see https://github.com/edenhill/librdkafka/blob/master/src/rdkafka.h#L2205
ticker_interval        = 60
async_buffer_size      = 20000

topic_constant = "test"
producer_conf = {
    ["queue.buffering.max.messages"] = async_buffer_size,
    ["batch.num.messages"] = 200,
    ["message.max.bytes"] = output_limit,
    ["queue.buffering.max.ms"] = 10,
    ["topic.metadata.refresh.interval.ms"] = -1,
}
```
--]]
local brokerlist = read_config("brokerlist") or error("brokerlist must be set")
local topic_constant = read_config("topic_constant")
local topic_variable = read_config("topic_variable") or "Logger"
local producer_conf = read_config("producer_conf")

local producer = kafka.producer(brokerlist, producer_conf)

function process_message(sequence_id)
    local topic = topic_constant
    if not topic then
        topic = read_message(topic_variable) or "unknown"
    end
    producer:create_topic(topic) -- creates the topic if it does not exist

    producer:poll()
    local ret = producer:send(topic, -1, sequence_id) -- sends the current message

    if ret ~= 0 then
        if ret == 105 then
            return -3, "queue full" -- retry
        elseif ret == 90 then
            return -1, "message too large" -- fail
        elseif ret == 2 then
            error("unknown topic: " .. topic)
        elseif ret == 3 then
            error("unknown partition")
        end
    end

    return -5 -- asynchronous checkpoint management
end

function timer_event(ns)
    producer:poll()
end
