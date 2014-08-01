# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(lua_sandbox C)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Lua sandbox for dynamic data analysis")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 7)
set(CPACK_PACKAGE_VERSION_PATCH 1)

option(LUA_JIT "Enable LuaJIT" off)
if(LUA_JIT)
    add_definitions(-DLUA_JIT)
endif()

find_package(Git REQUIRED)

set(LUA_OUTPUT 64512)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(mozsvc)
include(externals)

include_directories("${CMAKE_SOURCE_DIR}/include")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION modules)

add_subdirectory(src)
