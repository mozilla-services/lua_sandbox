# Input Sandbox Interface

## Recommendations
Since the sandbox does not run in isolation there are some expectations of how
the host infrastructure behaves.  The current recommendations are based on the
Hindsight reference implementation.

## Disabled Functionality
- [base library](http://www.lua.org/manual/5.1/manual.html#5.1)
    - dofile, load, loadfile, loadstring, newproxy
- [string](http://www.lua.org/manual/5.1/manual.html#5.4)
    - dump
- [os](http://www.lua.org/manual/5.1/manual.html#5.8)
    - exit, setlocale

## Required Lua Functions (called by the host)

### process_message

Entry point for message creation.

*Arguments*
* checkpoint (nil number, string) - value of the last checkpoint value passed
into `inject_message`

*Return*
* status_code (number)
  - success (less than or equal to zero)
  - fatal error (greater than zero)
* status_message (optional: string) logged when the status code is less than
zero

## Available C Functions (called from the plugin)

### read_config

Provides access to the sandbox configuration variables.

*Arguments*
* key (string) - configuration key name

*Return*
* value (string, number, bool, table)

### is_running

Provides a synchronization point for collecting statistics and communicating
shutdown status.

*Arguments*
* none

*Return*
* running (boolean) - true if a sandbox state is LSB_RUNNING, false if not or
  throws an error if the request to the host fails.

### decode_message

Converts a Heka protobuf encoded message string into a Lua table. See
[decode_message](analysis.md#decodemessage) for details.

### inject_message

Sends a Heka protocol buffer message into the host. For the Heka message table
arguments `Timestamp`, `Logger`, `Hostname` and `Pid` are restricted header
values. An override configuration option is provided `restricted_headers`; when
true the headers are always set to the configuration values; when false
(default) the headers are set to the values provide in the message table,
if no value is provided it defaults to the appropriate value.

*Arguments*
* msg ([Heka message table](message.md),
  [Heka stream reader](#heka-stream-reader-methods),
  Heka protobuf string,
  or nil (if only updating the checkpoint))
* checkpoint (optional: number, string) - checkpoint to be returned in the
 `process_message` call

*Return*
* none - throws an error on invalid input

### create_stream_reader
Creates a Heka stream reader to enable parsing of a framed Heka protobuf stream
in a Lua sandbox. See:
[Example of a Heka protobuf reader](#example-of-a-heka-protobuf-stdin-reader)

*Arguments*
* name (string) - name of the stream reader (used in the log)

*Return*
* hsr (userdata) - Heka stream reader or an error is thrown

#### Heka Stream Reader Methods

##### find_message

Locates a Heka message within the stream.

```lua
local found, consumed, need = hsr:find_message(buf)

```

*Arguments*
* buf (string, userdata (FILE*)) - buffer containing a Heka protobuf stream data
  or a userdata file object
* decode (bool default: true) - true if the framed message should be protobuf
  decoded

*Return*
* found (bool) - true if a message was found
* consumed (number) - number of bytes consumed so the offset can be tracked for
  checkpointing purposes
* need/read (number) - number of bytes needed to complete the message or fill
  the underlying buffer or in the case of a file object the number of bytes
  added to the buffer

##### decode_message

Converts a Heka protobuf encoded message string into a stream reader
representation. Note: this operation clears the internal stream reader buffer.

*Arguments*
* heka_pb (string) - Heka protobuf binary string

*Return*
* none - throws an error on failure

##### read_message

Provides access to the Heka message data within the reader object. The zeroCopy
flag is not accepted here.

```lua
local ts = hsr:read_message("Timestamp")

```
See [read_message](analysis.md#readmessage) for details.

## Modes of Operation

### Run Once
* Set the `ticker_interval` to zero and return from `process_message` done.
* The `instruction_limit` configuration can be set if desired.

#### Example startup ping
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

### Polling

* Set the `ticker_interval` greater than zero and non fatally (<=0) return from
  `process_message`, when the ticker interval expires `process_message` will be
  called again.
* The `instruction_limit` configuration can be set if desired.

#### Example heartbeat ping
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

### Continuous

* Don't return from `process_message`.
* The `instruction_limit` configuration **MUST** be set to zero.

#### Example of a Heka protobuf stdin reader

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
require "string"

local hsr = create_stream_reader(read_config("Logger"))

function process_message()
    local cnt = 0
    local found, consumed, read
    repeat
        repeat
            found, consumed, read = hsr:find_message(stdin)
            if found then
                inject_message(hsr)
                cnt = cnt + 1
            end
        until not found
    until read == 0
    return 0, string.format("processed %d messages", cnt)
end
```
