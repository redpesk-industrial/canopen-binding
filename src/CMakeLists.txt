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

# Add target to project dependency list
PROJECT_TARGET_ADD(CANopenEncoder)
	# Define project Targets
	add_library(${TARGET_NAME} SHARED
					CANopenEncoder.cpp
					CANopenGlue.cpp
					)

	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		LABELS "LIBRARY"
		IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib${TARGET_NAME}.so
	)

# Add target to project dependency list
PROJECT_TARGET_ADD(CANopen)

	# compile any C file starting with a lower case
	file(GLOB CHEADERS *.hpp utils/*.hpp)

	# Define project Targets
	add_library(${TARGET_NAME} MODULE
					CANopenMaster.cpp
					CANopenBinding.cpp
					CANopenSensor.cpp
					CANopenSlaveDriver.cpp
					utils/utils.c
					)

	# Binder exposes a unique public entry point
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX "afb-"
		LABELS "BINDING"
		LINK_FLAGS ${BINDINGS_LINK_FLAG}
		INSTALL_RPATH "$ORIGIN"
		IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/*${TARGET_NAME}.so
		OUTPUT_NAME ${TARGET_NAME}
	)

	target_link_libraries(${TARGET_NAME} ${link_libraries} CANopenEncoder)

INSTALL(FILES ${CHEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

include(GNUInstallDirs)
configure_file(../${TARGET_NAME}.pc.in ${TARGET_NAME}.pc @ONLY)

INSTALL(FILES
		${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.pc
		DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/pkgconfig
	)

add_subdirectory("plugins")
