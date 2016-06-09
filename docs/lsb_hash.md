# Lua Sandbox Hash Module

## Overview
Built-in Lua Sandbox module for various types of hashing.

### Example Usage
```lua
require "lsb_hash"
local crc = lsb_hash.crc32("foobar")
-- crc == 2666930069
```

### Functions

#### crc32

CRC-32 checksum (only availble if libz is installed on the system) .

*Arguments*
- value (string) - string to checksum

*Return*
- checksum (number) - unsigned int
