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

#ifndef _CANOPENSENSOR_INCLUDE_
#define _CANOPENSENSOR_INCLUDE_

#include <string>
#include <ostream>

#include "common-binding.hpp"
#include "CANopenEncoder.hpp"
#include "CANopenSlaveDriver.hpp"

class CANopenSensorId
{
private:
	uint32_t id_;
public:
	inline CANopenSensorId() : id_{0} {}
	inline CANopenSensorId(uint32_t id) : id_{id} {}
	inline CANopenSensorId(uint16_t reg, uint8_t subreg) : id_{(((uint32_t)reg) << 8) | (uint32_t)subreg} {}
	inline operator uint32_t() const { return id_; }
	inline uint32_t id() const { return id_; }
	inline uint16_t reg() const { return (uint16_t)(id_ >> 8); }
	inline uint8_t subreg() const { return (uint8_t)id_; }
	inline bool operator <(const CANopenSensorId &other) const { return id_ < other.id_; }
};

class CANopenSensor
{
public:
	CANopenSensor(CANopenSlaveDriver &driver, json_object *sensorJ);

	// return information about the sensor
	char *info();
	json_object *infoJ();

	inline coEncodeCB encoder() { return m_encode; }
	inline coDecodeCB decoder() { return m_decode; }
	inline const char *uid() const { return m_uid; }
	inline const std::string &verb() const { return m_verb; }
	inline const char *event_name() const { return m_uid; }
	inline CANopenSensorId id() const { return m_id; }
	inline uint16_t reg() const { return m_id.reg(); }
	inline uint8_t subReg() const { return m_id.subreg(); }
	inline int size() const { return m_size; }
	inline COdataType currentVal() const { return m_currentVal; }

	inline operator CANopenSlaveDriver&() { return m_driver; }
	inline CANopenSlaveDriver *driver() { return &m_driver; }
	inline operator afb_api_t() const { return m_driver; }
	inline operator ev_exec_t*() { return m_driver; }

	void readThenPush();

	template <class T>
	lely::canopen::SdoFuture<T> AsyncRead() {
		return m_driver.AsyncRead<T>(m_id.reg(), m_id.subreg());
	}

	template <class T>
	lely::canopen::SdoFuture<void> AsyncWrite(T&& value) {
		return m_driver.AsyncWrite(m_id.reg(), m_id.subreg(), std::forward<T>(value));
	}

	void dump(std::ostream &os) const;

private:
	CANopenSlaveDriver &m_driver;
	const char *m_uid;
	const char *m_info{""};
	const char *m_privileges;
	const char *m_format;
	std::string m_verb;
	CANopenSensorId m_id;
	json_object *m_sample{nullptr};
	afb_auth_t m_auth;

	// number of Bytes contained by the sensor register
	// 1 = 8bits 2 = 16bits 4 = 32bits 5 = string
	int m_size;
	afb_event_t m_event = nullptr;
	bool m_event_active = false;

	// read/write callback functions
	CANopenEncodeCbS m_function;

	// Formating encoder/decoder callback
	coEncodeCB m_encode = nullptr;
	coDecodeCB m_decode = nullptr;

	// store curent state value
	COdataType m_currentVal;

private:
	static void write_sync(CANopenSensor *sensor, COdataType data);

	static void sensorDynRequest(afb_req_t request, unsigned nparams, afb_data_t const params[]);

	// handle request coming from afb
	void request(afb_req_t request, unsigned nparams, afb_data_t const params[]);

	// push the current value
	void push();

	// Handle Write request
	int write(json_object *inputJ);
};

#endif /* _CANOPENSENSOR_INCLUDE_ */
