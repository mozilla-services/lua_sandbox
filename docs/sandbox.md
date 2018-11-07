# Generic Sandbox Interface

## Configuration

* **input_limit** - the largest input string that is allowed to be processed
  (bytes (unsigned), default 65536, 0 for unlimited*). This is not directly used
  by the sandbox it is made availble to modules to standardize the configuration.
* **output_limit** - the largest output string an input or analysis plugin can
  inject into the host (bytes (unsigned), default 65536, 0 for unlimited*)
* **memory_limit** - the maximum amount of memory a plugin can use before being
  terminated (bytes (unsigned), default 8388608, 0 for unlimited*)
* **instruction_limit** - the maximum number of Lua instructions a plugin can
  execute in a single API function call (count (unsigned), default 1000000, 0
  for unlimited)
* **path** - The path used by require to search for a Lua loader. See
  [package loaders](http://www.lua.org/manual/5.1/manual.html#pdf-package.loaders)
  for the path syntax.  By default no paths are set in the sandbox and
  everything has been moved to a sandbox configuration table.
* **cpath** - The path used by require to search for a C loader (same notes as
  above)
* **disabled_modules** - Hash specifying which modules should be completely
  inaccessible.  The existence of the key in the table will disable the module.
```lua
disabled_modules = {io = 1}
```
* **remove_entries** - Hash specifying which functions within a module should be
  inaccessible
```lua
remove_entries = {
    os = {"getenv", "execute"},
    string = {"dump"}
}
```
* **log_level** - syslog severity level, when set to  debug (7) the print
  function will be wired to the specified logger (default error (3))
* *user defined*  any other variable (string, bool, number, table) is passed
  through as-is and available via [read_config](#readconfig)

*_0 == SIZE_MAX which in not necessarily the upper limit of the
configuration range UINT_MAX_

## Lua functions exposed to C by the core sandbox

There are no functions exposed by default, see lsb_pcall_setup() and
lsb_pcall_teardown() when defining an API.

## Functions exposed to Lua by the core sandbox

### require

By default only the base library is loaded, additional libraries must be loaded
with require().

*Arguments*

- libraryName (string)

*Return*
- a table - For non user provided libraries the table is also globally
  registered with the library name.  User provided libraries may implement their
  own semantics i.e. the grammar libraries return a table but do not globally
  register the table name (see the individual module documentation for the
  correct usage).

*Notes*

The following modules have been modified, as described, for use in the sandbox.
  - [base library](http://www.lua.org/manual/5.1/manual.html#5.1)
    - The require() function has been modified to not expose any of the package
      table to the sandbox.
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

## Special Lua Global Variables

- **_PRESERVATION_VERSION** (number) This variable is examined during state
restoration; if the previous version does not match the current version the
restoration is aborted and the sandbox starts cleanly. The version should be
incremented any time an incompatible change is made to the global data schema.

When versioning is required, the use of a configuration variable name
`preservation_version` with the following syntax is recommended.
```lua
-- initial (allows the user to bump the version due to a cfg change)
_PRESERVATION_VERSION = read_config("preservation_version") or 0

-- if the plugin code alters the global data schema in an incompatible way this
-- ensures the state is properly cleared when there is a user provided version.
_PRESERVATION_VERSION = (read_config("preservation_version") or 0) + 1
```

## How to interact with the sandbox (creating an API)

The best place to start is with some examples:

### Unit Test API

[Unit Test API](https://mozilla-services.github.io/lua_sandbox/doxygen/test_2sandbox_8h.html)

#### Lua Functions Exposed to C

- int **process** (double)
    - exposes a process function that takes a test case number as its argument
      and returns and integer result code.
- void **report** (double)
    - exposes a report function that takes a test case number as its argument
      and returns nothing.

#### C Functions Exposed to Lua

- void **write_output** (optionalArg1, optionalArg2, optionalArgN)
    - captures whatever is in the output buffer for use by the host application,
      appending any optional arguments (optional arguments have the same
      restriction as output).

### Heka Sandbox API

[Heka Sandbox API](https://mozilla-services.github.io/lua_sandbox/doxygen/heka_2sandbox_8h.html)
