# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(luasandbox C)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Lua sandbox for dynamic data analysis")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 24)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_CONTACT "Mike Trinkala <trink@mozilla.com>")

include(GNUInstallDirs)
if(WIN32)
  set(INSTALL_CMAKE_DIR cmake)
  set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR})
else()
  set(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/cmake")
endif()

set(CPACK_DEB_COMPONENT_INSTALL ON)

set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_RPM_COMPONENT_INSTALL ON)
if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
  set(CPACK_RPM_PACKAGE_PROVIDES "libluasb.so.0()(64bit)")
else()
  set(CPACK_RPM_PACKAGE_PROVIDES "libluasb.so.0")
endif()

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

include_directories("${EP_BASE}/inst/include")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/modules COMPONENT core)
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT core)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/sandboxes/" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/sandboxes COMPONENT core)

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/luasandboxConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfig.cmake
                              INSTALL_DESTINATION ${INSTALL_CMAKE_DIR}
                              PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfigVersion.cmake
                                 VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                                 COMPATIBILITY AnyNewerVersion ) # todo change to SameMajorVersion after 1.0
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/luasandboxConfigVersion.cmake
        DESTINATION ${INSTALL_CMAKE_DIR} COMPONENT core)

add_subdirectory(src)
add_custom_target(copy_includes
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/include ${EP_BASE}/inst/include)
add_dependencies(${LUA_PROJECT} copy_includes)
add_dependencies(luasandboxutil ${LUA_PROJECT})
add_dependencies(luasandbox luasandboxutil)
add_dependencies(luasandboxtest luasandbox)
add_dependencies(luasandboxheka luasandboxtest)
