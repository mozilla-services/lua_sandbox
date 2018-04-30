# Analysis Sandbox Interface

## Recommendations
Since the sandbox does not run in isolation there are some expectations of how
the host infrastructure behaves.  The current recommendations are based on the
Hindsight reference implementation.

## Disabled Functionality
- [base library](http://www.lua.org/manual/5.1/manual.html#5.1)
    - collectgarbage, dofile, load, loadfile, loadstring, newproxy
- [coroutine](http://www.lua.org/manual/5.1/manual.html#5.2)
    - the entire module is inaccessible
- [string](http://www.lua.org/manual/5.1/manual.html#5.4)
    - dump
- [io](http://www.lua.org/manual/5.1/manual.html#5.7)
    - the entire module is inaccessible
- [os](http://www.lua.org/manual/5.1/manual.html#5.8)
    - getenv, execute, exit, remove, rename, setlocale, tmpname

## Required Lua Functions (called by the host)

### process_message

Called when the host has a message available for analysis.  Usually used in
combination with a [message matcher](/util/message_matcher.md) expression.

Recommenation: specify this as a `message_matcher` configuration option.

*Arguments*
* none

*Return*
* status_code (number)
    * success (less than or equal to zero)
    * fatal error (greater than zero)
* status_message (optional: string) logged when the status code is less than
  zero

### timer_event

Called when the host timer expires or on shutdown.

Recommendation: specify this as a `ticker_interval` configuration option.

*Arguments*
* ns (number) - nanosecond timestamp of the function call (it is actually
  `time_t * 1e9` to keep the timestamp units consistent so it will only have a
  one second resolution)
* shutdown (bool) - true if timer_event is being called due to a shutdown

*Return*
* none

## Available C Functions (called from the plugin)

### read_config

Provides access to the sandbox configuration variables.

*Arguments*
* key (string) - configuration key name

*Return*
* value (string, number, bool, table)

### read_message

Provides access to the Heka message data. Note that both fieldIndex and
arrayIndex are zero-based (i.e. the first element is 0) as opposed to Lua's
standard indexing, which is one-based.

*Arguments*
* variableName (string)
    * framed (returns the Heka message protobuf string including the framing
      header)
    * raw (returns the Heka message protobuf string)
    * size (returns the size of the raw Heka message protobuf string)
    * Uuid
    * Type
    * Logger
    * Payload
    * EnvVersion
    * Hostname
    * Timestamp
    * Severity
    * Pid
    * Fields[*name*]
* fieldIndex (unsigned) - only used in combination with the Fields variableName
use to retrieve a specific instance of a repeated field *name*; zero indexed
* arrayIndex (unsigned) - only used in combination with the Fields variableName
use to retrieve a specific element out of a field containing an array; zero
indexed
* zeroCopy (bool, optional default false) - returns a userdata place holder for
the message variable (only valid for string types). Non string headers throw an
error during construction, non string fields throw an error on data retrieval.

*Return*
* value (number, string, bool, nil, userdata depending on the type of variable
  requested)

### decode_message

Converts a Heka protobuf encoded message string into a Lua table or throws an
error.

*Arguments*
* heka_pb (string, userdata) - Heka protobuf binary string or a zero copy
  userdata object containing a Heka protobuf binary string.

*Return*
* msg ([Heka message table (array fields)](message.md#array-based-message-fields))
with the value member always being an array (even if there is only a
single item). This format makes working with the output more consistent. The
wide variation in the inject table formats is to ease the construction of the
message especially when using an LPeg grammar transformation.

### inject_message

Creates a new Heka protocol buffer message using the contents of the specified
Lua table (overwriting whatever is in the payload buffer). `Timestamp`,
`Logger`, `Hostname` and `Pid` are restricted header values. An override
configuration option is provided `restricted_headers`; when true (default) the
headers are always set to the configuration values; when false the headers are
set to the values provided in the message table, if no value is provided it
defaults to the appropriate value.

*Arguments*
* msg ([Heka message table](message.md))

*Return*
* none (throws an error if the table does not match the Heka message schema)

### add_to_payload

Appends the arguments to the payload buffer for incremental construction of the
final payload output (`inject_payload` finalizes the buffer and sends the
message to the infrastructure). This function is a rename of the generic sandbox
output function to improve the readability of the plugin code.

*Arguments*
* arg (number, string, bool, nil, supported userdata)

*Return*
* none (throws an error if arg is an unsupported type)

### inject_payload

This is a wrapper function for `inject_message` that is included for backwards
compatibility. The function creates a new Heka message using the contents of the
payload buffer (pre-populated with `add_to_payload`) and combined with any
additional payload_args passed here. The payload buffer is cleared after the
injection. The `payload_type` and `payload_name` arguments are two pieces of
optional metadata stored is message fields. The resulting message is structured
like this:
```lua
msg = {
Timestamp   = <current time>
Uuid        = "<auto generated>"
Hostname    = "<the Hostname from the config>"
Logger      = "<the Logger name from the config>"
Type        = "inject_payload"
Payload     = "<payload buffer contents>"
Fields      = {
    payload_type = "txt",
    payload_name = "",
}
```

*Arguments*

* payload_type (optional: string, default "txt") - describes the content type of
  the injected payload data
* payload_name (optional: string,  default "") - names the content to aid in
  downstream filtering
* arg3 (optional) -ame type restrictions as `add_to_payload`
* ...
* argN

*Return*
* none (throws an error if arg is an unsupported type)

### Modes of Operation

#### Lock Step
* Receives one call to `process_message`, operates on the message, and returns
  success (0) or failure (-1)

#### Example simple counter plugin
```lua
-- cfg
message_matcher = "TRUE"
ticker_interval = 5
```

```lua
-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local cnt = 0

function process_message()
    cnt = cnt + 1
    return 0
end

function timer_event()
    inject_payload("txt", "count", cnt)
end
```
