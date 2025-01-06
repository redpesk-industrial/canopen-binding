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

#ifndef _ServiceCANopenChannel_INCLUDE_
#define _ServiceCANopenChannel_INCLUDE_

#include <ostream>

#include <lely/coapp/master.hpp>

#include "common-binding.hpp"

#include "CANopenExec.hpp"

class CANopenExec;

class CANopenChannel
{
public:
	CANopenChannel(CANopenExec &exec, const char *uri, const char *dcf, uint8_t nodId, size_t rxlen = 0);
	~CANopenChannel() {}
	inline operator afb_api_t () const { return exec_; }
	inline bool isRunning() const { return is_running_; }
	inline operator lely::canopen::BasicMaster&() { return master_; }
	inline operator const lely::canopen::BasicMaster&() const { return master_; }
	inline const char *uri() const { return uri_.c_str(); }
	inline const char *dcf() const { return dcf_.c_str(); }
	void dump(std::ostream &os) const;

	void start();
	void reset();

	template <class T>
	lely::canopen::SdoFuture<T>
	AsyncRead(uint8_t id, uint16_t idx, uint8_t subidx) {
		return master_.AsyncRead<T>(exec_, id, idx, subidx);
	}

	template <class T>
	lely::canopen::SdoFuture<void>
	AsyncWrite(uint8_t id, uint16_t idx, uint8_t subidx, T&& value) {
		return master_.AsyncWrite(exec_, id, idx, subidx, ::std::forward<T>(value));
	}

	template <class T>
	T get(uint8_t id, uint16_t idx, uint8_t subidx) const {
		return master_.RpdoMapped(id)[idx][subidx];
	}

	using ConstSubObject = lely::canopen::BasicMaster::ConstSubObject;

	ConstSubObject rpdo(uint8_t id, uint16_t idx, uint8_t subidx) const {
		if (id == 0)
			return master_[idx][subidx];
		return master_.RpdoMapped(id)[idx][subidx];
	}

	ConstSubObject tpdo(uint8_t id, uint16_t idx, uint8_t subidx) const {
		if (id == 0)
			return master_[idx][subidx];
		// tricks for getting a TPDO
		const auto tm = const_cast<CANopenChannel*>(this)->master_.TpdoMapped(id);
		return tm[idx][subidx];
	}

private:
	CANopenExec &exec_;
	std::string uri_;
	std::string dcf_;
	lely::io::CanController ctrl_;
	lely::io::CanChannel chan_;
	lely::canopen::BasicMaster master_;
	bool is_running_ = false;
};



#endif // _ServiceCANopenChannel_INCLUDE_
