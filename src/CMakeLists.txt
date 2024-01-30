###########################################################################
#  Copyright (C) 2015-2020 IoT.bzh Company
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

add_library(CANopenEncoder SHARED
		CANopenEncoder.cpp
		CANopenGlue.cpp
	)
target_link_libraries(CANopenEncoder ${link_libraries})

INSTALL(TARGETS
		CANopenEncoder
		DESTINATION
		${APP_DIR}/lib
	)

#SET_TARGET_PROPERTIES(CANopenEncoder PROPERTIES
#	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libCANopenEncoder.so
#)

# Define project Targets
add_library(CANopen MODULE
		CANopenBinding.cpp
		CANopenMaster.cpp
		CANopenExec.cpp
		CANopenSensor.cpp
		CANopenSlaveDriver.cpp
		utils/utils.cpp
	)

INSTALL(TARGETS
		CANopen
		DESTINATION
		${APP_DIR}/lib
	)

# Binder exposes a unique public entry point
SET_TARGET_PROPERTIES(CANopen PROPERTIES
	PREFIX ""
	LINK_FLAGS "-Wl,--version-script=${vscript}"
	INSTALL_RPATH "$ORIGIN"
#	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/*${TARGET_NAME}.so
)
target_link_libraries(CANopen ${link_libraries} CANopenEncoder)

configure_file(manifest.yml.in manifest.yml @ONLY)
INSTALL(FILES
		${CMAKE_CURRENT_BINARY_DIR}/manifest.yml
		DESTINATION
		${APP_DIR}/.rpconfig
	)

# compile any C file starting with a lower case
file(GLOB CHEADERS *.hpp)
INSTALL(FILES ${CHEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CANopen)

configure_file(CANopen.pc.in CANopen.pc @ONLY)
INSTALL(FILES
		${CMAKE_CURRENT_BINARY_DIR}/CANopen.pc
		DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/pkgconfig
	)

