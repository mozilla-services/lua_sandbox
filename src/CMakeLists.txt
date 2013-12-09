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

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/lua.lib"
    "${EP_BASE}/lib/lpeg.lib"
    "${EP_BASE}/lib/cjson.lib"
    )
    add_library(luasandbox SHARED ${LUA_SANDBOX_SRC})
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.lib")
elseif(MINGW)
    add_definitions(-D_MINGW)
    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/liblua.dll"
    "${EP_BASE}/lib/liblpeg.dll"
    "${EP_BASE}/lib/libcjson.dll"
    )
    add_library(luasandbox SHARED ${LUA_SANDBOX_SRC})
    set_target_properties(luasandbox PROPERTIES LINK_FLAGS -s)
    if (ADDRESS_MODEL EQUAL 32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set_target_properties(luasandbox PROPERTIES LINK_FLAGS "-s -m32")
    endif()
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.dll")
else()
	if (LUA_JIT)
		if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
			set(LINK_DL "-ldl")
		endif()
	endif()

    set(LUA_SANDBOX_LIBS
    "${EP_BASE}/lib/liblua.a"
    "${EP_BASE}/lib/liblpeg.a"
    "${EP_BASE}/lib/libcjson.a"
    ${LINK_DL} -lm
    )
    add_library(luasandbox STATIC ${LUA_SANDBOX_SRC})
    install(DIRECTORY "${EP_BASE}/lib/"  DESTINATION lib FILES_MATCHING PATTERN "*.a")
    install(DIRECTORY "${EP_BASE}/include/" DESTINATION include)
endif()

target_link_libraries(luasandbox ${LUA_SANDBOX_LIBS})

add_dependencies(luasandbox ${LUA_PROJECT} lpeg-0_12 lua-cjson-2_1_0)
install(TARGETS luasandbox DESTINATION lib)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION include)
install(DIRECTORY "${LUA_INCLUDE_DIR}/" DESTINATION include)

add_subdirectory(test)
