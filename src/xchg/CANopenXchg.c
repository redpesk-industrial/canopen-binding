/*
 Copyright (C) 2015-2024 IoT.bzh Company

 Author: José Bollo <jose.bollo@iot.bzh>

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

#include "CANopenXchg.h"
#include <stdatomic.h>

#define canopen_xchg_v1_req_typename   "canopen-xchg-v1-req"
#define canopen_xchg_v1_value_typename "canopen-xchg-v1-value"

afb_type_t canopen_xchg_v1_req_type;
afb_type_t canopen_xchg_v1_value_type;

int canopen_xchg_init()
{
	int rc = afb_type_lookup(&canopen_xchg_v1_req_type, canopen_xchg_v1_req_typename);
	if (rc < 0)
		rc = afb_type_register(&canopen_xchg_v1_req_type, canopen_xchg_v1_req_typename, 0);
	if (rc >= 0) {
		rc = afb_type_lookup(&canopen_xchg_v1_value_type, canopen_xchg_v1_value_typename);
		if (rc < 0)
			rc = afb_type_register(&canopen_xchg_v1_value_type, canopen_xchg_v1_value_typename, 0);
	}
	return rc;
}
