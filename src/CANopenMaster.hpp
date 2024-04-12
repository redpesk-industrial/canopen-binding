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

#ifndef _ServiceCANopenMaster_INCLUDE_
#define _ServiceCANopenMaster_INCLUDE_

#include <list>
#include <memory>
#include <ostream>

#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/coapp/master.hpp>

#include <rp-utils/rp-path-search.h>

#include "common-binding.hpp"
#include "utils/cstrmap.hpp"
#include "CANopenExec.hpp"

class CANopenSlaveDriver;

/**
 * The class CANopenMaster holds a CANopen bus connection
 */
class CANopenMaster
{
public:
	CANopenMaster(CANopenExec &exec);
	~CANopenMaster();
	inline bool isRunning() { return m_can && m_can->isRunning(); }
	inline const char *info() { return m_info; }
	json_object *infoJ();
	int init(json_object *rtuJ, rp_path_search_t *paths);

	int start();
	json_object *statusJ();
	json_object *slaveListInfo(json_object *array);

	inline operator afb_api_t () const { return m_exec; }
	inline operator ev_exec_t*() const { return m_exec; }
	inline operator lely::canopen::BasicMaster&() const { return *m_can; }
	inline const char *uid() const { return m_uid; }
	void dump(std::ostream &os) const;

	template <class T>
	lely::canopen::SdoFuture<T> AsyncRead(uint8_t id, uint16_t idx, uint8_t subidx) {
		return m_can->AsyncRead<T>(id, idx, subidx);
	}

	template <class T>
	lely::canopen::SdoFuture<void> AsyncWrite(uint8_t id, uint16_t idx, uint8_t subidx, T&& value) {
		return m_can->AsyncWrite(id, idx, subidx, std::forward<T>(value));
	}


private:
	/// @brief the single execution handler
	CANopenExec &m_exec;

	/// @brief the CAN open bus/channel handler
	std::shared_ptr<CANopenChannel> m_can;

	/// @brief uid of the master
	const char *m_uid = nullptr;

	/// @brief description of the channel
	const char *m_info = nullptr;

	/// @brief A vector referencing slaves handle by the master
	cstrmap<std::shared_ptr<CANopenSlaveDriver>> m_slaves;

	/// @brief running status
	bool m_isRunning = false;
};

/**
 * The class CANopenMaster holds a CANopen bus connection
 */
class CANopenMasterSet
{
public:
	CANopenMasterSet(CANopenExec &exec) : exec_{exec}, masters_{} {}
	int add(json_object *cfg, rp_path_search_t *paths);
	int start();
	json_object *statusJ();
	void slaveListInfo(json_object *groups);
	void dump(std::ostream &os) const;

private:
	/// the single execution handler
	CANopenExec &exec_;

	/** masters canopen buses */
	cstrmap<std::shared_ptr<CANopenMaster>> masters_;
};

#endif /* _ServiceCANopenMaster_INCLUDE_ */
