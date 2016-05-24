-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "heka_kafka_producer"
require "string"

local ok, err
ok, err = pcall(heka_kafka_producer.new)
assert(err == "bad argument #0 to '?' (incorrect number of arguments)", err)

ok, err = pcall(heka_kafka_producer.new, "")
assert(err == "invalid broker list", err)

ok, err = pcall(heka_kafka_producer.new, "brokerlist", true)
assert(err == "bad argument #2 to '?' (table expected, got boolean)", err)

ok, err = pcall(heka_kafka_producer.new, "local host", {["message.max.bytes"] = "foo"})
assert(err:match("^Failed to set message.max.bytes = foo"), err)

ok, err = pcall(heka_kafka_producer.new, "local host", {["message.max.bytes"] = 1})
assert(err ==  'Failed to set message.max.bytes = 1 : Configuration property "message.max.bytes" value 1 is outside allowed range 1000..1000000000\n', err)

ok, err = pcall(heka_kafka_producer.new, "local host", {["message.max.bytes"] = true})
assert(err:match("^Failed to set message.max.bytes = true"), err)

ok, err = pcall(heka_kafka_producer.new, "brokerlist", {[assert] = true})
assert(err == "invalid config key type: function", err)

ok, err = pcall(heka_kafka_producer.new, "brokerlist", {foo = assert})
assert(err == "invalid config value type: function", err)

local producer = heka_kafka_producer.new("localhost:9092",
                                         {
                                             ["topic.metadata.refresh.interval.ms"] = -1,
                                             ["batch.num.messages"] = 1,
                                             ["queue.buffering.max.ms"] = 1,
                                         })
local cnt = 0
local topic = "test"
local sid
function process_message(sequence_id)
    sid = sequence_id
    if cnt == 0 then
        local topic_tmp = "tmp"
        producer:create_topic(topic)
        assert(producer:has_topic(topic))
        assert(0 == producer:send(topic, -1, sequence_id)) -- send the original

        producer:create_topic(topic_tmp)
        producer:create_topic(topic_tmp, nil)
        assert(producer:has_topic(topic_tmp))
        producer:destroy_topic(topic_tmp)
        assert(not producer:has_topic(topic_tmp))
        ok, err = pcall(producer.create_topic, producer, "topic", true)
        assert(err == "bad argument #3 to '?' (table expected, got boolean)", err)
        ok, err = pcall(producer.send, producer, "foobar", -1, sequence_id)
        assert(err == "invalid topic", err)
    elseif cnt == 1 then
        assert(0 == producer:send(topic, -1, sequence_id, encode_message({Payload = "two"})))
    elseif cnt == 2 then
        assert(0 == producer:send(topic, -1, sequence_id, encode_message({Payload = "three"})))
    end
    cnt = cnt + 1
    return 0
end

function timer_event(ns)
    if sid then
        ok, err = pcall(producer.send, producer, topic, -1, sid)
        assert(err == "no active message", err)
        sid = nil
    end
    producer:poll()
end
