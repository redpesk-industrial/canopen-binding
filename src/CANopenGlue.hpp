/*
 Copyright (C) 2015-2020 IoT.bzh Company

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

#ifndef _CANOPENGLUE_HEADER_
#define _CANOPENGLUE_HEADER_

#include <iostream>
#include <wrap-json.h>

typedef enum
{
	DATA_TYPE_NULL,
	DATA_TYPE_DECIMAL,
	DATA_TYPE_STR_DECIMAL,
	DATA_TYPE_HEX,
	DATA_TYPE_STRING
} data_type;

data_type get_data_type(std::string data);
data_type get_data_type(json_object *dataJ);
int get_data_int(json_object *dataJ);
double get_data_double(json_object *dataJ);

#endif /* _CANOPENGLUE_HEADER_ */