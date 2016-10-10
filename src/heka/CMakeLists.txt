# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(HEKA_SRC
message.c
read_message_zc.c
sandbox.c
stream_reader.c
)

add_library(luasandboxheka SHARED ${HEKA_SRC})
set_target_properties(luasandboxheka PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} SOVERSION 0)
target_compile_definitions(luasandboxheka PRIVATE -Dluasandboxheka_EXPORTS)
target_link_libraries(luasandboxheka luasandbox)
if(WIN32)
  target_link_libraries(luasandboxheka ws2_32)
endif()

if(LIBM_LIBRARY)
  target_link_libraries(luasandboxheka ${LIBM_LIBRARY})
endif()

install(TARGETS luasandboxheka DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_subdirectory(test)
