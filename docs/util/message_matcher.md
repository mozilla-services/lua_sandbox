# Message Matcher Syntax

The message matcher allows sandboxes to select which messages they want to
consume (see [Heka Message Structure](/heka/message.md))

## Examples

*  Type == "test" && Severity == 6
*  (Severity == 7 || Payload == "Test Payload") && Type == "test"
*  Fields[foo] != "bar"
*  Fields[foo][1][0] == "alternate"
*  Fields[MyBool] == TRUE
*  TRUE
*  Fields[created] =~ "^2015"
*  Fields[string] =~ "foo.example.com"% -- literal pattern vs "foo%.example%.com"
*  Fields[widget] != NIL
*  Timestamp >= "2016-05-24T00:00:00Z"
*  Timestamp >= 1464048000000000000

## Relational Operators

* == equals
* != not equals
* &gt; greater than
* &gt;= greater than equals
* < less than
* <= less than equals
* =~ Lua pattern match
* !~ Lua negated pattern match

## Logical Operators

* Parentheses are used for grouping expressions
* && and (higher precedence)
* || or

## Boolean

* TRUE
* FALSE

## Constants

* NIL used to test the existence (!=) or non-existence (==) of optional headers or field variables

## Message Variables

* All message variables must be on the left hand side of the relational
comparison

### String

* Uuid - 16 byte raw binary type 4 UUID (useful for partitioning data)
* Type
* Logger
* Payload
* EnvVersion
* Hostname

### Numeric

* Timestamp - in addition to nanoseconds since the UNIX epoch an RFC3339 string is also accepted e.g., "2016-05-24T21:51:00Z"
* Severity
* Pid

### Fields

* Fields[_field_name_] - shorthand for Field[_field_name_][0][0]
* Fields[_field_name_][_field_index_] - shorthand for Field[_field_name_][_field_index_][0]
* Fields[_field_name_][_field_index_][_array_index_] the indices are restricted to 0-255
* If a field type is mis-match for the relational comparison, false will be returned e.g., Fields[foo] == 6 where "foo" is a string

## Quoted String

* Single or double quoted strings are allowed
* The maximum string length is 255 bytes

## Lua Pattern Matching Expression

* Patterns are quoted string values
    * An 'escape' pattern modifier of `%` is allowed e.g. `"foo.bar"%` is
    treated as a literal instead of a pattern and behaves like the 'plain'
    option on string.find(). If there are no pattern match characters in the
    string this modifier is set automatically.
* See [Lua Patterns](http://www.lua.org/manual/5.1/manual.html#5.4.1)
* Capture groups are ignored

## Additional Restrictions

* Message matchers are restricted to 128 relational comparisons
* A NUL character '\0' is not allowed in a matcher string
