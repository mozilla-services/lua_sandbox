# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LUA_SANDBOX_SRC
luasandbox.c
lsb_output.c
lsb_serialize.c
lsb_serialize_protobuf.c
lsb_deserialize_protobuf.c
)

set ( CMAKE_SKIP_BUILD_RPATH FALSE CACHE STRING "" FORCE )
set ( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE STRING "" FORCE )

add_custom_target(copy_includes
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/include/luasandbox.h ${EP_BASE}/include
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/include/luasandbox_output.h ${EP_BASE}/include
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/include/luasandbox_serialize.h ${EP_BASE}/include)
add_library(luasandbox SHARED ${LUA_SANDBOX_SRC})
add_dependencies(luasandbox copy_includes)

add_dependencies(lua_bloom_filter luasandbox)
add_dependencies(lua_circular_buffer luasandbox)
add_dependencies(lua_hyperloglog luasandbox)
add_dependencies(lua_cuckoo_filter luasandbox)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(LUA_SANDBOX_LIBS "${EP_BASE}/lib/luasb.lib")
elseif(MINGW)
    add_definitions(-D_MINGW)
    set(LUA_SANDBOX_LIBS "${EP_BASE}/bin/libluasb.dll")
    set_target_properties(luasandbox PROPERTIES LINK_FLAGS -s)
    install(DIRECTORY "${EP_BASE}/bin/"  DESTINATION lib PATTERN "*.dll" COMPONENT core)
else()
    if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(LINK_DL "-ldl")
    endif()
    set(LUA_SANDBOX_LIBS "${EP_BASE}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}luasb${CMAKE_SHARED_LIBRARY_SUFFIX}" ${LINK_DL} -lm)
endif()

set_target_properties(luasandbox PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} SOVERSION 0)

target_link_libraries(luasandbox ${LUA_SANDBOX_LIBS})

install(TARGETS luasandbox DESTINATION lib COMPONENT core)
install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib COMPONENT core PATTERN "lua" EXCLUDE)
install(DIRECTORY "${EP_BASE}/lib/lua/" DESTINATION lib/${PROJECT_NAME}/modules COMPONENT core)
install(DIRECTORY "${EP_BASE}/include/" DESTINATION include COMPONENT core)

add_subdirectory(test)
