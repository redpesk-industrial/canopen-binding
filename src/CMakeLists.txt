###########################################################################
#  Copyright (C) 2015-2024 IoT.bzh Company
#
#  Author: Johann Gautier <johann.gautier@iot.bzh>
#
#  $RP_BEGIN_LICENSE$
#  Commercial License Usage
#   Licensees holding valid commercial IoT.bzh licenses may use this file in
#   accordance with the commercial license agreement provided with the
#   Software or, alternatively, in accordance with the terms contained in
#   a written agreement between you and The IoT.bzh Company. For licensing terms
#   and conditions see https://www.iot.bzh/terms-conditions. For further
#   information use the contact form at https://www.iot.bzh/contact.
#
#  GNU General Public License Usage
#   Alternatively, this file may be used under the terms of the GNU General
#   Public license version 3. This license is as published by the Free Software
#   Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
#   of this file. Please review the following information to ensure the GNU
#   General Public License requirements will be met
#   https://www.gnu.org/licenses/gpl-3.0.html.
#  $RP_END_LICENSE$
###########################################################################

add_subdirectory(xchg)

add_library(CANopenEncoder SHARED CANopenEncoder.cpp CANopenGlue.cpp)
target_link_libraries(CANopenEncoder ${link_libraries})
install(TARGETS CANopenEncoder LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES CANopenEncoder.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CANopen)

# Define project Targets
add_library(CANopen MODULE
	CANopenBinding.cpp
	CANopenChannel.cpp
	CANopenExec.cpp
	CANopenMaster.cpp
	CANopenMasterSet.cpp
	CANopenSensor.cpp
	CANopenSlaveDriver.cpp
	utils/utils.cpp
	utils/jsonc.cpp
)

install(TARGETS
	CANopen
	DESTINATION
	${APP_DIR}/lib
)

# Binder exposes a unique public entry point
set_target_properties(CANopen PROPERTIES PREFIX "")
target_link_libraries(CANopen ${link_libraries} CANopenEncoder CANopenXchg)

pkg_get_variable(vscript QUIET afb-binding version_script)
if(vscript_FOUND)
	set_target_properties(CANopen PROPERTIES LINK_FLAGS "-Wl,--version-script=${vscript}")
endif()

configure_file(manifest.yml.in manifest.yml @ONLY)
install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/manifest.yml
		DESTINATION
		${APP_DIR}/.rpconfig
	)

# compile any C file starting with a lower case
file(GLOB CHEADERS *.hpp)
install(FILES ${CHEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CANopen)

configure_file(CANopen.pc.in CANopen.pc @ONLY)
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/CANopen.pc
	DESTINATION
	${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
