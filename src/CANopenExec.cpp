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

#include "CANopenExec.hpp"

#include "utils/utils.hpp"

#include "CANopenChannel.hpp"

CANopenExec::CANopenExec(afb_api_t api)
	: lely::io::Context{}
	, lely::io::Poll{*this, 0}
	, lely::ev::Loop{io_poll_get_poll(*this), 1}
	, lely::io::Timer{*this, *this, CLOCK_MONOTONIC}
	, api_(api)
	, io_guard_{}
	, channels_{}
{
}

CANopenExec::~CANopenExec()
{
	cleanDcfRequires();
}

std::shared_ptr<CANopenChannel> CANopenExec::open(const char *uri, const char *dcf, uint8_t nodID)
{
	// Check if master DCF require Upload Files(slave configuration SDO binary file)
	// and make symbolic links to them in working directory so master can find them.
	if (fixDcfRequires(dcf) != 0)
		AFB_API_WARNING(*this, "One or more UploadFile required by master DCF could not be linked");

	std::shared_ptr<CANopenChannel> channel(new CANopenChannel(*this, uri, dcf, nodID));
	channels_.push_back(channel);
	return channel;
}

struct synchro_exec_start
{
	CANopenExec *exec;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	synchro_exec_start(CANopenExec *e)
		: exec(e)
		, mutex(PTHREAD_MUTEX_INITIALIZER)
		, cond(PTHREAD_COND_INITIALIZER)
		{ pthread_mutex_lock(&mutex); }
	void wait()
		{ pthread_cond_wait(&cond, &mutex); }
	void signal()
		{ pthread_mutex_lock(&mutex);
		  pthread_cond_signal(&cond);
		  pthread_mutex_unlock(&mutex); }
	~synchro_exec_start()
		{ pthread_mutex_unlock(&mutex); }
};

int CANopenExec::start()
{
	synchro_exec_start synchro(this);
	pthread_create(&evl, NULL, run_, &synchro);
	synchro.wait();
	return 0;
}

void *CANopenExec::run_(void *arg)
{
	synchro_exec_start *synchro(reinterpret_cast<synchro_exec_start*>(arg));
	CANopenExec *me = synchro->exec;
	synchro->signal();
	for(;;) {
		me->run();
//		me->lely::io::Poll::get_poll().wait(-1);
	}
	return nullptr;
}

