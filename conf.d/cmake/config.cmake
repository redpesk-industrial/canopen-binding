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

# Project Info
# ------------------
set(PROJECT_PRETTY_NAME "CANopen binding")
set(PROJECT_ICON "icon.png")
set(PROJECT_AUTHOR "Iot-Team")
set(PROJECT_AUTHOR_MAIL "johann.gautier@iot.bzh")
set(PROJECT_LICENSE "APL2.0")
set(PROJECT_URL ${PROJECT_HOMEPAGE_URL})
set(API_NAME "CANopen")

# Where are stored default templates files from submodule or subtree app-templates in your project tree
# relative to the root project directory
set(PROJECT_CMAKE_CONF_DIR "conf.d")

# PKG_CONFIG required packages
# -----------------------------
set (PKG_REQUIRED_LIST
	json-c
	afb-binding
	afb-libcontroller
	afb-libhelpers
	liblely-coapp
)


add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)

add_compile_options(-DAFB_BINDING_VERSION=3)

set(MANIFEST_YAML_TEMPLATE ${CMAKE_SOURCE_DIR}/conf.d/wgt/manifest.yml.in CACHE PATH "Path to manifest file template")

set(WIDGET_TYPE application/vnd.RP.service)
set(WIDGET_ENTRY_POINT lib/afb-CANopen.so)

include(CMakeAfbTemplates)
