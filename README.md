Lua Sandbox Library {#mainpage}
-------------------

## Overview

Sandboxes provide a dynamic and isolated execution environment
for data parsing, transformation, and analysis.  They allow access to data
without jeopardizing the integrity or performance of the processing
infrastructure. This broadens the audience that the data can be
exposed to and facilitates new uses of the data (i.e. debugging, monitoring,
dynamic provisioning,  SLA analysis, intrusion detection, ad-hoc reporting,
etc.)

The Lua sandbox is library allowing customized control over the Lua execution
environment including functionality like global data preservation/restoration on
shutdown/startup, output collection in textual, binary or [Heka protobuf format]
(https://hekad.readthedocs.org/en/latest/sandbox/index.html#sample-lua-message-structure)
and an array of parsers for various data types (Nginx, Apache, Syslog, MySQL and
many RFC grammars)

### Features

- small - memory requirements are as little as 6 KiB for a basic sandbox
- fast - microsecond execution times
- stateful - ability to resume where it left off after a restart/reboot
- isolated - failures are contained and malfunctioning sandboxes are terminated.
  Containment is defined in terms of restriction to the operating system,
  file system, libraries, memory use, Lua instruction use, and output size.

## Installation

### Prerequisites
* C compiler (GCC 4.7+, Visual Studio 2013 (LuaJIT), MinGW (Lua 5.1))
* CMake (2.8.7+) - http://cmake.org/cmake/resources/software.html
* Git http://git-scm.com/download

#### Optional (used for documentation)
* Graphviz (2.28.0) - http://graphviz.org/Download..php
* Doxygen (1.8+)- http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc

### CMake Build Instructions

    git clone https://github.com/mozilla-services/lua_sandbox.git
    cd lua_sandbox
    mkdir release
    cd release

    # UNIX
    cmake -DCMAKE_BUILD_TYPE=release ..
    make

    # Windows Visual Studio 2013
    cmake -DCMAKE_BUILD_TYPE=release -G "NMake Makefiles" ..
    nmake

    # Windows MinGW
    cmake -DCMAKE_BUILD_TYPE=release -G "MinGW Makefiles" ..
    mingw32-make

    ctest

## Sandbox API

### Lua functions exposed to C by the core sandbox

There are no functions exposed by default, see lsb_pcall_setup() and
lsb_pcall_teardown() when defining an API.

### Functions exposed to Lua by the core sandbox
**require(libraryName)**

By default only the base library is loaded additional libraries must be loaded with require().

*Arguments*

- libraryName (string)
  - [base library](http://www.lua.org/manual/5.1/manual.html#5.1)
    - The require() function has been modified to not expose any of the package table to the sandbox.
    - Disabled functions (default): collectgarbage, coroutine, dofile, load, loadfile, loadstring, newproxy, print.
  - [bloom_filter](https://github.com/mozilla-services/lua_bloom_filter/blob/master/README.md) Test whether an element is a member of a set
  - [circular_buffer](https://github.com/mozilla-services/lua_circular_buffer/blob/master/README.md) In memory time series data base and analysis
  - [cjson](http://www.kyne.com.au/~mark/software/lua-cjson-manual.html) JSON parser with the following modifications:
    - Loads the cjson module in a global cjson table
    - The encode buffer is limited to the sandbox output_limit.
    - The decode buffer will be roughly limited to one half of the sandbox memory_limit.
    - The NULL value is not decoded to cjson.null it is simply discarded.
      If the original behavior is desired use cjson.decode_null(true) to enable NULL decoding.
    - The new() function has been disabled so only a single cjson parser can be created.
    - The encode_keep_buffer() function has been disabled (the buffer is always reused).
  - [cuckoo_filter](https://github.com/mozilla-services/lua_cuckoo_filter/blob/master/README.md) Bloom filter alternative supporting deletions
  - [hyperloglog](https://github.com/mozilla-services/lua_hyperloglog/blob/master/README.md) Efficiently count the number of elements in a set 
  - [lpeg](http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html) Lua Parsing Expression Grammar Library
  - [re](http://www.inf.puc-rio.br/~roberto/lpeg/re.html) Regex syntax for LPEG
  - [math](http://www.lua.org/manual/5.1/manual.html#5.6)
  - [os](http://www.lua.org/manual/5.1/manual.html#5.8)
    - The local timezone is set to UTC in all sandboxes.
    - Disabled functions (default): getenv, execute, exit, remove, rename, setlocale, tmpname.
  - [sax](https://github.com/trink/symtseries/blob/master/README.md) Symbolic time series data analysis based on
    [Symbolic Aggregate approXimation](http://www.cs.ucr.edu/~eamonn/SAX.pdf).
  - [string](http://www.lua.org/manual/5.1/manual.html#5.4)
  - [struct](http://www.inf.puc-rio.br/~roberto/struct/) Converts data to/from C structs
  - [table](http://www.lua.org/manual/5.1/manual.html#5.5)
  - Grammar Libraries
    - cbufd - Parses the circular buffer library delta output (use for data aggregation)
    - common_log_format - Nginx and Apache meta grammar generators (creates a grammar based on the log_format configuration)
    - data_time - RFC3339, RFC3164, strftime, common log format, MySQL and Postgres timestamps
    - ip_address - IPv4 and IPv6 address
    - mysql - MySQL and MariaDB slow query and short slow query parsers
    - postfix - Postfix messages
    - syslog - Rsyslog meta grammar generator (creates a grammar based on the template configuration)
    - syslog_message - Syslog messages
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

## How to interact with the sandbox (creating an API)

The best place to start is with some examples:

### Unit Test API

[Unit Test Source Code](https://github.com/mozilla-services/lua_sandbox/blob/master/src/test/test_lua_sandbox.c)

#### Lua Functions Exposed to C

- int **process** (double)
    - exposes a process function that takes a test case number as its argument and returns and integer result code.
- void **report** (double)
    - exposes a report function that takes a test case number as its argument and returns nothing.

#### C Functions Exposed to Lua

- void **write_output** (optionalArg1, optionalArg2, optionalArgN)
    - captures whatever is in the output buffer for use by the host application, appending any optional arguments
    (optional arguments have the same restriction as output).
- void **write_message** (table)
    - serializes the Lua table into a Heka protobuf message and captures the output.
    - the table must use the following [schema](https://hekad.readthedocs.org/en/latest/sandbox/index.html#sample-lua-message-structure).

### Heka Sandbox API

[Heka Sandbox](https://hekad.readthedocs.org/en/latest/sandbox/index.html#lua-sandbox)
