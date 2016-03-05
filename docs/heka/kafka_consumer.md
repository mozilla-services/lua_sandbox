## Heka Kafka Consumer

Kafka Lua module for input sandbox plugins.

### API

#### new

Creates a Heka Kafka consumer.

```lua
local brokerlist    = "localhost:9092"
local topics        = {"test"}
local consumer_conf = {["group.id"] = "test_g1"})
local topic_conf    = nil
local consumer   = heka_kafka_consumer.new(brokerlist, topics, consumer_conf, topic_conf)

```

*Arguments*
* brokerlist (string) - [librdkafka broker string](https://github.com/edenhill/librdkafka/blob/master/src/rdkafka.h#L2205)
* topics (array of 'topic[:partition]' strings) - Balanced consumer group mode a
  consumer can only subscribe on topics, not topics:partitions. The partition 
  syntax is only used for manual assignments (without balanced consumer groups).
* consumer_conf (table) - must contain 'group.id' see: [librdkafka consumer configuration](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md#global-configuration-properties)
* topic_conf (table, optional) - [librdkafka topic configuration](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md#topic-configuration-properties)

*Return*
* consumer (userdata) - Kafka consumer or an error is thrown

### API Methods

#### receive

Receives a message from the specified Kafka topic(s).

```lua
local msg, topic, partition, key = consumer:receive()

```

*Arguments*
* none

*Return*
* msg (string) - Kafka message payload
* topic (string) - Topic name the message was received from
* partition (number) - Topic partition the message was received from
* key (string) - Message key (if available)
