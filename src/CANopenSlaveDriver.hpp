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

#ifndef _CANOPENSLAVEDRIVER_INCLUDE_
#define _CANOPENSLAVEDRIVER_INCLUDE_

#include <set>
#include <ostream>

#include <lely/coapp/driver.hpp>

#include "common-binding.hpp"

class CANopenSensor;

#include "CANopenMaster.hpp"
#include "utils/cstrmap.hpp"

class CANopenSlaveDriver : public lely::canopen::BasicDriver
{
public:
	using lely::canopen::BasicDriver::BasicDriver;

	CANopenSlaveDriver(
		CANopenMaster &master,
		json_object *slaveJ,
		uint8_t nodId);

	// IMPORTANT : use this funtion only in the driver exec
	int addSensorEvent(CANopenSensor *sensor);

	// IMPORTANT : use this funtion only in the driver exec
	int delSensorEvent(CANopenSensor *sensor);

	json_object *infoJ();
	const char *info();

	inline size_t uid_length() const { return m_uid_len; }
	inline const char *uid() const { return m_uid; }
	inline const char *info() const { return m_info; }
	inline bool isup() const { return m_connected; }
	inline operator afb_api_t() const { return m_api; }
	inline operator CANopenMaster&() { return m_master; }
	inline operator ev_exec_t*() const { return m_master; }
	void dump(std::ostream &os) const;

	template <class T>
	lely::canopen::SdoFuture<T> AsyncRead(uint16_t idx, uint8_t subidx) {
		return m_master.AsyncRead<T>(id(), idx, subidx);
	}

	template <class T>
	lely::canopen::SdoFuture<void> AsyncWrite(uint16_t idx, uint8_t subidx, T&& value) {
		return m_master.AsyncWrite(id(), idx, subidx, std::forward<T>(value));
	}


private:
	void request(afb_req_t request, unsigned nparams, afb_data_t const params[]);

private:
	CANopenMaster &m_master;
	afb_api_t m_api = nullptr;
	const char *m_uid = nullptr;
	unsigned m_uid_len = 0;
	const char *m_info = nullptr;
	uint m_count = 0;
	bool m_connected = true;//false;
	cstrmap<std::shared_ptr<CANopenSensor>> m_sensors;
	std::set<CANopenSensor *> m_sensorEventQueue;
	json_object *m_onconfJ = nullptr;

	void doStartAction(int idx, ::std::function<void(::std::error_code ec)> res) noexcept;

	void request_read(afb_req_t request, uint32_t reg) noexcept;
	void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override;
	void OnHeartbeat(bool occurred) noexcept override;
	void OnBoot(lely::canopen::NmtState nmtState, char es, const ::std::string &) noexcept override;
	void OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override;
	static void OnRequest(afb_req_t request, unsigned nparams, afb_data_t const params[]);
};

#endif /* _CANOPENSLAVEDRIVER_INCLUDE_ */
