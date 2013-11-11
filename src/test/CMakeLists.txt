# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(test_lua_sandbox test_lua_sandbox.c)
target_link_libraries(test_lua_sandbox luasandbox)

add_test(NAME test_sandbox WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND test_lua_sandbox)

install(TARGETS test_lua_sandbox DESTINATION lib)
