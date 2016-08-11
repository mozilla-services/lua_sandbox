# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(test_input_buffer test_input_buffer.c)
target_link_libraries(test_input_buffer luasandboxutil)
add_test(NAME test_input_buffer COMMAND test_input_buffer)

add_executable(test_output_buffer test_output_buffer.c)
target_link_libraries(test_output_buffer luasandboxutil)
add_test(NAME test_output_buffer COMMAND test_output_buffer)

add_executable(test_protobuf test_protobuf.c)
target_link_libraries(test_protobuf luasandboxutil)
add_test(NAME test_protobuf COMMAND test_protobuf)

add_executable(test_string_matcher test_string_matcher.c)
target_link_libraries(test_string_matcher luasandboxutil)
add_test(NAME test_string_matcher COMMAND test_string_matcher)

add_executable(test_running_stats test_running_stats.c)
target_link_libraries(test_running_stats luasandboxutil)
add_test(NAME test_running_stats COMMAND test_running_stats)

add_executable(test_util test_util.c)
target_link_libraries(test_util luasandboxutil)
add_test(NAME test_util COMMAND test_util)

add_executable(test_heka_message test_heka_message.c)
target_link_libraries(test_heka_message luasandboxutil)
add_test(NAME test_heka_message COMMAND test_heka_message)

add_executable(test_heka_message_matcher test_heka_message_matcher.c)
target_link_libraries(test_heka_message_matcher luasandboxutil)
add_test(NAME test_heka_message_matcher COMMAND test_heka_message_matcher)

add_executable(test_string test_string.c)
target_link_libraries(test_string luasandboxutil)
add_test(NAME test_string COMMAND test_string)

if(WIN32)
   set(LIBRARY_PATHS "${CMAKE_BINARY_DIR}/src/util")
   set_tests_properties(test_input_buffer PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_output_buffer PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_protobuf PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_string_matcher PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_running_stats PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_util PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_heka_message PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_heka_message_matcher PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
   set_tests_properties(test_string PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
endif()
