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

#ifndef _ServiceCANopenExec_INCLUDE_
#define _ServiceCANopenExec_INCLUDE_

#include <pthread.h>
#include <memory>
#include <list>
#include <ostream>

#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/ev/loop.hpp>
#include <lely/coapp/master.hpp>
#include <lely/coapp/loop_driver.hpp>
#include <lely/util/util.h>

#include "common-binding.hpp"

class CANopenChannel;
class CANopenExec;

class CANopenExec final
	: public lely::io::Context
	, public lely::io::Poll
	, public lely::ev::Loop
	, public lely::io::Timer
{
public:
	CANopenExec(afb_api_t api);
	~CANopenExec();
//	inline bool isRunning() { return is_running_; }
//	int start();

	std::shared_ptr<CANopenChannel> open(const char *uri, const char *dcf, uint8_t nodId);

	inline operator afb_api_t () const { return api_; }
	inline void set(afb_api_t api) { api_ = api; }
	inline operator ev_exec_t*() { return lely::ev::Loop::get_executor(); }

	int start();

private:
	afb_api_t api_;
	lely::io::IoGuard io_guard_;
	std::list<std::shared_ptr<CANopenChannel>> channels_;
struct node { node*next; CANopenChannel*item; } *head_ = nullptr;

	pthread_t evl = 0;
	static void *run_(void*);
};

#endif // _ServiceCANopenExec_INCLUDE_
