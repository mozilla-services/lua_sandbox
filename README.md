Prerequisites {#mainpage}
====
* C compiler
* CMake (2.8.7+) - http://cmake.org/cmake/resources/software.html

Optional (used for documentation)
----
* Graphviz (2.28.0) - http://graphviz.org/Download..php
* Doxygen (1.8+)- http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc

lua_sandbox  - UNIX Build Instructions
====
    git clone https://github.com/mozilla-services/lua_sandbox.git
    cd lua_sandbox 
    mkdir release
    cd release
    cmake -DCMAKE_BUILD_TYPE=release ..
    make
    ctest

lua_sandbox  - Windows Build Instructions
====
    git clone https://github.com/mozilla-services/lua_sandbox.git
    cd lua_sandbox 
    mkdir release
    cd release
    cmake -DCMAKE_BUILD_TYPE=release -G "NMake Makefiles" ..
    nmake
    ctest

