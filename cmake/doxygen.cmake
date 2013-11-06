# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYCONF_IN  ${CMAKE_SOURCE_DIR}/doxygen.in.conf)
    set(DOXYCONF_OUT ${CMAKE_BINARY_DIR}/doxygen.conf)
    if(EXISTS ${DOXYCONF_IN})
        configure_file(${DOXYCONF_IN} ${DOXYCONF_OUT})
    else()
        file(WRITE ${DOXYCONF_OUT} "
PROJECT_NAME        = \"${PROJECT_NAME}\"
PROJECT_BRIEF       = \"${CPACK_PACKAGE_DESCRIPTION_SUMMARY}\"
OUTPUT_DIRECTORY    = docs
GENERATE_LATEX      = NO
GENERATE_TODOLIST   = YES
FULL_PATH_NAMES     = YES
STRIP_FROM_PATH     = \"${CMAKE_SOURCE_DIR}\"
SOURCE_BROWSER      = YES
TAB_SIZE            = 4
EXTRACT_ALL         = YES
JAVADOC_AUTOBRIEF   = YES
RECURSIVE           = YES
INPUT               = \"${CMAKE_SOURCE_DIR}\"
EXCLUDE_PATTERNS    = \"${CMAKE_SOURCE_DIR}/.*\" \"${CMAKE_SOURCE_DIR}/debug*\" \"${CMAKE_SOURCE_DIR}/release*\"
EXAMPLE_PATH        = ${EXAMPLE_PATHS}
IMAGE_PATH          = ${IMAGE_PATHS}
BUILTIN_STL_SUPPORT = YES
STRIP_CODE_COMMENTS = NO
SHOW_DIRECTORIES    = YES
PROJECT_NUMBER      = ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
    endif()

    add_custom_target(docs ${DOXYGEN_EXECUTABLE} ${DOXYCONF_OUT})
else()
    message("Doxygen was not found, the documentation pages will not be generated")
endif()
