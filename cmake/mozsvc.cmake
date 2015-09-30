# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if(MSVC)
    # Predefined Macros: http://msdn.microsoft.com/en-us/library/b0084kay.aspx
    # Compiler options: http://msdn.microsoft.com/en-us/library/fwkeyyhe.aspx

    # set a high warning level and treat them as errors
    set(CMAKE_C_FLAGS           "/W3 /WX")

    # enable C++ exception handling
    set(CMAKE_CXX_FLAGS         "${CMAKE_C_FLAGS} /EHsc")

    # debug multi threaded dll runtime, complete debugging info, runtime error checking
    set(CMAKE_C_FLAGS_DEBUG     "/MDd /Zi /RTC1")
    set(CMAKE_CXX_FLAGS_DEBUG   ${CMAKE_C_FLAGS_DEBUG})

    # multi threaded dll runtime, optimize for speed, auto inlining
    set(CMAKE_C_FLAGS_RELEASE   "/MD /O2 /Ob2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

    set(CPACK_GENERATOR         "ZIP")
else()
    # Predefined Macros: clang|gcc -dM -E -x c /dev/null
    # Compiler options: http://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html#Invoking-GCC
    set(CMAKE_C_FLAGS   "-std=gnu99 -pedantic -Werror -Wall -Wextra")
    if (NOT WIN32)
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fPIC")
    else()
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -D__USE_MINGW_ANSI_STDIO")
    endif()
    set(CMAKE_CXX_FLAGS "-std=c++11 -pedantic -Werror -Wall -Wextra -fPIC -isystem /usr/local/include -isystem /opt/local/include")
    set(CMAKE_C_FLAGS_DEBUG     "-g")
    set(CMAKE_CXX_FLAGS_DEBUG   ${CMAKE_C_FLAGS_DEBUG})

    set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

    set(CMAKE_C_FLAGS_PROFILE   "${CMAKE_C_FLAGS_RELEASE} -g -pg")
    set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})

    set(CPACK_GENERATOR         "TGZ")

    set(CMAKE_SKIP_BUILD_RPATH              FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH      FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH   FALSE)
endif()

set(CPACK_PACKAGE_VENDOR        "Mozilla Services")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_STRIP_FILES TRUE)

include(CPack)
include(CTest)
include(doxygen)
