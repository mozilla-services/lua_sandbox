# Heka JSON Module

Allows for JSON-Schema validation and more efficient manipulation of large JSON
structures where only a small amount of the data is consumed by the plugin.
Schema pattern matching is restricted to a subset of regex decribed in the
Regular Expression section of the
[RapidJSON Schema Documentation](http://rapidjson.org/md_doc_schema.html).

## Availability

Input/Output plugins only.

## API Functions

### parse

Creates a Heka JSON Document from a string.

```lua
local ok, doc = pcall(heka_json.parse, '{"foo":"bar"}')
assert(ok, doc)

```
*Arguments*
* JSON (string) - JSON string to parse

*Return*
* doc (userdata) - Heka JSON document or an error is thrown

### parse_schema

Creates a Heka JSON Schema.

```lua
local ok, doc = pcall(heka_json.parse_schema, '{"type":"array","minItems": 1,"oneOf": [{"items": {"type":"number"}}]}')
assert(ok, doc)

```
*Arguments*
* JSON (string) - JSON schema string to parse

*Return*
* schema (userdata) - Heka JSON schema or an error is thrown

### parse_message

Creates a Heka JSON Document from a message variable.

```lua
local ok, doc = pcall(heka_json.parse_message, "Fields[myjson]")
assert(ok, doc)

```
*Arguments*
* heka_stream_reader (userdata) - require only for Input plugins since there is
  no active message available.
* variableName (string)
  * Payload
  * Fields[*name*]
* fieldIndex (unsigned) - optional, only used in combination with the Fields
  variableName use to retrieve a specific instance of a repeated field name; 
  zero indexed
* arrayIndex (unsigned) - optional, only used in combination with the Fields
  variableName use to retrieve a specific element out of a field containing an
  array; zero indexed

*Return*
* doc (userdata) - Heka JSON document or an error is thrown

## Heka JSON Document API Methods

### validate

Checks that the JSON document conforms to the specified schema.

```lua
local ok, err = doc:validate(schema)
assert(ok, err)

```
*Arguments*
* heka_schema (userdata) - a compiled schema to validate against

*Return*
* ok (bool) - true if valid
* err (string) - error message on failure

### find

Searches for and returns a value in the JSON structure.

```lua
local v = doc:find("obj", "arr", 0, "foo")
assert(v, "not found")

```
*Arguments*
* value (lightuserdata) - optional, when not specified the function is applied to document
* key (string, number) - object key, or array index
* keyN (string, number) - final object key, or array index

*Return*
* value (lightuserdata) - handle to be passed to other methods, nil if not found

### remove

Searches for and NULL's out the resulting value in the JSON structure returning the
extracted Heka JSON document.

```lua
local rv = doc:remove("obj", "arr")
assert(rv, "not found")
rv:size() -- number of elements in the extracted array

```
*Arguments*
* value (lightuserdata) - optional, when not specified the function is applied to document
* key (string, number) - object key, or array index
* keyN (string, number) - final object key, or array index

*Return*
* doc (userdata) - the object removed from the original JSON or nil

### value

Returns the primitive value of the JSON element.

```lua
local v = doc:find("obj", "arr", 0, "foo")
local str = doc:value(v)
assert("bar" == str, tostring(str))

```
*Arguments*
* value (lightuserdata, nil) - optional, when not specified the function is applied to document
  (accepts nil for easier nesting without having to test the inner expression)
  e.g., str = doc:value(doc:find("foo")) or "my default"

*Return*
* primitive - string, number, bool, nil or throws an error if not convertable (object, array)

### type

Returns the type of the value in the JSON structure.

```lua
local t = doc:type()
assert(t == "object", t)

```
*Arguments*
* value (lightuserdata, nil) - optional, when not specified the function is applied to document
  (accepts nil for easier nesting without having to test the inner expression)

*Return*
* type (string, nil) - "string", "number", "boolean", "object", "array" or "null"

### iter

Retrieves an interator function for an object/array.

```lua
local v = doc:find("obj", "arr")
for i,v in doc:iter(v) do
-- ...
end
```
*Arguments*
* value (lightuserdata, nil) - optional, when not specified the function is applied to document
  (accepts nil for API consistency)

*Return*
* iter (function, nil) - iterator function returning an index/value for arrays or a key/value for
  objects.  Throws an error on primitive types.

### size

Returns the size of the value.
```lua
local v = doc:find("obj", "arr")
local n = doc:size(v)

```
*Arguments*
* value (lightuserdata, nil) - optional, when not specified the function is applied to document
  (accepts nil for easier nesting without having to test the inner expression)

*Return*
* size (number, nil) - Number of element in an array/object or the length of the string.
  Throws an error on numeric, boolean and null types.

### make_field

Helper function to wrap the lightuserdata so it can be used in an inject_message field.

```lua
local msg = {Fields = {}}
local v = doc:find("obj", "arr")
msg.Fields.array = doc:make_field(v)  -- set array to the JSON string representation of "arr"
inject_message(msg)

```
*Arguments*
* value (lightuserdata, nil) - optional, when not specified the function is applied to document
  (accepts nil for easier nesting without having to test the inner expression)

*Return*
* field (table, nil) - i.e., `{value = v, userdata = doc}`
