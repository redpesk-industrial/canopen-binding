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

#pragma once

#include "../common-binding.hpp"

bool get(afb_api_t api, json_object *obj, const char *key, json_object *&item, bool mandatory = true, json_type type = json_type_null);
bool get(afb_req_t req, json_object *obj, const char *key, json_object *&item, bool mandatory = true, json_type type = json_type_null);

bool get(afb_api_t api, json_object *obj, const char *key, const char *&item, bool mandatory = true);
bool get(afb_req_t req, json_object *obj, const char *key, const char *&item, bool mandatory = true);

bool get(afb_api_t api, json_object *obj, const char *key, int &item, bool mandatory = true);
bool get(afb_req_t req, json_object *obj, const char *key, int &item, bool mandatory = true);