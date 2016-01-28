# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(luasandbox C)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Lua sandbox for dynamic data analysis")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 12)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_CONTACT "Mike Trinkala <trink@mozilla.com>")

set(CPACK_DEB_COMPONENT_INSTALL ON)

set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_RPM_COMPONENT_INSTALL ON)

set(CPACK_COMPONENTS_ALL core)

option(LUA_JIT "Enable LuaJIT" off)
if(LUA_JIT)
    add_definitions(-DLUA_JIT)
endif()

find_library(LIBM_LIBRARY m)
find_package(Git REQUIRED)
set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_RPM_PACKAGE_PROVIDES "libluasb.so.0()(64bit)")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(mozsvc)
include(externals)

include(CheckFunctionExists)
check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
  add_definitions(-DHAVE_CLOCK_GETTIME)
endif()

set(LIB_INSTALL_DIR lib/)
set(INCLUDE_INSTALL_DIR include/)

include_directories("${EP_BASE}/include")
include_directories("${CMAKE_SOURCE_DIR}/include")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION ${LIB_INSTALL_DIR}${PROJECT_NAME}/modules COMPONENT core)
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION share/doc/${PROJECT_NAME} COMPONENT core)

if(WIN32 AND NOT CYGWIN)
  set(INSTALL_CMAKE_DIR cmake)
else()
  set(INSTALL_CMAKE_DIR ${LIB_INSTALL_DIR}${CMAKE_PROJECT_NAME}/cmake)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/luasandboxConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfig.cmake
                              INSTALL_DESTINATION ${INSTALL_CMAKE_DIR}
                              PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfigVersion.cmake
                                 VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                                 COMPATIBILITY AnyNewerVersion ) # todo change to SameMajorVersion after 1.0
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfigVersion.cmake
        DESTINATION ${INSTALL_CMAKE_DIR} COMPONENT core)

add_subdirectory(src)
