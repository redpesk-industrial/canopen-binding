###########################################################################
#  Copyright (C) 2015-2025 IoT.bzh Company
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

cmake_minimum_required(VERSION 3.15)

# Project Info
# ------------------
project(canopen-binding
	VERSION 2.1.1
	DESCRIPTION "Provide a redpesk CANopen Binding"
	HOMEPAGE_URL "https://github.com/iotbzh/CANopen-service"
	LANGUAGES CXX C)

set(PROJECT_PRETTY_NAME "CANopen binding")
set(PROJECT_ICON "icon.png")
set(PROJECT_AUTHOR "Iot-Team")
set(PROJECT_AUTHOR_MAIL "johann.gautier@iot.bzh")
set(PROJECT_LICENSE "APL2.0")

include(FindPkgConfig)
include(GNUInstallDirs)

# Declare options
set(AFM_APP_DIR ${CMAKE_INSTALL_PREFIX}/redpesk CACHE PATH "Applications directory")
set(APP_DIR ${AFM_APP_DIR}/${PROJECT_NAME})

# PKG_CONFIG required packages
# -----------------------------

pkg_check_modules(deps REQUIRED
	json-c
	afb-binding
	librp-utils-static
	afb-helpers4-static>=10.0.5
	liblely-coapp>=2.3
)

add_compile_options(${deps_CFLAGS} -DAFB_BINDING_VERSION=4)
include_directories(${deps_INCLUDE_DIRS})
add_link_options(${deps_LDFLAGS})
link_libraries(${deps_LINK_LIBRARIES})

add_subdirectory(src)
add_subdirectory(src/plugins)
add_subdirectory(scripts)
