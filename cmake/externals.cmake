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
set(SANDBOX_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${EP_BASE} -DEP_BASE=${EP_BASE} -DADDRESS_MODEL=${ADDRESS_MODEL} -DLUA_SANDBOX_INCLUDE=${CMAKE_SOURCE_DIR}/include --no-warn-unused-cli)
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
        GIT_REPOSITORY https://github.com/trink/lua.git
        GIT_TAG 63a4f245d214cdb61e7ae1ce2595e5c1ce3c7f51
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_DIR ${EP_BASE}
    )
endif()
include_directories(${LUA_INCLUDE_DIR})

externalproject_add(
    lua_lpeg
    GIT_REPOSITORY https://github.com/LuaDist/lpeg.git
    GIT_TAG baf0dc90b9278360be719dbfb8e56d34ce3c61bd
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lua_lpeg ${LUA_PROJECT})

externalproject_add(
    lua_cjson
    GIT_REPOSITORY https://github.com/trink/lua-cjson.git
    GIT_TAG af793dbfd83d9be69835faf8b88702ebb74547ed
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lua_cjson ${LUA_PROJECT})

externalproject_add(
    lua_struct
    GIT_REPOSITORY https://github.com/trink/struct.git
    GIT_TAG 5cf31819bee0d829d058cb5219e95ef0b1dd43a8
    UPDATE_COMMAND cmake -E copy ${CMAKE_CURRENT_LIST_DIR}/FindLua.cmake <SOURCE_DIR>/cmake
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lua_struct ${LUA_PROJECT})

externalproject_add(
    lua_bloom_filter
    GIT_REPOSITORY https://github.com/mozilla-services/lua_bloom_filter.git
    GIT_TAG ba756520a51592612c9709ebdfb99392f97eb51f
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lua_bloom_filter ${LUA_PROJECT})

externalproject_add(
    lua_circular_buffer
    GIT_REPOSITORY https://github.com/mozilla-services/lua_circular_buffer.git
    GIT_TAG ca4b0271bd741b857527f8315387142beac9cef9
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lua_circular_buffer ${LUA_PROJECT})

externalproject_add(
    lua_hyperloglog
    GIT_REPOSITORY https://github.com/mozilla-services/lua_hyperloglog.git
    GIT_TAG c9e3a7310b6e65a1ed54bc073ca6b836edb84cd5
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_DIR ${EP_BASE}
)
add_dependencies(lua_hyperloglog ${LUA_PROJECT})
