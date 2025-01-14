/*
 Copyright (C) 2015-2025 IoT.bzh Company

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

static const char *_get_(json_object *obj, const char *key, json_object *&item, bool mandatory, json_type type)
{
	const char *errtxt = nullptr;

	if (!json_object_object_get_ex(obj, key, &item)) {
		if (mandatory)
			errtxt = "set";
	}
	else if (type != json_type_null && type != json_object_get_type(item)) {
		if (type != json_type_double || json_type_int != json_object_get_type(item))
			errtxt = "of valid type";
	}
	return errtxt;
}

bool get(afb_api_t api, json_object *obj, const char *key, json_object *&item, bool mandatory, json_type type)
{
	const char *errtxt = _get_(obj, key, item, mandatory, type);
	if (errtxt != nullptr) {
		AFB_API_ERROR(api, "key '%s' is not %s in object %s",
			key, errtxt, json_object_to_json_string(obj));
		return false;
	}
	return true;
}

bool get(afb_req_t req, json_object *obj, const char *key, json_object *&item, bool mandatory, json_type type)
{
	const char *errtxt = _get_(obj, key, item, mandatory, type);
	if (errtxt != nullptr) {
		AFB_REQ_ERROR(req, "key '%s' is not %s in object %s",
			key, errtxt, json_object_to_json_string(obj));
		return false;
	}
	return true;
}

static bool _to_(const char *&item, json_object *&jso, bool status)
{
	if (status)
		item = json_object_get_string(jso);
	return status;
}

bool get(afb_api_t api, json_object *obj, const char *key, const char *&item, bool mandatory)
{
	json_object *jso;
	return _to_(item, jso, get(api, obj, key, jso, mandatory, json_type_string));
}

bool get(afb_req_t req, json_object *obj, const char *key, const char *&item, bool mandatory)
{
	json_object *jso;
	return _to_(item, jso, get(req, obj, key, jso, mandatory, json_type_string));
}

static bool _to_(int &item, json_object *&jso, bool status)
{
	if (status)
		item = json_object_get_int(jso);
	return status;
}

bool get(afb_api_t api, json_object *obj, const char *key, int &item, bool mandatory)
{
	json_object *jso;
	return _to_(item, jso, get(api, obj, key, jso, mandatory, json_type_int));
}

bool get(afb_req_t req, json_object *obj, const char *key, int &item, bool mandatory)
{
	json_object *jso;
	return _to_(item, jso, get(req, obj, key, jso, mandatory, json_type_int));
}

