# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(ExternalProject)
string(REPLACE ")$" "|INSTALL_ARGS)$" _ep_keywords_ExternalProject_Add ${_ep_keywords_ExternalProject_Add})

get_filename_component(GIT_PATH ${GIT_EXECUTABLE} PATH)
find_program(PATCH_EXECUTABLE patch HINTS "${GIT_PATH}" "${GIT_PATH}/../bin")
if (NOT PATCH_EXECUTABLE)
   message(FATAL_ERROR "patch not found")
endif()

set(EP_BASE "${CMAKE_BINARY_DIR}/ep_base")
set_property(DIRECTORY PROPERTY EP_BASE ${EP_BASE})
set(SANDBOX_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${EP_BASE} -DEP_BASE=${EP_BASE} -DLUA_SANDBOX_INCLUDE=${CMAKE_SOURCE_DIR}/include -DUSE_RPATH=false --no-warn-unused-cli)
set(LUA_INCLUDE_DIR "${EP_BASE}/include/${PROJECT_NAME}")

set(INST_ARGS "install/strip")
if(MSVC)
    set(INST_ARGS "install")
endif()

if (LUA_JIT)
    message(FATAL_ERROR, "LuaJIT support has not been added back in yet, issue #66")
    set(LUA_PROJECT "luajit-2_0_2")
    if(MSVC)
        externalproject_add(
            ${LUA_PROJECT}
            BUILD_IN_SOURCE 1
            URL http://luajit.org/download/LuaJIT-2.0.2.tar.gz
            URL_MD5 112dfb82548b03377fbefbba2e0e3a5b
            PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/luajit-2_0_2.patch
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ${CMAKE_COMMAND} -E chdir src msvcbuild.bat
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy src/lua.dll ${EP_BASE}/lib/lua.dll
            COMMAND ${CMAKE_COMMAND} -E copy src/lua.lib ${EP_BASE}/lib/lua.lib
            COMMAND ${CMAKE_COMMAND} -E copy src/lauxlib.h "${LUA_INCLUDE_DIR}/lauxlib.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/luaconf.h "${LUA_INCLUDE_DIR}/luaconf.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/lua.h "${LUA_INCLUDE_DIR}/lua.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/luajit.h "${LUA_INCLUDE_DIR}/luajit.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/lualib.h "${LUA_INCLUDE_DIR}/lualib.h"
        )
    elseif(UNIX)
        externalproject_add(
            ${LUA_PROJECT}
            BUILD_IN_SOURCE 1
            URL http://luajit.org/download/LuaJIT-2.0.2.tar.gz
            URL_MD5 112dfb82548b03377fbefbba2e0e3a5b
            PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/luajit-2_0_2.patch
            CONFIGURE_COMMAND ""
            BUILD_COMMAND make
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy src/libluajit.a ${EP_BASE}/lib/liblua.a
            COMMAND ${CMAKE_COMMAND} -E copy src/lauxlib.h "${LUA_INCLUDE_DIR}/lauxlib.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/luaconf.h "${LUA_INCLUDE_DIR}/luaconf.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/lua.h "${LUA_INCLUDE_DIR}/lua.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/luajit.h "${LUA_INCLUDE_DIR}/luajit.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/lualib.h "${LUA_INCLUDE_DIR}/lualib.h"
        )
    else()
        message(FATAL_ERROR "Cannot use LuaJIT with ${CMAKE_GENERATOR}")
    endif()
else()
    set(LUA_PROJECT "lua-5_1_5")
    externalproject_add(
        ${LUA_PROJECT}
        GIT_REPOSITORY https://github.com/trink/lua.git
        GIT_TAG 57d3e82ef270b20dc976f3a6001439589c807793
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
endif()

externalproject_add(
    lua_lpeg
    GIT_REPOSITORY https://github.com/LuaDist/lpeg.git
    GIT_TAG baf0dc90b9278360be719dbfb8e56d34ce3c61bd
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/FindLua.cmake <SOURCE_DIR>/cmake
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_lpeg ${LUA_PROJECT})

externalproject_add(
    lua_cjson
    GIT_REPOSITORY https://github.com/trink/lua-cjson.git
    GIT_TAG b24f724a4982531e392a88679b3783fdc5361c5d
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_cjson ${LUA_PROJECT})

externalproject_add(
    lua_struct
    GIT_REPOSITORY https://github.com/trink/struct.git
    GIT_TAG b7e9b87d1ee36a5e22c6749be0959b45858beaad
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/FindLua.cmake <SOURCE_DIR>/cmake
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_struct ${LUA_PROJECT})

externalproject_add(
    lua_socket
    GIT_REPOSITORY https://github.com/LuaDist/luasocket.git
    GIT_TAG b97ed47e7a01e0d2523809efe11676333a85dcab
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/FindLua.cmake <SOURCE_DIR>/cmake
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX=${EP_BASE}/io
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_socket ${LUA_PROJECT})

# sandbox enhanced modules

externalproject_add(
    lua_bloom_filter
    GIT_REPOSITORY https://github.com/mozilla-services/lua_bloom_filter.git
    GIT_TAG 4151be752c3dd2f74d1c3487d8352ca54055eb81
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_bloom_filter luasandbox)

externalproject_add(
    lua_circular_buffer
    GIT_REPOSITORY https://github.com/mozilla-services/lua_circular_buffer.git
    GIT_TAG bb6dd9f88f148813315b5a660b7e2ba47f958b31
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_circular_buffer luasandbox)

externalproject_add(
    lua_hyperloglog
    GIT_REPOSITORY https://github.com/mozilla-services/lua_hyperloglog.git
    GIT_TAG 92f8a59e9e6f4ee381482f449b6cee78bb55460a
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_hyperloglog luasandbox)

externalproject_add(
    lua_cuckoo_filter
    GIT_REPOSITORY https://github.com/mozilla-services/lua_cuckoo_filter.git
    GIT_TAG 8343207aa7656d0324d23f79a5732fcb0bdb5917
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_cuckoo_filter luasandbox)

externalproject_add(
    lua_sax
    GIT_REPOSITORY https://github.com/trink/symtseries.git
    GIT_TAG e50724abf8b22198ab275b9d9ccabbef89250bc1
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_sax luasandbox)
