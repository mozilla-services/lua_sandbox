# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(ExternalProject)
string(REPLACE ")$" "|INSTALL_ARGS)$" _ep_keywords_ExternalProject_Add ${_ep_keywords_ExternalProject_Add})

get_filename_component(GIT_PATH ${GIT_EXECUTABLE} PATH)

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
else()
    set(LUA_PROJECT "lua-5_1_5")
    externalproject_add(
        ${LUA_PROJECT}
        GIT_REPOSITORY https://github.com/trink/lua.git
        GIT_TAG 308080049143009486221653a490c9e16f711f78
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
    add_library(luasb SHARED IMPORTED)
    set_target_properties(luasb PROPERTIES IMPORTED_LOCATION "${EP_BASE}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}luasb${CMAKE_SHARED_LIBRARY_SUFFIX}")
    #install(TARGETS luasb DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core OPTIONAL)
    #http://public.kitware.com/Bug/view.php?id=14311&nbn=6
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

if (OPENSSL_FOUND)
    if (APPLE)
        set(SANDBOX_CMAKE_ARGS ${SANDBOX_CMAKE_ARGS} -DCMAKE_PROJECT_luasec_INCLUDE=${CMAKE_SOURCE_DIR}/cmake/fixup_lua_sec.cmake)
    endif()
    externalproject_add(
        lua_sec
        GIT_REPOSITORY https://github.com/LuaDist/luasec.git
        GIT_TAG 329a8789f142bbbfef4fd7ed506285a25c3c0e0e
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/FindLua.cmake <SOURCE_DIR>/cmake
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX=${EP_BASE}/io
        INSTALL_ARGS ${INST_ARGS}
    )
    add_dependencies(lua_sec ${LUA_PROJECT})
endif()

# sandbox enhanced modules

externalproject_add(
    lua_bloom_filter
    GIT_REPOSITORY https://github.com/mozilla-services/lua_bloom_filter.git
    GIT_TAG 255af01bc127fccbd11fc5e57218a5fcc8659c69
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_bloom_filter luasandbox)

externalproject_add(
    lua_circular_buffer
    GIT_REPOSITORY https://github.com/mozilla-services/lua_circular_buffer.git
    GIT_TAG 85fc04e61e650ed65a7ba12ea02f8d307a5f1eab
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_circular_buffer luasandbox)

externalproject_add(
    lua_hyperloglog
    GIT_REPOSITORY https://github.com/mozilla-services/lua_hyperloglog.git
    GIT_TAG 792b11dd811d89d6f0ad973b04a742c02c4ef7b5
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_hyperloglog luasandbox)

externalproject_add(
    lua_cuckoo_filter
    GIT_REPOSITORY https://github.com/mozilla-services/lua_cuckoo_filter.git
    GIT_TAG bc7f406b3997fc9432bb5d1db96cad902bd3283a
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_cuckoo_filter luasandbox)

externalproject_add(
    lua_sax
    GIT_REPOSITORY https://github.com/trink/symtseries.git
    GIT_TAG bd2426c93d0d55977ca7b21354105463b0470a49
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_sax luasandbox)

externalproject_add(
    rapidjson
    GIT_REPOSITORY https://github.com/miloyip/rapidjson.git
    GIT_TAG ed7efe6289cfa1fafc354aaa8a29fcd31c1607fd
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "no configure"
    BUILD_COMMAND ${CMAKE_COMMAND} -E echo "no build"
    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "no install"
)
include_directories(${EP_BASE}/Source/rapidjson/include)
