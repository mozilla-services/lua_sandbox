# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LUA_SANDBOX_SRC
lsb.c
lsb_output.c
lsb_serialize.c
lsb_serialize_protobuf.c
struct.c
)

add_library(luasandbox SHARED ${LUA_SANDBOX_SRC})
add_dependencies(luasandbox ${LUA_PROJECT} lua_lpeg lua_cjson)
add_dependencies(lua_bloom_filter luasandbox)
add_dependencies(lua_circular_buffer luasandbox)
add_dependencies(lua_hyperloglog luasandbox)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/lua.lib"
    "${EP_BASE}/lib/lpeg.lib"
    "${EP_BASE}/lib/cjson.lib"
    )
elseif(MINGW)
    add_definitions(-D_MINGW)
    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/liblua.dll"
    "${EP_BASE}/lib/liblpeg.dll"
    "${EP_BASE}/lib/libcjson.dll"
    )
    set_target_properties(luasandbox PROPERTIES LINK_FLAGS -s)
    if (ADDRESS_MODEL EQUAL 32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set_target_properties(luasandbox PROPERTIES LINK_FLAGS "-s -m32")
    endif()
else()
    if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(LINK_DL "-ldl")
    endif()

    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/liblua.a"
    "${EP_BASE}/lib/liblpeg.a"
    "${EP_BASE}/lib/libcjson.a"
	${LINK_DL} -lm
    )
endif()

target_link_libraries(luasandbox ${LUA_SANDBOX_LIBS})

install(TARGETS luasandbox DESTINATION lib)
install(DIRECTORY "${EP_BASE}/modules/" DESTINATION modules)
install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION include)
install(DIRECTORY "${LUA_INCLUDE_DIR}/" DESTINATION include)
install(DIRECTORY "${EP_BASE}/include/" DESTINATION include)

add_subdirectory(test)
