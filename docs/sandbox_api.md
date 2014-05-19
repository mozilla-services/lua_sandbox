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

By default only the base library is loaded additional libraries must be explicitly specified.

*Arguments*

- libraryName (string)

  - [bloom_filter](bloom_filter.md) 
  - [circular_buffer](circular_buffer.md) 
  - **cjson** loads the cjson.safe module in a global cjson table, exposing the decoding functions only. http://www.kyne.com.au/~mark/software/lua-cjson-manual.html.
  - [hyperloglog](hyperloglog.md) 
  - **lpeg** loads the Lua Parsing Expression Grammar Library http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html
  - **math**
  - **os**
  - **string**
  - **table**
  - _user provided_

*Return*
- a table - For non user provided libraries the table is also globally registered 
    with the library name.  User provided libraries may implement there own semantics 
    i.e. the grammar libraries return a table but do not globally register the table name
    (see the individual module documentation for the correct usage).

____
**output(arg0, arg1, ...argN)**
    Appends data to the output buffer, which cannot exceed the output_limit 
    configuration parameter. See lsb_get_output() to connect the output to the 
    host application.

*Arguments*
- arg (number, string, bool, nil, table, circular_buffer) Lua variable or literal to be appended the output buffer

*Return*
- none

*Notes*
- Outputting a Lua table will serialize it to JSON according to the following 
guidelines/restrictions:
    - Tables cannot contain internal of circular references.
    - Keys starting with an underscore are considered private and will not
    be serialized.
    - Arrays only use contiguous numeric keys starting with an index of 1.
    Private keys are the exception i.e. local a = {1,2,3,_hidden="private"}
    will be serialized as: ``[1,2,3]\n``
    - Hashes only use string keys (numeric keys will not be quoted and the
    JSON output will be invalid). Note: the hash keys are output in an arbitrary
    order i.e. local a = {x = 1, y = 2} will be serialized as: `{"y":2,"x":1}\n`.

**Note:** To extend the function set exposed to Lua see lsb_add_function()


Lua functions exposed to C by the core sandbox
==============================================
There an no functions exposed by default, see lsb_pcall_setup() and 
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
- void **write** ()
    - captures whatever is in the output buffer for use by the host application.
- void **write** (circular_buffer)
    - serializes the circular buffer and captures the output.
- void **write** (table)
    - serializes the Lua table into a Heka protobuf message and captures the output.

[Unit Test Source Code](https://github.com/mozilla-services/lua_sandbox/blob/master/src/test/test_lua_sandbox.c)

Heka Sandbox API
================
[Heka Sandbox](https://hekad.readthedocs.org/en/latest/sandbox/index.html#lua-sandbox)
