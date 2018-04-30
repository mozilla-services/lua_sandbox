# Heka Message Table

## Hash Based Message Fields

```lua
{
Uuid        = "data",               -- restricted header, auto generated if not a 16 byte raw binary UUID or a 36 character human readable UUID
Logger      = "nginx",              -- restricted header, defaults to the Logger configuration value
Hostname    = "example.com",        -- restricted header, defaults to the Hostname configuration value
Timestamp   = 1e9,                  -- restricted header, defaults to the current time
Type        = "TEST",
Payload     = "Test Payload",
EnvVersion  = "0.8",
Pid         = 1234,                 -- restricted header, defaults to the Pid configuration value
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
