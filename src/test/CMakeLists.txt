# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(test_luasandbox test_luasandbox.c)
target_link_libraries(test_luasandbox luasandbox luasandboxutil)
set(LIBRARY_PATHS "${CMAKE_BINARY_DIR}/ep_base/lib;${CMAKE_BINARY_DIR}/src;${CMAKE_BINARY_DIR}/src/util")

add_test(NAME test_move_luasandbox_c_modules COMMAND cmake -E copy_directory ${CMAKE_BINARY_DIR}/ep_base/lib/lua ${CMAKE_CURRENT_BINARY_DIR}/modules)
add_test(NAME test_move_luasandbox_lua_modules COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/modules ${CMAKE_CURRENT_BINARY_DIR}/modules)
add_test(NAME test_move_luasandbox_tests COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME test_luasandbox COMMAND test_luasandbox)

if(WIN32)
   STRING(REPLACE ";" "\\\\;" LIBRARY_PATHS "${LIBRARY_PATHS}")
   set_tests_properties(test_luasandbox PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
elseif(APPLE)
   STRING(REPLACE ";" ":" LIBRARY_PATHS "${LIBRARY_PATHS}")
   set_tests_properties(test_luasandbox PROPERTIES ENVIRONMENT DYLD_LIBRARY_PATH=${LIBRARY_PATHS})
endif()

