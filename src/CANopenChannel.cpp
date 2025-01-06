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

#include "CANopenChannel.hpp"

// it to 128 if it is too small, which requires the CAP_NET_ADMIN capability
// (the reason for this is to ensure proper blocking and polling behavior, see
// section 3.4 in https://rtime.felk.cvut.cz/can/socketcan-qdisc-final.pdf).
// There are two ways to avoid the need for sudo:
// * Increase the size of the transmit queue : ip link set can0 txqueuelen 128
// * Set CanController TX queue length to linux default : CanController ctrl("can0", 10)
//   but this may cause frames to be dropped.
CANopenChannel::CANopenChannel(CANopenExec &exec, const char *uri, const char *dcf, uint8_t nodID, size_t rxlen)
	: exec_{exec}
	, uri_{uri}
	, dcf_{dcf}
	, ctrl_{uri, rxlen}
	, chan_{exec, exec}
	, master_{exec, exec, chan_, dcf, "", nodID}
{
#if 1
        if(afb_api_wants_log_level(*this, AFB_SYSLOG_LEVEL_DEBUG)) {
		master_.OnRpdoWrite([this](uint8_t id, uint16_t idx, uint8_t subidx) {
			AFB_API_DEBUG(*this, "OnRpdoWrite(id=%d, idx=%d, subidx=%d)", int(id), int(idx), int(subidx));
		});
	}
#endif
}

void CANopenChannel::reset()
{
	master_.Reset();
}

void CANopenChannel::start()
{
	chan_.open(ctrl_);
	reset();
	is_running_ = true;
}

void CANopenChannel::dump(std::ostream &os) const
{
	const char *i = "   ";
	os << i << "uri " << uri() << std::endl;
	os << i << "run? " << (isRunning() ? "yes" : "no") << std::endl;
	os << i << "dcf " << dcf() << std::endl;
}
