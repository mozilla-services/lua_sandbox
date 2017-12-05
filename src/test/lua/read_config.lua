assert(type(read_config) == "function")
assert(read_config("memory_limit") == 65765)
assert(read_config("instruction_limit") == 1000)
assert(read_config("output_limit") == 1024)
assert(read_config("input_limit") == 1024 * 64)

local array = read_config("array")
assert(type(array) ==  "table")
assert(array[1] == "foo")
assert(array[2] == 99)

local hash = read_config("hash")
assert(type(hash) == "table")
assert(hash.foo == "bar")
assert(hash.hash1.subfoo == "subbar")

assert(type(read_config("path")) == "string")
assert(type(read_config("cpath")) == "string")
