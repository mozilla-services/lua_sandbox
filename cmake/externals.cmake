# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(ExternalProject)

get_filename_component(GIT_PATH ${GIT_EXECUTABLE} PATH)
find_program(PATCH_EXECUTABLE patch HINTS "${GIT_PATH}" "${GIT_PATH}/../bin")
if (NOT PATCH_EXECUTABLE)
   message(FATAL_ERROR "patch not found")
endif()

set(EP_BASE "${CMAKE_BINARY_DIR}/ep_base")
set_property(DIRECTORY PROPERTY EP_BASE ${EP_BASE})
set(CJSON_PATCH_FILE "lua-cjson-2_1_0.patch")
set(SANDBOX_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${EP_BASE} -DEP_BASE=${EP_BASE} -DADDRESS_MODEL=${ADDRESS_MODEL} --no-warn-unused-cli)
set(LUA_INCLUDE_DIR "${EP_BASE}/include")

if (LUA_JIT)
    set(LUA_PROJECT "luajit-2_0_2")
    if(MSVC)
        externalproject_add(
            ${LUA_PROJECT}
            BUILD_IN_SOURCE 1
            URL http://luajit.org/download/LuaJIT-2.0.2.tar.gz
            URL_MD5 112dfb82548b03377fbefbba2e0e3a5b
            PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/luajit-2_0_2.patch
            CONFIGURE_COMMAND ""
            BUILD_COMMAND cmake -E chdir src msvcbuild.bat
            INSTALL_COMMAND cmake -E copy src/lua.dll ${EP_BASE}/lib/lua.dll
            COMMAND cmake -E copy src/lua.lib ${EP_BASE}/lib/lua.lib
            COMMAND cmake -E copy src/lauxlib.h "${LUA_INCLUDE_DIR}/lauxlib.h"
            COMMAND cmake -E copy src/luaconf.h "${LUA_INCLUDE_DIR}/luaconf.h"
            COMMAND cmake -E copy src/lua.h "${LUA_INCLUDE_DIR}/lua.h"
            COMMAND cmake -E copy src/luajit.h "${LUA_INCLUDE_DIR}/luajit.h"
            COMMAND cmake -E copy src/lualib.h "${LUA_INCLUDE_DIR}/lualib.h"
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
            INSTALL_COMMAND cmake -E copy src/libluajit.a ${EP_BASE}/lib/liblua.a
            COMMAND cmake -E copy src/lauxlib.h "${LUA_INCLUDE_DIR}/lauxlib.h"
            COMMAND cmake -E copy src/luaconf.h "${LUA_INCLUDE_DIR}/luaconf.h"
            COMMAND cmake -E copy src/lua.h "${LUA_INCLUDE_DIR}/lua.h"
            COMMAND cmake -E copy src/luajit.h "${LUA_INCLUDE_DIR}/luajit.h"
            COMMAND cmake -E copy src/lualib.h "${LUA_INCLUDE_DIR}/lualib.h"
        )
    else()
        message(FATAL_ERROR "Cannot use LuaJIT with ${CMAKE_GENERATOR}")
    endif()
else()
    set(LUA_PROJECT "lua-5_1_5")
    externalproject_add(
        ${LUA_PROJECT}
        URL http://www.lua.org/ftp/lua-5.1.5.tar.gz
        URL_MD5 2e115fe26e435e33b0d5c022e4490567
        PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/lua-5_1_5.patch
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_DIR ${EP_BASE}
    )
endif()
include_directories(${LUA_INCLUDE_DIR})

externalproject_add(
    lpeg-0_12
    URL http://www.inf.puc-rio.br/~roberto/lpeg/lpeg-0.12.tar.gz
    URL_MD5 4abb3c28cd8b6565c6a65e88f06c9162
    PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/lpeg-0_12.patch
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lpeg-0_12 ${LUA_PROJECT})

externalproject_add(
    lua-cjson-2_1_0
    GIT_REPOSITORY https://github.com/trink/lua-cjson.git
    GIT_TAG be85986dc481ad2a0d3abc06ac57e1d1241d9d4c
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lua-cjson-2_1_0 ${LUA_PROJECT})

