# Lua Sandbox Compression Module

## Overview
Built-in Lua Sandbox module for various types of compresssion.


### Example Usage
```lua
require "lsb_compression"
local inflated = lsb_compression.ungzip("\031\139\008\000\000\000\000\000\000\003\075\203\207\079\074\044\226\002\000\071\151\044\178\007\000\000\000")
-- inflated == "foobar\n"
```

### Functions

#### ungzip
Inflates a gzipped string (only availble if libz is installed on the system).

*Arguments*
- bytes (string) - gzip compressed string to inflate
- max_size (number, nil) - maximum size the of the resulting string (default 0 (unlimited))

*Return*
- inflated (string) - nil if bytes are not a gzip string or the output would exceed `max_size`
