# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

configure_file(test.h.in test.h ESCAPE_QUOTES)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(LIBRARY_PATHS "${CMAKE_BINARY_DIR}/ep_base/${LIB_DIR};${CMAKE_BINARY_DIR}/src/util;${CMAKE_BINARY_DIR}/src;${CMAKE_BINARY_DIR}/src/heka")

add_test(NAME test_move_heka_sandbox_tests COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_heka_sandbox test_sandbox.c)
target_link_libraries(test_heka_sandbox luasandboxheka luasandbox)
add_test(NAME test_heka_sandbox COMMAND test_heka_sandbox)

add_executable(test_message_matcher test_message_matcher.c)
target_link_libraries(test_message_matcher luasandboxheka luasandboxutil)
add_test(NAME test_message_matcher COMMAND test_message_matcher)

if(WIN32)
   STRING(REPLACE ";" "\\\\;" LIBRARY_PATHS "${LIBRARY_PATHS}")
   set_tests_properties(test_heka_sandbox PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_message_matcher PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
elseif(APPLE)
   STRING(REPLACE ";" ":" LIBRARY_PATHS "${LIBRARY_PATHS}")
   set_tests_properties(test_heka_sandbox PROPERTIES ENVIRONMENT DYLD_LIBRARY_PATH=${LIBRARY_PATHS})
   set_tests_properties(test_message_matcher PROPERTIES ENVIRONMENT DYLD_LIBRARY_PATH=${LIBRARY_PATHS})
endif()
