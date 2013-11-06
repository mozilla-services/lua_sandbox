# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(telemetry)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Lua sandbox")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)

find_package(Git REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(mozsvc)
include(externals)

include_directories("${CMAKE_SOURCE_DIR}/include")

add_subdirectory(src)
