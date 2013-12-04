Prerequisites {#mainpage}
====
* C compiler (GCC 4.7+, Visual Studio 2013 (LuaJIT), MinGW (Lua 5.1))
* CMake (2.8.7+) - http://cmake.org/cmake/resources/software.html
* Git http://git-scm.com/download

Optional (used for documentation)
----
* Graphviz (2.28.0) - http://graphviz.org/Download..php
* Doxygen (1.8+)- http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc

lua_sandbox  - UNIX Build Instructions
----

    git clone https://github.com/mozilla-services/lua_sandbox.git
    cd lua_sandbox 
    mkdir release
    cd release
    cmake -DCMAKE_BUILD_TYPE=release ..
    make
    ctest

lua_sandbox  - Windows Build Instructions
----

    # in a VS2013 command prompt window

    git clone https://github.com/mozilla-services/lua_sandbox.git
    cd lua_sandbox 
    mkdir release
    cd release
    cmake -DCMAKE_BUILD_TYPE=release -G "NMake Makefiles" ..
    nmake

    # To run the tests you must install
    cmake -DCMAKE_INSTALL_PREFIX="" ..
    nmake install DESTDIR=test
    cd ..\src\test
    ..\..\release\test\lib\test_lua_sandbox.exe 

