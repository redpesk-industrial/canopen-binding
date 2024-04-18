/*
 Copyright (C) 2015-2024 IoT.bzh Company

 $RP_BEGIN_LICENSE$
 Commercial License Usage
  Licensees holding valid commercial IoT.bzh licenses may use this file in
  accordance with the commercial license agreement provided with the
  Software or, alternatively, in accordance with the terms contained in
  a written agreement between you and The IoT.bzh Company. For licensing terms
  and conditions see https://www.iot.bzh/terms-conditions. For further
  information use the contact form at https://www.iot.bzh/contact.

 GNU General Public License Usage
  Alternatively, this file may be used under the terms of the GNU General
  Public license version 3. This license is as published by the Free Software
  Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
  of this file. Please review the following information to ensure the GNU
  General Public License requirements will be met
  https://www.gnu.org/licenses/gpl-3.0.html.
 $RP_END_LICENSE$
*/

#include "jsonc.hpp"

bool get(afb_api_t api, json_object *obj, const char *key, json_object *&item, json_type type, bool mandatory)
{
	const char *errtxt = NULL;

	if (!json_object_object_get_ex(obj, key, &item)) {
		if (mandatory)
			errtxt = "set";
	}
	else if (type != json_type_null && type != json_object_get_type(item)) {
		if (type != json_type_double || json_type_int != json_object_get_type(item))
			errtxt = "of valid type";
	}
	if (errtxt) {
		AFB_API_ERROR(api, "key '%s' is not %s in configuration object %s",
			key, errtxt, json_object_to_json_string(obj));
		return false;
	}
	return true;
}

