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

##################################################
# Control Policy Config file
##################################################
PROJECT_TARGET_ADD(CANopen-config)

	file(GLOB CONF_FILES "*.json" "*.dcf")

	add_input_files("${CONF_FILES}")

	######################################################################
	# Genegating master dcf file if a yml file is availe for eatch config
	######################################################################

	foreach(CONF_FILE ${CONF_FILES})
		
		get_filename_component(CONF_DIR ${CONF_FILE} NAME_WLE)
		
		file(GLOB YML_CONF "${CONF_DIR}/*.yml")
		
		if(EXISTS ${YML_CONF})

			set(OUT_DIR "${PROJECT_PKG_BUILD_DIR}/etc/${CONF_DIR}")

			file(MAKE_DIRECTORY ${OUT_DIR})

			add_custom_command(OUTPUT "${OUT_DIR}/master.dcf"
				DEPENDS ${YML_CONF}
				COMMAND dcfgen -r -d ${OUT_DIR} ${YML_CONF}
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${CONF_DIR}"
			)

			add_custom_target(
				${CONF_DIR}_genereted_master
				DEPENDS "${OUT_DIR}/master.dcf"
				COMMENT "Build mastes.dcf based on yml shema : ${YML_CONF}"
			)

			add_dependencies(${TARGET_NAME} ${CONF_DIR}_genereted_master)
			
		endif()

	endforeach()

	SET_TARGET_PROPERTIES(
		${TARGET_NAME} PROPERTIES
		LABELS "BINDING-CONFIG"
		OUTPUT_NAME ${TARGET_NAME}
	)
