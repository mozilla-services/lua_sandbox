-- minimal message, required fields only
local test = "\010\016\233\213\137\149\106\254\064\066\175\098\058\163\017\067\202\068\016\000"
local msg = decode_message(test)
assert(msg.Timestamp == 0)

-- message headers only
local test = "\010\016\233\213\137\149\106\254\064\066\175\098\058\163\017\067\202\068\016\128\148\235\220\003\026\004type\034\006logger\040\009\050\007payload\058\011env_version\074\008hostname"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Type == "type")
assert(msg.Logger == "logger")
assert(msg.Payload == "payload")
assert(msg.EnvVersion == "env_version")
assert(msg.Hostname == "hostname")
assert(msg.Severity == 9)

-- repeated unpacked doubles
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\027\010\005count\016\003\057\000\000\000\000\000\000\240\063\057\000\000\000\000\000\000\240\063"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "count")
assert(msg.Fields[1].value_type == 3)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == 1)
assert(msg.Fields[1].value[2] == 1)

-- repeated packed doubles
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\027\010\005count\016\003\058\016\000\000\000\000\000\000\240\063\000\000\000\000\000\000\240\063"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "count")
assert(msg.Fields[1].value_type == 3)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == 1)
assert(msg.Fields[1].value[2] == 1)

-- repeated unpacked ints
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\013\010\005count\016\002\048\001\048\001"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "count")
assert(msg.Fields[1].value_type == 2)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == 1)
assert(msg.Fields[1].value[2] == 1)

-- repeated packed ints
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\013\010\005count\016\002\050\002\001\001"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "count")
assert(msg.Fields[1].value_type == 2)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == 1)
assert(msg.Fields[1].value[2] == 1)

-- repeated unpacked bools
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\013\010\005count\016\004\064\001\064\000"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "count")
assert(msg.Fields[1].value_type == 4)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == true)
assert(msg.Fields[1].value[2] == false)

-- repeated packed bools
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\013\010\005count\016\004\066\002\001\000"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "count")
assert(msg.Fields[1].value_type == 4)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == true)
assert(msg.Fields[1].value[2] == false)

-- repeated strings
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\023\010\005names\016\000\034\002s1\034\002s2\026\004keys"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "names")
assert(msg.Fields[1].representation == "keys")
assert(msg.Fields[1].value_type == 0)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == "s1")
assert(msg.Fields[1].value[2] == "s2")

-- repeated bytes
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\023\010\005names\016\001\042\002s1\042\002s2\026\004keys"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(msg.Fields[1].name == "names")
assert(msg.Fields[1].representation == "keys")
assert(msg.Fields[1].value_type == 1)
assert(#msg.Fields[1].value == 2)
assert(msg.Fields[1].value[1] == "s1")
assert(msg.Fields[1].value[2] == "s2")

-- recent timestamp
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\152\141\135\236\222\200\233\19"
msg = decode_message(test)
assert(msg.Timestamp == 1428523950000000000)

-- benchmark message
test = "\010\016\096\006\214\155\119\188\078\023\172\076\081\127\129\143\250\040\016\128\148\235\220\003\082\019\010\006\110\117\109\098\101\114\016\003\057\000\000\000\000\000\000\240\063\082\044\010\007\110\117\109\098\101\114\115\016\003\026\005\099\111\117\110\116\058\024\000\000\000\000\000\000\240\063\000\000\000\000\000\000\000\064\000\000\000\000\000\000\008\064\082\014\010\005\098\111\111\108\115\016\004\066\003\001\000\000\082\010\010\004\098\111\111\108\016\004\064\001\082\016\010\006\115\116\114\105\110\103\034\006\115\116\114\105\110\103\082\021\010\007\115\116\114\105\110\103\115\034\002\115\049\034\002\115\050\034\002\115\051"
msg = decode_message(test)
assert(msg.Timestamp == 1e9)
assert(#msg.Fields == 6)

assert(#msg.Fields[1].value == 1)
assert(msg.Fields[1].name == "number")
assert(msg.Fields[1].value_type == 3)
assert(msg.Fields[1].value[1] == 1)

assert(msg.Fields[2].name == "numbers")
assert(msg.Fields[2].value_type == 3)
assert(#msg.Fields[2].value == 3)
assert(msg.Fields[2].value[1] == 1)
assert(msg.Fields[2].value[2] == 2)
assert(msg.Fields[2].value[3] == 3)
assert(msg.Fields[2].representation == "count")

assert(#msg.Fields[3].value == 3)
assert(msg.Fields[3].name == "bools")
assert(msg.Fields[3].value[1])
assert(msg.Fields[3].value[2] == false)
assert(msg.Fields[3].value[3] == false)

assert(#msg.Fields[4].value == 1)
assert(msg.Fields[4].name == "bool")
assert(msg.Fields[4].value[1])

assert(#msg.Fields[5].value == 1)
assert(msg.Fields[5].name == "string")
assert(msg.Fields[5].value[1] == "string")

assert(msg.Fields[6].name == "strings")
assert(#msg.Fields[6].value == 3)
assert(msg.Fields[6].value[1] =="s1")
assert(msg.Fields[6].value[2] =="s2")
assert(msg.Fields[6].value[3] =="s3")

-- test negative varint headers
test = "\010\016\243\083\052\234\016\052\066\236\160\084\236\003\227\231\170\203\016\255\255\255\255\255\255\255\255\255\001\040\255\255\255\255\255\255\255\255\255\001\064\255\255\255\255\255\255\255\255\255\001"
msg = decode_message(test)
assert(msg.Timestamp == -1)
assert(msg.Pid == -1)
assert(msg.Severity == -1)

-- multi byte length
test = "\010\016\104\183\062\106\120\227\073\248\173\168\122\019\057\236\124\025\016\128\148\235\220\003\082\141\001\010\006\115\116\114\105\110\103\034\130\001\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057\048\049\050\051\052\053\054\055\056\057"
msg = decode_message(test)
assert(msg.Fields[1].value[1] == "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")


--[[
Parse Errors
--]]

-- too short
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "invalid message, too short", msg)

-- no uuid
test = "\016\128\148\235\220\003\082\023\010\005names\016\001\042\002s1\042\002s2\026\004keys"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "missing required field uuid: not found timestamp: found", msg)

-- no timestamp
local test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\082\023\010\005names\016\001\042\002s1\042\002s2\026\004keys"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "missing required field uuid: found timestamp: not found", msg)

-- missing field name
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\020\016\003\057\000\000\000\000\000\000\240\063\057\000\000\000\000\000\000\240\063"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "error in tag: 10 wiretype: 2 offset: 24", msg)

-- random string
test = "this is a test item over twenty bytes long"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "error in tag: 14 wiretype: 4 offset: 0", msg)

-- repeated packed with ints invalid length
test = "\010\016\111\021\235\034\090\107\077\120\169\175\058\232\153\002\231\132\016\128\148\235\220\003\082\013\010\005count\016\002\050\003\001\001"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "error in tag: 10 wiretype: 2 offset: 24", msg)

-- invalid timestamp varint encoding
local test = "\010\016\233\213\137\149\106\254\064\066\175\098\058\163\017\067\202\068\016\128\148\235\220\220\220\220\220\220\220"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "error in tag: 2 wiretype: 0 offset: 18", msg)

-- invalid length encoding
local test = "\010\016\233\213\137\149\106\254\064\066\175\098\058\163\017\067\202\068\016\128\148\235\220\003\026\128\148\235\220\220\220\220\220\220\220type"
ok, msg = pcall(decode_message, test)
assert(not ok)
assert(msg == "error in tag: 3 wiretype: 2 offset: 24", msg)
