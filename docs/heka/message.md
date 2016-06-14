# Heka Message Table

## Hash Based Message Fields

```lua
{
Uuid        = "data",               -- auto generated if not a 16 byte raw binary UUID or a 36 character human readable UUID
Logger      = "nginx",              -- defaults to the Logger configuration value but can be overridden with the `restricted_headers` configuration
Hostname    = "example.com",        -- defaults to the Hostname configuration value but can be overridden with the `restricted_headers` configuration
Timestamp   = 1e9,                  -- auto generated if not a number
Type        = "TEST",
Payload     = "Test Payload",
EnvVersion  = "0.8",
Pid         = 1234,
Severity    = 6,
Fields      = {
            http_status     = 200,  -- encoded as a double
            request_size    = {value=1413, value_type=2, representation="B"} -- encoded as an integer
            }
}
```

## Array Based Message Fields
```lua
{
-- Message headers are the same as above
Fields      = {
            {name="http_status" , value=200}, -- encoded as a double
            {name="request_size", value=1413, value_type=2, representation="B"} -- encoded as an integer
            }
}
```
