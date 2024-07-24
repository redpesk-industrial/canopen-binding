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

#ifndef _CANOPENXCHG_INCLUDE_
#define _CANOPENXCHG_INCLUDE_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <afb/afb-binding.h>

#define canopen_xchg_version 1

#define canopen_xchg_u8    0
#define canopen_xchg_i8    1
#define canopen_xchg_u16   2
#define canopen_xchg_i16   3
#define canopen_xchg_u32   4
#define canopen_xchg_i32   5
#define canopen_xchg_u64   6
#define canopen_xchg_i64   7

/* versioned structure */
typedef
struct canopen_xchg_v1_req_s
{
	uint8_t  itf;      /* index of the interface */
	uint8_t  id;       /* slave id or 0 for master SDO */
	uint16_t reg;      /* register index of the PDO mapped value */
	uint8_t  subreg;   /* sub-register index of the PDO mapped value */
	uint8_t  type;     /* type of the value (see canopen_xchg_(i|u)(8|16|32|64))*/
	uint8_t  tpdo;     /* boolean telling if TPDO (otherwise RPDO) */
}
	canopen_xchg_v1_req_t;

typedef
union canopen_xchg_v1_value_s
{
	uint8_t  u8;
	int8_t   i8;
	uint16_t u16;
	int16_t  i16;
	uint32_t u32;
	int32_t  i32;
	uint64_t u64;
	int64_t  i64;
}
	canopen_xchg_v1_value_t;

extern afb_type_t canopen_xchg_v1_req_type;
extern afb_type_t canopen_xchg_v1_value_type;

extern int canopen_xchg_init();

#ifdef __cplusplus
}
#endif

#endif
