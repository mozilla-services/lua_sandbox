# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if(MSVC)
	add_definitions(
	-DLUA_BUILD_AS_DLL
	-D_CRT_SECURE_NO_WARNINGS
	)
else()
	add_definitions(
	-DLUA_USE_POSIX 
	-DLUA_USE_DLOPEN 
	-DLUA_USE_STRTODHEX
	-DLUA_USE_LONGLONG 
	-DLUA_USE_GMTIME_R
	)
endif()

set(LUA_SRC
lua/lapi.c
lua/lauxlib.c
lua/lbaselib.c
lua/lcode.c
lua/ldblib.c
lua/ldebug.c
lua/ldo.c
lua/ldump.c
lua/lfunc.c 
lua/lgc.c
lua/linit.c
lua/liolib.c
lua/llex.c
lua/lmathlib.c
lua/lmem.c
lua/loadlib.c 
lua/lobject.c
lua/lopcodes.c
lua/loslib.c
lua/lparser.c
lua/lstate.c
lua/lstring.c
lua/lstrlib.c
lua/ltable.c
lua/ltablib.c
lua/ltm.c
lua/lundump.c
lua/lvm.c
lua/lzio.c
)

set(LUA_SANDBOX_SRC
luasandbox.c
luasandbox_output.c
luasandbox_serialize.c
)

add_library(luasandbox SHARED ${LUA_SANDBOX_SRC} ${LUA_SRC})
set_target_properties(luasandbox PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0)
target_link_libraries(luasandbox luasandboxutil ${CMAKE_DL_LIBS})
if(LIBM_LIBRARY)
  target_link_libraries(luasandbox ${LIBM_LIBRARY})
endif()
install(TARGETS luasandbox DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(util)
add_subdirectory(heka)
if(NOT WIN32) # todo need to add getopt support for Windows
  add_subdirectory(cli)
endif()
add_subdirectory(test)
