## Input Sandbox

### Recommendations
Since he sandbox does not run in isolation there are some expectations of how
the host infrastructure behaves.  The current recommendation are based on the
Hindsight reference implementation.

### Required Lua Functions (called by the host)

#### process_message

Entry point for message creation.

*Arguments*
* checkpoint (nil number, string) - value of the last checkpoint value passed
  into `inject_message`

*Return*
* status_code (number)
  - success (less than or equal to zero)
  - fatal error (greater than zero)
* status_message (optional: string) logged when the status code is less than zero

### Available C Functions (called from the plugin)

#### read_config

Provides access to the sandbox configuration variables.

*Arguments*
* key (string) - configuration key name

*Return*
* value (string, number, bool, table)

#### decode_message

Converts a Heka protobuf encoded message string into a Lua table.

*Arguments*
* heka_pb (string) - Heka protobuf binary string

*Return*
* msg ([Heka message table (array fields)](message.md#array-based-message-fields))

#### inject_message

Sends a Heka protocol buffer message into the host.

*Arguments*
* msg ([Heka message table](message.md), [Heka stream reader](stream_reader.md) or Heka protobuf string)
* checkpoint (optional: number, string) - checkpoint to be returned in the `process_message` call

*Return*
* none - throws an error on invalid input

### Modes of Operation

#### Run Once
* Set the `ticker_interval` to zero and return from `process_message` when you
  are done.
* The `instruction_limit` configuration can be set if desired.

##### Example startup ping
```lua
-- cfg
-- send a simple 'hello' messages every time the host is started
ticker_interval = 0
```

```lua
-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- the checkpoint is optional and not being used by this plugin
function process_message()
    inject_message({Type = "hello"})
    return 0
end

```

#### Polling

* Set the `ticker_interval` greater than zero and non fatally (<=0) return from
  `process_message`, when the ticker interval expires `process_message` will be
  called again.
* The `instruction_limit` configuration can be set if desired.

##### Example startup ping
```lua
-- cfg
ticker_interval = 60
```

```lua
-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "io"

local msg = {
Type        = "uptime",
Payload     = "",
}

function process_message()
    local fh = io.popen("uptime")
    if fh then
        msg.Payload = fh:read("*a")
        fh:close()
    else
        return -1, "popen failed"
    end
    if msg.Payload then inject_message(msg) end
    return 0
end

```

#### Continuous

* Don't return from `process_message`.
* The `instruction_limit` configuration **MUST** be set to zero.

##### Example of a Heka protobuf stdin reader

```lua
-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
Reads a Heka protobuf stream from the stdin file handle

-- .cfg
instruction_limit = 0

--]]

local stdin = require "io".stdin
require "heka_stream_reader"

local hsr = heka_stream_reader.new(read_config("Logger"))

function process_message()
    local found, consumed, read
    repeat
        repeat
            found, consumed, read = hsr:find_message(stdin)
            if found then
                inject_message(hsr)
            end
        until not found
    until read == 0
    return 0
end
```
