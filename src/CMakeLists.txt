# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LUA_SANDBOX_SRC
luasandbox.c
luasandbox_output.c
luasandbox_serialize.c
)

set ( CMAKE_SKIP_BUILD_RPATH FALSE CACHE STRING "" FORCE )
set ( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE STRING "" FORCE )

add_library(luasandbox SHARED ${LUA_SANDBOX_SRC})

add_dependencies(lua_bloom_filter luasandbox)
add_dependencies(lua_circular_buffer luasandbox)
add_dependencies(lua_hyperloglog luasandbox)
add_dependencies(lua_cuckoo_filter luasandbox)

if(WIN32)
  set(LIB_DIR bin)
else()
  set(LIB_DIR lib)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(LUA_SANDBOX_LIBS "${EP_BASE}/lib/luasb.lib")
    install(DIRECTORY "${EP_BASE}/${LIB_DIR}/"  DESTINATION ${LIB_DIR} COMPONENT core PATTERN "*.dll")
elseif(MINGW)
    add_definitions(-D_MINGW)
    set(LUA_SANDBOX_LIBS "${EP_BASE}/bin/libluasb.dll")
    set_target_properties(luasandbox PROPERTIES LINK_FLAGS -s)
    install(DIRECTORY "${EP_BASE}/${LIB_DIR}/"  DESTINATION ${LIB_DIR} COMPONENT core PATTERN "*.dll")
else()
    if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(LINK_DL "-ldl")
    endif()
    set(LUA_SANDBOX_LIBS "${EP_BASE}/${LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}luasb${CMAKE_SHARED_LIBRARY_SUFFIX}" ${LINK_DL} ${LIBM_LIBRARY})
endif()

set_target_properties(luasandbox PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} SOVERSION 0)

target_link_libraries(luasandbox luasandboxutil ${LUA_SANDBOX_LIBS})

install(TARGETS luasandbox DESTINATION ${LIB_DIR} COMPONENT core)
install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib COMPONENT core PATTERN "lua" EXCLUDE)
install(DIRECTORY "${EP_BASE}/lib/lua/" DESTINATION lib/${PROJECT_NAME}/modules COMPONENT core)
install(DIRECTORY "${EP_BASE}/io/lib/lua/" DESTINATION lib/${PROJECT_NAME}/io_modules COMPONENT core)
install(DIRECTORY "${EP_BASE}/include/" DESTINATION include COMPONENT core)

add_subdirectory(util)
add_subdirectory(heka)
add_subdirectory(test)
