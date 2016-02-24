-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require "heka_json"
require "heka_stream_reader"

schema_json = [[{
    "type":"object",
    "required":["Timestamp"],
    "properties":{
        "Timestamp":{
            "type":"integer",
            "minimum":0
        },
        "Type":{
            "type":"string"
        },
        "Logger":{
            "type":"string"
        },
        "Hostname":{
            "type":"string",
            "format":"hostname"
        },
        "EnvVersion":{
            "type":"string",
            "pattern":"^[0-9]+(\\.[0-9]+){0,2}"
        },
        "Severity":{
            "type":"integer",
            "minimum":0,
            "maximum":7
        },
        "Pid":{
            "type":"integer",
            "minimum":0
        },
        "Fields":{
            "type":"object",
            "minProperties":1,
            "additionalProperties":{
                "anyOf": [
                    { "$ref": "#/definitions/field_value"},
                    { "$ref": "#/definitions/field_array"},
                    { "$ref": "#/definitions/field_object"}
                ]
            }
        }
    },
    "definitions":{
        "field_value":{
            "type":["string", "number", "boolean"]
        },
        "field_array":{
            "type":"array",
            "minItems": 1,
            "oneOf": [
                    {"items": {"type":"string"}},
                    {"items": {"type":"number"}},
                    {"items": {"type":"boolean"}}
            ]
        },
        "field_object":{
            "type":"object",
            "required":["value"],
            "properties":{
                "value":{
                    "oneOf": [
                        { "$ref": "#/definitions/field_value" },
                        { "$ref": "#/definitions/field_array" }
                    ]
                },
                "representation":{"type":"string"}
            }
        }
    }
}]]

json = [[{"Timestamp":0, "Type":"foo", "Logger":"bar"}]]

schema = heka_json.parse_schema(schema_json)
assert(schema)

ok, err = pcall(heka_json.parse_schema, "{")
assert(not ok)
assert(err == "failed to parse offset:1 Missing a name for object member.", err)
ok, err = pcall(heka_json.parse, "{")
assert(not ok)
assert(err == "failed to parse offset:1 Missing a name for object member.", err)

doc = heka_json.parse(json)
assert(doc)
assert(doc:validate(schema))
doc1 = heka_json.parse(json)

json = [[{"Timestamp":-1, "Type":"foo", "Logger":"bar"}]]
doc = heka_json.parse(json)
assert(doc)
assert(not doc:validate(schema))

json = [[{"Timestamp":0, "EnvVersion":"unknown"}]]
doc = heka_json.parse(json)
assert(doc)
assert(not doc:validate(schema))

json = [[{"array":[1,"string",true,false,[3,4],{"foo":"bar"},null]}]]
doc = heka_json.parse(json)
assert(doc)
assert(doc:type() == "object")
a = doc:find("array");
assert(doc:type(a) == "array")
str = doc:find(a, 1)
assert("string" == doc:type(str), doc:type(str))
tmp = doc:find("array", 3);
assert("boolean" == doc:type(tmp), doc:type(tmp))
ok, err = pcall(doc.iter, doc, tmp)
assert("iter() not allowed on a primitive type", err, err)
ok, err = pcall(doc.find, doc, doc1, "array")
assert("invalid value" == err, err)
tmp = doc:find("array", "key")
assert(not tmp)
tmp = doc:find("array", 5, 0)
assert(not tmp)
tmp = doc:find("array", 7)
assert(not tmp)
tmp = doc:find(true)
assert(not tmp)
tmp = doc:find("array", 5, "missing")
assert(not tmp)

ra = {
    {value = 1, type = "number"},
    {value = "string", type = "string", len = 6},
    {value = true, type = "boolean"},
    {value = false, type = "boolean"},
    {type = "array", len = 2},
    {type = "object", len = 1},
    {type = "null"},
}

for i,v in doc:iter(a) do
    assert(ra[i+1].type == doc:type(v))
    if ra[i+1].value ~= nil then
        assert(ra[i+1].value == doc:value(v))
    else
        ok, err = pcall(doc.value, doc, v)
        if ra[i+1].type == "null" then
            assert(ok)
        else
            assert(not ok)
        end
    end
    if ra[i+1].len then
        assert(ra[i+1].len == doc:size(v))
    else
        ok, err = pcall(doc.size, doc, v)
        assert(not ok)
    end
end

a0 = doc:find("array", 0);
assert(a0)
assert(doc:value(a0) == 1)

a5 = doc:find("array", 5);
assert(a5)
assert(doc:type(a5) == "object")
assert(doc:size(a5) == 1)
for k,v in doc:iter(a5) do
    assert(doc:type(v) == "string")
    assert(doc:value(v) == "bar")
    assert(k == "foo", tostring(k))
end

a5x = doc:find("array", 5);
assert(a5 == a5x)

ok, doc = pcall(heka_json.parse_message, 1)
assert("bad argument #0 to '?' (invalid number of arguments)" == doc, doc)

ok, doc = pcall(heka_json.parse_message, "", "Fields[json]")
assert("bad argument #1 to '?' (mozsvc.heka_stream_reader expected, got string)" == doc, doc)

hsr = heka_stream_reader.new("test")
hsr:decode_message("\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\4\82\23\10\4\106\115\111\110\16\1\42\13\123\34\102\111\111\34\58\34\98\97\114\34\125")
ok, doc = pcall(heka_json.parse_message, hsr, "Fields[missing]")
assert("field not found" == doc, doc)

ok, doc = pcall(heka_json.parse_message, hsr, "Fields[missing]", "")
assert("bad argument #3 to '?' (number expected, got string)" == doc, doc)

ok, doc = pcall(heka_json.parse_message, hsr, "Fields[missing]", 0, "")
assert("bad argument #4 to '?' (number expected, got string)" == doc, doc)

ok, err = pcall(heka_json.parse_message, hsr, "Fields[json")
assert("field not found" == err, err)

ok, err = pcall(heka_json.parse_message, hsr, "Fieldsjson]")
assert("field not found" ==  err, err)

ok, err = pcall(heka_json.parse_message, hsr, "foo")
assert("field not found" == err, err)

ok, doc = pcall(heka_json.parse_message, hsr, "Fields[json]")
assert(ok, doc)

foo = doc:find("foo")
assert(foo)
assert("bar" == doc:value(foo), tostring(doc:value(foo)))
ok, err = pcall(doc.value, doc, (doc1:find()))
assert(not ok)
assert("invalid value" == err, err)

assert("object" == doc:type(), doc:type());
rdoc = doc:remove()
assert("null" == doc:type(), doc:type());
foo = rdoc:find("foo")
assert("bar" == rdoc:value(foo), tostring(doc:value(foo)))

assert("object" == rdoc:type(), rdoc:type());
nrdoc = rdoc:remove("foo")
assert("object" == rdoc:type(), rdoc:type());

nested = '{"main":{"m1":1}, "payload":{"values":[1,2,3]}}'
json = heka_json.parse(nested)
assert(json)
assert(json:size() == 2, json:size())
values = json:find("payload", "values")
assert(values)
vit = json:iter(values)
i,v = vit()
assert(i == 0, tostring(i))
assert(json:value(v) == 1, tostring(json:value(v)))
i,v = vit()
json:remove("payload")
assert(i == 1, tostring(i))
assert(json:value(v) == 2, tostring(json:value(v)))
i,v = vit()
assert(i == 2, tostring(i))
assert(json:value(v) == 3, tostring(json:value(v)))
i,v = vit()
assert(i == nil, tostring(i))

json = heka_json.parse(nested)
assert(json)
object = json:find("main")
vit = json:iter(object)
json:remove("main")
ok, err = pcall(vit)
assert(err == "iterator has been invalidated")

json = heka_json.parse(nested)
assert(json)
values = json:find("payload", "values")
vit = json:iter(values)
rvalues = json:remove("payload", "values")
ok, err = pcall(vit)
assert(err == "iterator has been invalidated")
schema_array = [[{
"type":"array",
"minItems": 1,
"oneOf": [{"items": {"type":"number"}}]
}]]
sa = heka_json.parse_schema(schema_array)
assert(sa)
assert(rvalues:validate(sa))

gz_nested = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\99\50\66\31\139\8\0\0\0\0\0\0\3\171\86\202\77\204\204\83\178\170\86\202\53\84\178\50\172\213\81\80\42\72\172\204\201\79\76\1\137\149\37\230\148\166\22\43\89\69\27\234\24\233\24\199\214\214\114\1\0\64\251\6\210\48\0\0\0"
hsr:decode_message(gz_nested)
ok, json = pcall(heka_json.parse_message, hsr, "Payload")
assert(ok, json)
values = json:find("payload", "values")
assert(values)

gz_corrupt = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\99\50\66\31\139foobar\0\3\171\86\202\77\204\204\83\178\170\86\202\53\84\178\50\172\213\81\80\42\72\172\204\201\79\76\1\137\149\37\230\148\166\22\43\89\69\27\234\24\233\24\199\214\214\114\1\0\64\251\6\210\48\0\0\0"
hsr:decode_message(gz_corrupt)
ok, json = pcall(heka_json.parse_message, hsr, "Payload")
assert("lsb_ungzip failed" == json,  json)

minimal = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0"
hsr:decode_message(minimal)
ok, err = pcall(heka_json.parse_message, hsr, "Payload")
assert("field not found" == err, err)

invalid_json = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\50\001{"
hsr:decode_message(invalid_json)
ok, err = pcall(heka_json.parse_message, hsr, "Payload")
assert("failed to parse offset:1 Missing a name for object member." == err, err)

short_json = "\10\16\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\16\0\50\1\57"
hsr:decode_message(short_json)
ok, doc = pcall(heka_json.parse_message, hsr, "Payload")
assert(ok, doc)
assert("number" == doc:type(), doc:type())
assert(9 == doc:value(), tostring(doc:value()))
rv = doc:remove("foo")
assert(not rv)

assert(nil == doc:find(nil))
assert(nil == doc:find(nil))
assert(nil == doc:size(nil))
assert(nil == doc:type(nil))
assert(nil == doc:make_field(nil))
assert(nil == doc:iter(nil))
