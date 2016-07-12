# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(UTIL_SRC
heka_message.c
heka_message_matcher.c
heka_message_matcher_parser.c
input_buffer.c
output_buffer.c
protobuf.c
running_stats.c
string.c
string_matcher.c
util.c
)

add_library(luasandboxutil SHARED ${UTIL_SRC})
set_target_properties(luasandboxutil PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} SOVERSION 0)
target_compile_definitions(luasandboxutil PRIVATE -Dluasandboxutil_EXPORTS)

if(LIBM_LIBRARY)
  target_link_libraries(luasandboxutil ${LIBM_LIBRARY})
endif()

install(TARGETS luasandboxutil DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_subdirectory(test)
