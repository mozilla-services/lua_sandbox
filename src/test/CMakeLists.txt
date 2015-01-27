# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(test_lua_sandbox test_lua_sandbox.c)
target_link_libraries(test_lua_sandbox luasandbox)

add_test(NAME test_setup_cmod COMMAND cmake -E copy_directory ${CMAKE_BINARY_DIR}/ep_base/modules ${CMAKE_CURRENT_SOURCE_DIR}/modules)
if(WIN32)
   add_test(NAME test_setup_lib COMMAND cmake -E copy_directory ${CMAKE_BINARY_DIR}/ep_base/lib ${CMAKE_CURRENT_SOURCE_DIR})
   if(MSVC)
       set(_DLL_NAME "luasandbox.dll")
   else()
       set(_DLL_NAME "libluasandbox.dll")
   endif()
   add_test(NAME test_setup_target COMMAND cmake -E copy ${CMAKE_BINARY_DIR}/src/${_DLL_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_test(NAME test_setup_lua COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/modules ${CMAKE_CURRENT_SOURCE_DIR}/modules)
add_test(NAME test_sandbox WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND test_lua_sandbox)

install(TARGETS test_lua_sandbox DESTINATION lib)
