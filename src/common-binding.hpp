/*
 Copyright (C) 2015-2024 IoT.bzh Company

 Author: IoT.bzh <team@iot.bzh>

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

#ifndef _COMMON_BINDING_INCLUDED_
#define _COMMON_BINDING_INCLUDED_

#include <json-c/json.h>

#include <afb/afb-binding>
#include <afb-helpers4/afb-data-utils.h>
#include <afb-helpers4/afb-req-utils.h>
#include <afb-helpers4/ctl-lib.h>

#if !defined(VERBOSE_ERROR_REPLY)
# define VERBOSE_ERROR_REPLY 1
#endif

#if VERBOSE_ERROR_REPLY
# define REQERRF(req,code,...)		afb_req_reply_string_f(req, code, __VA_ARGS__)
#else
# define REQERRF(req,code,...)		afb_req_reply_string_f(req, code, 0, NULL)
#endif

#define REQFAIL(req,code,...) do {\
		AFB_REQ_WARNING(req, __VA_ARGS__); \
		REQERRF(req, code, __VA_ARGS__); \
	} while(0)

#define APITHROW(api,...) do {\
        char *str; \
        (void)asprintf(&str,__VA_ARGS__); \
        AFB_API_ERROR(api,"%s",str); \
	std::runtime_error e(str); \
        free(str); \
        throw e; \
    } while(0);

#endif // _COMMON_BINDING_INCLUDED_