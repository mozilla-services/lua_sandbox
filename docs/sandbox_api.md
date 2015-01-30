Sandbox API
===========

Sandboxes provide a dynamic and isolated execution environment
for data parsing, transformation, and analysis.  They allow access to data
without jeopardizing the integrity or performance of the processing
infrastructure. This broadens the audience that the data can be
exposed to and facilitates new uses of the data (i.e. debugging, monitoring,
dynamic provisioning,  SLA analysis, intrusion detection, ad-hoc reporting,
etc.)

Features
========
- small - memory requirements are as little as 6 KiB for a basic sandbox
- fast - microsecond execution times
- stateful - ability to resume where it left off after a restart/reboot
- isolated - failures are contained and malfunctioning sandboxes are terminated.
  Containment is defined in terms of restriction to the operating system,
  file system, libraries, memory use, Lua instruction use, and output size.

Functions exposed to Lua by the core sandbox
============================================
**require(libraryName)**

By default only the base library is loaded additional libraries must be loaded with require().

*Arguments*

- libraryName (string)
  - [base library(http://www.lua.org/manual/5.1/manual.html#5.1)
    - The require() function has been modified to not expose any of the package table to the sandbox.
    - Disabled functions (default): collectgarbage, coroutine, dofile, load, loadfile, loadstring, print.
  - [bloom_filter](https://github.com/mozilla-services/lua_bloom_filter/blob/master/README.md)
  - [circular_buffer](https://github.com/mozilla-services/lua_circular_buffer/blob/master/README.md)
  - [cjson](http://www.kyne.com.au/~mark/software/lua-cjson-manual.html) With the following modifications:
    - Loads the cjson module in a global cjson table
    - The encode buffer is limited to the sandbox output_limit.
    - The decode buffer will be roughly limited to one half of the sandbox memory_limit.
    - The NULL value is not decoded to cjson.null it is simply discarded.
      If the original behavior is desired use cjson.decode_null(true) to enable NULL decoding.
    - The new() function has been disabled so only a single cjson parser can be created.
    - The encode_keep_buffer() function has been disabled (the buffer is always reused).
  - [hyperloglog](https://github.com/mozilla-services/lua_hyperloglog/blob/master/README.md)
  - [lpeg](http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html) Lua Parsing Expression Grammar Library
  - [re](http://www.inf.puc-rio.br/~roberto/lpeg/re.html) Regex syntax for LPEG
  - [math](http://www.lua.org/manual/5.1/manual.html#5.6)
  - [os](http://www.lua.org/manual/5.1/manual.html#5.8)
    - The local timezone is set to UTC in all sandboxes.
    - Disabled functions (default): execute, exit, remove, rename, setlocale, tmpname.
  - [strict](http://www.lua.org/extras/5.2/strict.lua) Checks for use of undeclared global variables.
  - [string](http://www.lua.org/manual/5.1/manual.html#5.4)
  - [struct](http://www.inf.puc-rio.br/~roberto/struct/)
  - [table](http://www.lua.org/manual/5.1/manual.html#5.5)
  - _user provided_ (lua, so/dll)

*Return*
- a table - For non user provided libraries the table is also globally registered
    with the library name.  User provided libraries may implement there own semantics
    i.e. the grammar libraries return a table but do not globally register the table name
    (see the individual module documentation for the correct usage).

____
**output(arg0, arg1, ...argN)**
    lsb_appends data to the output buffer, which cannot exceed the output_limit
    configuration parameter. See lsb_get_output() to connect the output to the
    host application.

*Arguments*
- arg (number, string, bool, nil, circular_buffer) Lua variable or literal to be appended the output buffer

*Return*
- none

**Note:** To extend the function set exposed to Lua see lsb_add_function()


Lua functions exposed to C by the core sandbox
==============================================
There are no functions exposed by default, see lsb_pcall_setup() and
lsb_pcall_teardown() when defining an API.

How to interact with the sandbox (creating an API)
==================================================
The best place to start is with some examples:

Unit Test API
=============
Lua Functions Exposed to C
--------------------------
- int **process** (double)
    - exposes a process function that takes a test case number as its argument and returns and integer result code.
- void **report** (double)
    - exposes a report function that takes a test case number as its argument and returns nothing.

C Functions Exposed to Lua
--------------------------
- void **write_output** (optionalArg1, optionalArg2, optionalArgN)
    - captures whatever is in the output buffer for use by the host application, appending any optional arguments
    (optional arguments have the same restriction as output).
- void **write_message** (table)
    - serializes the Lua table into a Heka protobuf message and captures the output.
    - the table must use the following [schema](https://hekad.readthedocs.org/en/latest/sandbox/index.html#sample-lua-message-structure).

[Unit Test Source Code](https://github.com/mozilla-services/lua_sandbox/blob/master/src/test/test_lua_sandbox.c)

Heka Sandbox API
================
[Heka Sandbox](https://hekad.readthedocs.org/en/latest/sandbox/index.html#lua-sandbox)
