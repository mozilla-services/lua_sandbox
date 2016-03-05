## Heka Kafka Producer

Kafka Lua module for output sandboxes.

### API

#### new

Creates a Heka Kafka producer.

```lua
local brokerlist    = "localhost:9092"
local producer_conf = producer_conf = {
    ["queue.buffering.max.messages"] = 20000,
    ["batch.num.messages"] = 200,
    ["message.max.bytes"] = 1024 * 1024,
    ["queue.buffering.max.ms"] = 10,
    ["topic.metadata.refresh.interval.ms"] = -1,
}
local producer = heka_kafka_producer.new(brokerlist, producer_conf)

```

*Arguments*
* brokerlist (string) - [librdkafka broker string](https://github.com/edenhill/librdkafka/blob/master/src/rdkafka.h#L2205)
* producer_conf (table) - [librdkafka producer configuration](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md#global-configuration-properties)

*Return*
* producer (userdata) - Kafka producer or an error is thrown

### API Methods

#### create_topic

Creates a topic to be used by a producer, no-op if the topic already exists.

```lua
producer:create_topic(topic) -- creates the topic if it does not exist

```

*Arguments*
* topic (string) - Name of the topic

*Return*
* none


#### has_topic

Tests if a producer is managing a topic.

```lua
local b = producer:has_topic(topic)

```

*Arguments*
* topic (string) - Name of the topic

*Return*
* bool - True if the producer is managing a topic with the specificed name


#### destroy_topic

Removes a topic from the producer.

```lua
producer:destroy_topic(topic)

```

*Arguments*
* topic (string) - Name of the topic

*Return*
* none - no-op on non-existent topic


#### send

Sends a message using the specified topic.

```lua
local ret = producer:send(topic, -1, sequence_id, payload)

```

*Arguments*
* topic (string) - Name of the topic
* partition (number) - Topic partition number (-1 for automatic assignment)
* sequence_id (lightuserdata) - Opaque pointer for checkpointing (passed into process_message)
* payload (string or (nil/none)) - Use nil/none to send the current active message while in
  the process_message call

*Return*
* ret (number) - 0 on success or errno
  - ENOBUFS (105) maximum number of outstanding messages has been reached
  - EMSGSIZE (90) message is larger than configured max size
  - ESRCH (2) requested partition is unknown in the Kafka cluster
  - ENOENT (3) topic is unknown in the Kafka cluster

#### poll

Polls the provided Kafka producer for events and invokes callback.  This should
be called after every send.

```lua
producer:poll()

```

*Arguments*
* none

*Return*
* none 
