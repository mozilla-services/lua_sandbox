# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LUA_SANDBOX_SRC
lua_sandbox.c
lua_sandbox_private.c
lua_serialize.c
lua_serialize_json.c
lua_serialize_protobuf.c
lua_circular_buffer.c
)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/lua51.lib"
    "${EP_BASE}/lib/lpeg.lib"
    "${EP_BASE}/lib/cjson.lib"
    )
    add_library(luasandbox SHARED ${LUA_SANDBOX_SRC})
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.lib")
else()
    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/libluajit-5.1.a"
    "${EP_BASE}/lib/liblpeg.a"
    "${EP_BASE}/lib/libcjson.a"
    -ldl - lm
    )
    add_library(luasandbox STATIC ${LUA_SANDBOX_SRC})
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.a")
    install(DIRECTORY "${EP_BASE}/include/luajit-2.0/" DESTINATION include)
endif()

target_link_libraries(luasandbox ${LUA_SANDBOX_LIBS})

add_dependencies(luasandbox luajit-2_0_2 lpeg-0_12 lua-cjson-2_1_0)
install(TARGETS luasandbox DESTINATION lib)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION include)
install(DIRECTORY "${LUA_JIT_INCLUDE}/" DESTINATION include)

add_subdirectory(test)
