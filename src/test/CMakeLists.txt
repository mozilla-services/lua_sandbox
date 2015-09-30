# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(test_lua_sandbox test_lua_sandbox.c)
target_link_libraries(test_lua_sandbox luasandbox)

add_test(NAME test_move_c_modules COMMAND cmake -E copy_directory ${CMAKE_BINARY_DIR}/ep_base/lib/lua ${CMAKE_CURRENT_BINARY_DIR}/modules)
if(WIN32)
   add_test(NAME test_move_lua_lib COMMAND cmake -E copy
   ${CMAKE_BINARY_DIR}/ep_base/bin/${CMAKE_SHARED_MODULE_PREFIX}luasb${CMAKE_SHARED_MODULE_SUFFIX} ${CMAKE_CURRENT_BINARY_DIR})
   add_test(NAME test_move_sandbox_lib COMMAND cmake -E copy
   ${CMAKE_BINARY_DIR}/src/${CMAKE_SHARED_MODULE_PREFIX}luasandbox${CMAKE_SHARED_MODULE_SUFFIX} ${CMAKE_CURRENT_BINARY_DIR})
elseif(APPLE)
   add_test(NAME test_move_lua_lib COMMAND cmake -E copy_directory
   ${CMAKE_BINARY_DIR}/ep_base/lib ${CMAKE_CURRENT_BINARY_DIR}/../lib)
   add_test(NAME test_move_sandbox_lib COMMAND cmake -E copy
   ${CMAKE_BINARY_DIR}/src/${CMAKE_SHARED_LIBRARY_PREFIX}luasandbox${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/../lib)
endif()

add_test(NAME test_move_lua_modules COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/modules ${CMAKE_CURRENT_BINARY_DIR}/modules)
add_test(NAME test_move_tests COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME test_sandbox COMMAND test_lua_sandbox)
if(APPLE)
	set_tests_properties(test_sandbox PROPERTIES ENVIRONMENT DYLD_LIBRARY_PATH=../lib)
endif()
