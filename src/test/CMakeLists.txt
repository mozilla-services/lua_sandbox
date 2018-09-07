# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(luasandboxtest SHARED sandbox.c)
set_target_properties(luasandboxtest PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} SOVERSION 0)
target_compile_definitions(luasandboxtest PRIVATE -Dluasandboxtest_EXPORTS)
target_link_libraries(luasandboxtest luasandbox)
install(TARGETS luasandboxtest DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_executable(test_generic_sandbox test_generic_sandbox.c)
target_link_libraries(test_generic_sandbox luasandboxtest)
set(LIBRARY_PATHS "${CMAKE_BINARY_DIR}/src;${CMAKE_BINARY_DIR}/src/util;${CMAKE_BINARY_DIR}/src/test")

add_test(NAME test_move_luasandbox_tests COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME test_generic_sandbox COMMAND test_generic_sandbox)

if(WIN32)
   STRING(REPLACE ";" "\\\\;" LIBRARY_PATHS "${LIBRARY_PATHS}")
   set_tests_properties(test_generic_sandbox PROPERTIES ENVIRONMENT PATH=${LIBRARY_PATHS})
endif()

