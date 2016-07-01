# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(ExternalProject)
string(REPLACE ")$" "|INSTALL_ARGS)$" _ep_keywords_ExternalProject_Add ${_ep_keywords_ExternalProject_Add})
set(EP_BASE ${CMAKE_BINARY_DIR}/ep_base)

get_filename_component(GIT_PATH ${GIT_EXECUTABLE} PATH)

set(LUASANDBOX_LUASB_LIBRARY ${EP_BASE}/inst/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}luasb${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LUASANDBOX_UTIL_LIBRARY ${EP_BASE}/../src/util/${CMAKE_SHARED_LIBRARY_PREFIX}luasandboxutil${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LUASANDBOX_LIBRARY ${EP_BASE}/../src/${CMAKE_SHARED_LIBRARY_PREFIX}luasandbox${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LUASANDBOX_TEST_LIBRARY ${EP_BASE}/../src/test/${CMAKE_SHARED_LIBRARY_PREFIX}luasandboxtest${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LUASANDBOX_HEKA_LIBRARY ${EP_BASE}/../src/heka/${CMAKE_SHARED_LIBRARY_PREFIX}luasandboxheka${CMAKE_SHARED_LIBRARY_SUFFIX})

set_property(DIRECTORY PROPERTY EP_BASE ${EP_BASE})
set(SANDBOX_CMAKE_ARGS
-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
-DCMAKE_INSTALL_PREFIX=${EP_BASE}/inst
-DLUASANDBOX_INCLUDE_DIR=${EP_BASE}/inst/include
-DLUASANDBOX_LUASB_LIBRARY=${LUASANDBOX_LUASB_LIBRARY}
-DLUASANDBOX_UTIL_LIBRARY=${LUASANDBOX_UTIL_LIBRARY}
-DLUASANDBOX_LIBRARY=${LUASANDBOX_LIBRARY}
-DLUASANDBOX_TEST_LIBRARY=${LUASANDBOX_TEST_LIBRARY}
-DLUASANDBOX_HEKA_LIBRARY=${LUASANDBOX_HEKA_LIBRARY}
-DUSE_RPATH=false # todo remove after the LUA_SOCKET build is updated to https://github.com/diegonehab/luasocket
--no-warn-unused-cli)

set(INST_ARGS "install/strip")
if(MSVC)
    set(INST_ARGS "install")
endif()

if(LUA_JIT)
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
    set_target_properties(luasb PROPERTIES IMPORTED_LOCATION ${LUASANDBOX_LUASB_LIBRARY})
    #install(TARGETS luasb DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core OPTIONAL)
    #http://public.kitware.com/Bug/view.php?id=14311&nbn=6
endif()

externalproject_add(
    lua_lpeg
    URL http://www.inf.puc-rio.br/~roberto/lpeg/lpeg-1.0.0.tar.gz
    URL_MD5 0aec64ccd13996202ad0c099e2877ece
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.lpeg <SOURCE_DIR>/CMakeLists.txt
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_lpeg luasandboxheka)

## sandbox enhanced modules

externalproject_add(
    lua_bloom_filter
    GIT_REPOSITORY https://github.com/mozilla-services/lua_bloom_filter.git
    GIT_TAG dbfd90e313be86fe1864fe0e18f6e9d5cb0e6f16
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
    TEST_AFTER_INSTALL 1
)
add_dependencies(lua_bloom_filter luasandboxheka)

externalproject_add(
    lua_circular_buffer
    GIT_REPOSITORY https://github.com/mozilla-services/lua_circular_buffer.git
    GIT_TAG f86e296bb0fafbc8529d74336cc17ff18d8a9e75
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
    TEST_AFTER_INSTALL 1
)
add_dependencies(lua_circular_buffer luasandboxheka)

externalproject_add(
    lua_hyperloglog
    GIT_REPOSITORY https://github.com/mozilla-services/lua_hyperloglog.git
    GIT_TAG eb1e27c00c206ceffaab6787b4f54c59d4451a5c
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
    TEST_AFTER_INSTALL 1
)
add_dependencies(lua_hyperloglog luasandboxheka)

externalproject_add(
    lua_cuckoo_filter
    GIT_REPOSITORY https://github.com/mozilla-services/lua_cuckoo_filter.git
    GIT_TAG a5c031fe70008d0f3d347371a7d7ae68acaba575
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
    TEST_AFTER_INSTALL 1
)
add_dependencies(lua_cuckoo_filter luasandboxheka)

externalproject_add(
    lua_sax
    GIT_REPOSITORY https://github.com/trink/symtseries.git
    GIT_TAG bd2426c93d0d55977ca7b21354105463b0470a49
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.sax <SOURCE_DIR>/CMakeLists.txt
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_sax luasandboxheka)

externalproject_add(
    lua_cjson
    GIT_REPOSITORY https://github.com/trink/lua-cjson.git
    GIT_TAG b24f724a4982531e392a88679b3783fdc5361c5d
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.cjson <SOURCE_DIR>/CMakeLists.txt
    CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
    INSTALL_ARGS ${INST_ARGS}
)
add_dependencies(lua_cjson luasandboxheka)

set(OPTIONAL_MODULES LUA_RJSON LUA_STRUCT LUA_KAFKA LUA_SOCKET LUA_SEC LUA_OPENSSL LUA_POSTGRES LUA_GEOIP LUA_SNAPPY)
foreach(module_name IN LISTS OPTIONAL_MODULES)
    if(ALL_OPTIONAL_MODULES)
        option(${module_name} "Include the ${module_name} module" on)
    else()
        option(${module_name} "Include the ${module_name} module" off)
    endif()
endforeach()

# Optionally Built modules

if(LUA_RJSON)
    externalproject_add(
        lua_rjson
        GIT_REPOSITORY https://github.com/mozilla-services/lua_rjson.git
        GIT_TAG 664351d4b19011375cef41365000201b2728be20
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
        TEST_AFTER_INSTALL 1
    )
    add_dependencies(lua_rjson luasandboxheka)
endif()

if(LUA_KAFKA)
    externalproject_add(
        lua_kafka
        GIT_REPOSITORY https://github.com/mozilla-services/lua_kafka.git
        GIT_TAG 27b63f1e3f56ff641661e983e70e79062511d091
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
        # TEST_AFTER_INSTALL 1 # requires a running kafka installation
    )
    add_dependencies(lua_kafka luasandboxheka)
endif()

if(LUA_STRUCT)
    externalproject_add(
        lua_struct
        URL http://www.inf.puc-rio.br/~roberto/struct/struct-0.2.tar.gz
        URL_MD5 99384bf1f54457ec9f796ad0b539d19c
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.struct <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
    add_dependencies(lua_struct luasandboxheka)
endif()

if(LUA_SOCKET) # todo move off the LuaDist version to https://github.com/diegonehab/luasocket
    externalproject_add(
        lua_socket
        GIT_REPOSITORY https://github.com/LuaDist/luasocket.git
        GIT_TAG b97ed47e7a01e0d2523809efe11676333a85dcab
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/FindLua.cmake <SOURCE_DIR>/cmake
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX=${EP_BASE}/lua_socket
        INSTALL_ARGS ${INST_ARGS}
    )
    externalproject_add_step(lua_socket copy_install
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${EP_BASE}/lua_socket/lib/lua ${EP_BASE}/inst/lib/luasandbox/io_modules
    DEPENDEES install)
    add_dependencies(lua_socket luasandboxheka)
endif()

if(LUA_SEC)
    externalproject_add(
        lua_sec
        GIT_REPOSITORY https://github.com/brunoos/luasec.git
        GIT_TAG 20443861ebc3f6498ee7d9c70fbdaa059bec15e1
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.lua_sec <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
    add_dependencies(lua_sec luasandboxheka)
endif()

if(LUA_OPENSSL)
    externalproject_add(
        lua_openssl
        GIT_REPOSITORY https://github.com/zhaozg/lua-openssl.git
        GIT_TAG 2e8930c5e13b52705ba9c390110e84b1f4748ba9
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.lua_openssl <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
    add_dependencies(lua_openssl luasandboxheka)
endif()

if(LUA_POSTGRES)
    externalproject_add(
        lua_postgres
        GIT_REPOSITORY https://github.com/LuaDist/luasql-postgresql.git
        GIT_TAG 29a3aa1964aeac93323ec5d1446ac7d32ec700df
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.lua_postgres <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
    add_dependencies(lua_postgres luasandboxheka)
endif()

if(LUA_GEOIP)
    externalproject_add(
        lua_geoip
        GIT_REPOSITORY https://github.com/agladysh/lua-geoip.git
        GIT_TAG a07d261d8a2c7ff854fe6cd72cb8c2e16ec638ff
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.lua_geoip <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
    add_dependencies(lua_geoip luasandboxheka)
endif()

if(LUA_SNAPPY)
    externalproject_add(
        lua_snappy
        GIT_REPOSITORY https://github.com/forhappy/lua-snappy.git
        GIT_TAG 6b4f3f6736857d5c72aa6ed0ec566af6e222278d
        UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.lua_snappy <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${SANDBOX_CMAKE_ARGS}
        INSTALL_ARGS ${INST_ARGS}
    )
    add_dependencies(lua_snappy luasandboxheka)
endif()
