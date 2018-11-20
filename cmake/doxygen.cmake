# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Doxygen QUIET)
find_program(LUA_EXE lua QUIET)
find_program(PANDOC_EXE pandoc QUIET)
find_program(gitbook gitbook QUIET)
if(DOXYGEN_FOUND AND LUA_EXE AND PANDOC_EXE)
    set(DOXYCONF_IN  ${CMAKE_SOURCE_DIR}/doxygen.in.conf)
    set(DOXYCONF_OUT ${CMAKE_BINARY_DIR}/doxygen.conf)
    if(EXISTS ${DOXYCONF_IN})
        configure_file(${DOXYCONF_IN} ${DOXYCONF_OUT})
    else()
        file(WRITE ${DOXYCONF_OUT} "
PROJECT_NAME            = \"${PROJECT_NAME}\"
PROJECT_BRIEF           = \"${CPACK_PACKAGE_DESCRIPTION_SUMMARY}\"
OUTPUT_DIRECTORY        = \"${CMAKE_SOURCE_DIR}/gh-pages\"
HTML_OUTPUT             = doxygen
GENERATE_LATEX          = NO
GENERATE_TODOLIST       = YES
FULL_PATH_NAMES         = YES
STRIP_FROM_PATH         = \"${CMAKE_SOURCE_DIR}\"
SOURCE_BROWSER          = YES
TAB_SIZE                = 4
EXTRACT_ALL             = YES
JAVADOC_AUTOBRIEF       = YES
RECURSIVE               = YES
INPUT                   = \"${CMAKE_SOURCE_DIR}/include\" \"${CMAKE_SOURCE_DIR}/README.md\"
USE_MDFILE_AS_MAINPAGE  = \"${CMAKE_SOURCE_DIR}/README.md\"
EXAMPLE_PATH            = ${EXAMPLE_PATHS}
IMAGE_PATH              = ${IMAGE_PATHS}
BUILTIN_STL_SUPPORT     = YES
STRIP_CODE_COMMENTS     = NO
SHOW_DIRECTORIES        = YES
PROJECT_NUMBER          = ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
    endif()

    add_custom_target(docs ${DOXYGEN_EXECUTABLE} ${DOXYCONF_OUT}
    COMMAND lua gen_gh_pages.lua "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
    "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
else()
    message("The optional documentation tools were not found; the doc target has not been created")
endif()
