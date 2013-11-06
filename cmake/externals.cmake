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

externalproject_add(
    luajit-2_0_2
    BUILD_IN_SOURCE 1
    URL http://luajit.org/download/LuaJIT-2.0.2.tar.gz
    URL_MD5 112dfb82548b03377fbefbba2e0e3a5b
    PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/luajit-2_0_2.patch
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make
    INSTALL_COMMAND make install PREFIX="${EP_BASE}"
)
include_directories("${EP_BASE}/include/luajit-2.0")

externalproject_add(
    lpeg-0_12
    URL http://www.inf.puc-rio.br/~roberto/lpeg/lpeg-0.12.tar.gz
    URL_MD5 4abb3c28cd8b6565c6a65e88f06c9162
    PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/lpeg-0_12.patch
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_BASE} -DADDRESS_MODEL=${ADDRESS_MODEL} -DEP_BASE=${EP_BASE} --no-warn-unused-cli
    INSTALL_DIRECTORY ${EP_BASE}
)
add_dependencies(lpeg-0_12 luajit-2_0_2)

externalproject_add(
    lua-cjson-2_1_0
    URL http://www.kyne.com.au/~mark/software/download/lua-cjson-2.1.0.tar.gz
    URL_MD5 24f270663e9f6ca8ba2a02cef19f7963
    PATCH_COMMAND ${PATCH_EXECUTABLE} -p0 < ${CMAKE_CURRENT_LIST_DIR}/lua-cjson-2_1_0.patch
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_BASE} -DEP_BASE=${EP_BASE}
    INSTALL_DIRECTORY ${EP_BASE}
)
add_dependencies(lua-cjson-2_1_0 luajit-2_0_2)

