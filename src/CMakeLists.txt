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

set(LUA_SANDBOX_STATIC_LIBS
"${EP_BASE}/lib/libluajit-5.1.a"
"${EP_BASE}/lib/liblpeg.a"
"${EP_BASE}/lib/libcjson.a"
)

add_library(luasandbox STATIC ${LUA_SANDBOX_SRC})
target_link_libraries(luasandbox ${LUA_SANDBOX_STATIC_LIBS} -lm -ldl)

add_dependencies(luasandbox luajit-2_0_2 lpeg-0_12 lua-cjson-2_1_0)
install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.a")
install(DIRECTORY "${EP_BASE}/include/luajit-2.0/" DESTINATION include)
install(TARGETS luasandbox DESTINATION lib)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION include)

add_subdirectory(test)
