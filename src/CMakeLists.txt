# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LUA_SANDBOX_SRC
luasandbox.c
luasandbox_modules.c
luasandbox_output.c
luasandbox_serialize.c
)

set ( CMAKE_SKIP_BUILD_RPATH FALSE CACHE STRING "" FORCE )
set ( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE STRING "" FORCE )

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core PATTERN "*.dll")
endif()

add_library(luasandbox SHARED ${LUA_SANDBOX_SRC})
add_dependencies(luasandbox ${LUA_PROJECT})

set_target_properties(luasandbox PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} SOVERSION 0)
target_link_libraries(luasandbox luasandboxutil luasb ${CMAKE_DL_LIBS})
if(LIBM_LIBRARY)
  target_link_libraries(luasandbox ${LIBM_LIBRARY})
endif()

install(TARGETS luasandbox DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core)
install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core PATTERN "lua" EXCLUDE)
install(DIRECTORY "${EP_BASE}/lib/lua/" DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/modules COMPONENT core)
install(DIRECTORY "${EP_BASE}/io/lib/lua/" DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/io_modules COMPONENT core)
install(DIRECTORY "${EP_BASE}/include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT core)

add_subdirectory(util)
add_subdirectory(heka)
if(NOT WIN32) # todo need to add getopt support for Windows
  add_subdirectory(cli)
endif()
add_subdirectory(test)
