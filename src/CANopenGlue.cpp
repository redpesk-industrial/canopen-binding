/*
 Copyright (C) 2015-2024 IoT.bzh Company

 Author: Johann Gautier <johann.gautier@iot.bzh>

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

#include "CANopenGlue.hpp"

#include <algorithm>


typedef enum
{
	DATA_TYPE_NULL,
	DATA_TYPE_DECIMAL,
	DATA_TYPE_STR_DECIMAL,
	DATA_TYPE_HEX,
	DATA_TYPE_STRING
} data_type;

data_type get_data_type(json_object *dataJ)
{
	const char *txt;
	bool hexa;

	switch(json_object_get_type(dataJ))
	{
	case json_type_string:
		txt = json_object_get_string(dataJ);
		if (txt[0] < '0' || txt[0] > '9')
			return DATA_TYPE_STRING;

		hexa = txt[0] == '0' && ((txt[1] | '\040') == 'x');
		txt += hexa;
		while(char c = *++txt) {
			if (c < '0'
			 || (c > '9' && (!hexa || (c | '\040') < 'a' || (c | '\040') > 'f')))
				return DATA_TYPE_STRING;
		}
		return hexa ? DATA_TYPE_HEX : DATA_TYPE_STR_DECIMAL;

	case json_type_boolean:
	case json_type_int:
	case json_type_double:
		return DATA_TYPE_DECIMAL;

	default:
		return DATA_TYPE_NULL;
	}
}

int32_t get_data_int32(json_object *dataJ)
{
	if (json_object_get_type(dataJ) == json_type_string)
	{
		long cvt;
		char *end;
		const char *txt = json_object_get_string(dataJ);
		if (txt[0] == '0' && (txt[1] & '\337') == 'X')
			cvt = strtol(&txt[2], &end, 16);
		else
			cvt = strtol(txt, &end, 10);
		if (!*end)
			return (int32_t)cvt;
	}
	else if (json_object_get_type(dataJ) == json_type_int)
		return json_object_get_int(dataJ);

	throw std::runtime_error("data " + (std::string)json_object_to_json_string(dataJ) + " not handled by get_data_int32");
	return 0;
}

int64_t get_data_int64(json_object *dataJ)
{
	errno = 0;
	int64_t result = json_object_get_int64(dataJ);
	if (!errno)
		return result;

	if (json_object_get_type(dataJ) == json_type_string)
	{
		const char *txt = json_object_get_string(dataJ);
		if (*txt++ == '0' && (*txt++ | '\040') == 'x')
		{
			char *end;
			long long cvt = strtoll(txt, &end, 16);
			if (!*end)
				return (int64_t)cvt;
		}
	}
	throw std::runtime_error("data " + (std::string)json_object_to_json_string(dataJ) + " not handeled by get_data_int64");
	return 0;
}