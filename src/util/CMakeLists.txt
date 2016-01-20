# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(UTIL_SRC
heka_message.c
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
target_link_libraries(luasandboxutil ${LIBM_LIBRARY})

install(TARGETS luasandboxutil DESTINATION lib COMPONENT core)
add_subdirectory(test)

