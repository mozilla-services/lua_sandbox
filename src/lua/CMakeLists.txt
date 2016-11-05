# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(luasandbox_bin lua)
target_link_libraries(luasandbox_bin luasandbox)
set_target_properties(luasandbox_bin PROPERTIES OUTPUT_NAME luasandbox)

install(TARGETS luasandbox_bin DESTINATION ${CMAKE_INSTALL_BINDIR})
