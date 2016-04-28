# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(luasandbox C)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Lua sandbox for dynamic data analysis")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 10)
set(CPACK_PACKAGE_VERSION_PATCH 2)
set(CPACK_PACKAGE_CONTACT "Mike Trinkala <trink@mozilla.com>")

set(CPACK_DEB_COMPONENT_INSTALL ON)

set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_RPM_COMPONENT_INSTALL ON)

set(CPACK_COMPONENTS_ALL core)

option(LUA_JIT "Enable LuaJIT" off)
if(LUA_JIT)
    add_definitions(-DLUA_JIT)
endif()

find_package(Git REQUIRED)
set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_RPM_PACKAGE_PROVIDES "libluasb.so.0()(64bit)")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(mozsvc)
include(externals)

include_directories("${EP_BASE}/include")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION lib/${PROJECT_NAME}/modules COMPONENT core)
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION share/doc/${PROJECT_NAME} COMPONENT core)

add_subdirectory(src)
add_custom_target(copy_includes
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/include ${EP_BASE}/include)
add_dependencies(luasandbox copy_includes)
