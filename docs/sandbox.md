# Generic Sandbox Interface

## Configuration

* **output_limit** - the largest output string an input or analysis plugin can inject into the host (bytes, default 64KiB)
* **memory_limit** - the maximum amount of memory a plugin can use before being terminated (bytes, default 8MiB)
* **instruction_limit** - the maximum number of Lua instructions a plugin can execute in a single API function call (count, default 1MM)
* **path** - The path used by require to search for a Lua loader. See [package loaders](http://www.lua.org/manual/5.1/manual.html#pdf-package.loaders)
  for the path syntax.  By default no paths are set in the sandbox and everything has been moved to a sandbox configuration table.
* **cpath** - The path used by require to search for a C loader (same notes as above)
* **disabled_modules** - Hash specifying which modules should be completely inaccessible.  The existence of the key in the table will
  disable the module.
```lua
disabled_modules = {io = 1}
```
* **remove_entries** - Hash specifying which functions within a module should be inaccessible.
```lua
remove_entries = {
    os = {"getenv", "execute"},
    string = {"dump"}
}
```
* **log_level** - Integer specifying the syslog severity level, when set to debug (7) the print function will be wired to the specified logger
* *user defined*  any other variable (string, bool, number, table) is passed through as-is and available via [read_config](#read_config)

## Lua functions exposed to C by the core sandbox

There are no functions exposed by default, see lsb_pcall_setup() and
lsb_pcall_teardown() when defining an API.

## Functions exposed to Lua by the core sandbox

### require

By default only the base library is loaded additional libraries must be loaded with require().

*Arguments*

- libraryName (string)

*Return*
- a table - For non user provided libraries the table is also globally registered
    with the library name.  User provided libraries may implement there own semantics
    i.e. the grammar libraries return a table but do not globally register the table name
    (see the individual module documentation for the correct usage).

*Notes*

The following modules have been modified, as described, for use in the sandbox.
  - [base library](http://www.lua.org/manual/5.1/manual.html#5.1)
    - The require() function has been modified to not expose any of the package table to the sandbox.
  - [math](http://www.lua.org/manual/5.1/manual.html#5.6)
    - Added Functions
        - erf(x) - Returns the error function value for x.
        - erfc(x)- Returns the complementary error function value for x.
  - [os](http://www.lua.org/manual/5.1/manual.html#5.8)
    - The local timezone is set to UTC in all sandboxes.

### read_config

Provides access to the sandbox configuration variables.

*Arguments*
* key (string) - configuration key name

*Return*
* value (string, number, bool, table)

### output
Receives any number of arguments and appends data to the output buffer, which
cannot exceed the output_limit configuration parameter. See lsb_get_output() to
connect the output to the host application.

*Arguments*
- arg (number, string, bool, nil, userdata implementing output support) - Lua 
  variable or literal to be appended the output buffer

*Return*
- none

### print
Receives any number of arguments and sends a debug message to the host's logger
function as a tab delimited message. The function clears and then uses the
output buffer so if pending output has been queued and not flushed it will be
lost (the same output_limit restrictions apply).  Non-printable characters
are replaced with spaces to preserve the host's log integrity and any embedded
NULs terminate each argument.

*Arguments*
- arg (anything that can be converted to a string with the tostring function)

*Return*
- none

**Note:** To extend the function set exposed to Lua see lsb_add_function()

## How to interact with the sandbox (creating an API)

The best place to start is with some examples:

### Unit Test API

[Unit Test API](/lua_sandbox/doxygen/test_2sandbox_8h.html)

#### Lua Functions Exposed to C

- int **process** (double)
    - exposes a process function that takes a test case number as its argument and returns and integer result code.
- void **report** (double)
    - exposes a report function that takes a test case number as its argument and returns nothing.

#### C Functions Exposed to Lua

- void **write_output** (optionalArg1, optionalArg2, optionalArgN)
    - captures whatever is in the output buffer for use by the host application, appending any optional arguments
    (optional arguments have the same restriction as output).

### Heka Sandbox API

[Heka Sandbox API](/lua_sandbox/doxygen/heka_2sandbox_8h.html)

