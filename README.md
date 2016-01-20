Lua Sandbox Library
-------------------

## Overview

Sandboxes provide a dynamic and isolated execution environment
for data parsing, transformation, and analysis.  They allow access to data
without jeopardizing the integrity or performance of the processing
infrastructure. This broadens the audience that the data can be
exposed to and facilitates new uses of the data (i.e. debugging, monitoring,
dynamic provisioning,  SLA analysis, intrusion detection, ad-hoc reporting,
etc.)

The Lua sandbox is a library allowing customized control over the Lua execution
environment including functionality like global data preservation/restoration on
shutdown/startup, output collection in textual or binary formats and an array of
parsers for various data types (Nginx, Apache, Syslog, MySQL and many RFC grammars)

### Features

- small - memory requirements are as little as 8 KiB for a basic sandbox
- fast - microsecond execution times
- stateful - ability to resume where it left off after a restart/reboot
- isolated - failures are contained and malfunctioning sandboxes are terminated.
  Containment is defined in terms of restriction to the operating system,
  file system, libraries, memory use, Lua instruction use, and output size.

[Full Documentation](docs/index.md)

## Installation

### Prerequisites
* C compiler (GCC 4.7+, Visual Studio 2013)
* CMake (3.0+) - http://cmake.org/cmake/resources/software.html
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

    ctest
