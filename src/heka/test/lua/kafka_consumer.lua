-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "heka_kafka_consumer"
require "string"

local ok, err
ok, err = pcall(heka_kafka_consumer.new)
assert(err == "bad argument #0 to '?' (incorrect number of arguments)", err)

ok, err = pcall(heka_kafka_consumer.new, true, nil, nil)
assert(err == "bad argument #1 to '?' (string expected, got boolean)", err)

ok, err = pcall(heka_kafka_consumer.new, "test", true, nil)
assert(err == "bad argument #2 to '?' (table expected, got boolean)", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {}, nil)
assert(err == "bad argument #2 to '?' (the topics array is empty)", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test"}, {})
assert(err == "group.id must be set", err)

ok, err = pcall(heka_kafka_consumer.new, "", {"test"}, {["group.id"] = "foo"})
assert(err == "invalid broker list", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test"}, {["group.id"] = "foo"}, {["auto.offset.reset"] = "foobar"})
assert(err == 'Failed to set auto.offset.reset = foobar : Invalid value for configuration property "auto.offset.reset"', err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test"}, {["group.id"] = "foo"}, {["auto.offset.reset"] = 0})
assert(err == 'Failed to set auto.offset.reset = 0 : Invalid value for configuration property "auto.offset.reset"', err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test"}, {["group.id"] = "foo"}, {["auto.offset.reset"] = true})
assert(err == 'Failed to set auto.offset.reset = true : Invalid value for configuration property "auto.offset.reset"', err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test"}, {["group.id"] = "foo"}, {["auto.offset.reset"] = assert})
assert(err == "invalid config value type: function", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test"}, {["group.id"] = "foo"}, {[assert] = true})
assert(err == "invalid config key type: function", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test:9000000000"}, {["group.id"] = "foo"})
assert(err == "invalid topic partition > INT32_MAX", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test:-1"}, {["group.id"] = "foo"})
assert(err == "invalid topic partition < 0", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {true}, {["group.id"] = "foo"})
assert(err == "topics must be an array of strings", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"one", item = "test"}, {["group.id"] = "foo"})
assert(err == "topics must be an array of strings", err)

ok, err = pcall(heka_kafka_consumer.new, "test", {"test"}, {["group.id"] = "foo", ["message.max.bytes"] = true})
assert(err ==  'Failed to set message.max.bytes = true : Configuration property "message.max.bytes" value 0 is outside allowed range 1000..1000000000\n', err)


local consumer = heka_kafka_consumer.new("localhost:9092", {"test"}, {["group.id"] = "integration_testing"})
local consumer1 = heka_kafka_consumer.new("localhost:9092", {"test:1"}, {["group.id"] = "other"})
local pb, topic, partition, key = consumer1:receive()
assert(not pb)

local payloads = {"one", "two", "three"}

function process_message()
    local cnt = 0
    for i=1, 10 do
        pb, topic, partition, key = consumer:receive()
        if pb then
            cnt = cnt + 1
            local msg = decode_message(pb)
            if msg.Payload ~= payloads[cnt] then
                return -1, string.format("expected: %s received: %s", payloads[cnt], msg.Payload)
            end
            if cnt == 3 then return 0 end
        end
    end
    return -1, string.format("received %d/3 messages", cnt)
end
