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

#include "CANopenSensor.hpp"

#include <iostream>
#include <string.h>

#include "CANopenSlaveDriver.hpp"
#include "CANopenEncoder.hpp"
#include "CANopenGlue.hpp"

#include <rp-utils/rp-jsonc.h>

#include "utils/jsonc.hpp"

struct sensor_config
{
	const char *uid = NULL;
	const char *type = NULL;
	json_object *reg = NULL;
	const char *format = NULL;
	int size = 0;
	const char *info = NULL;
	const char *privilege = NULL;
	json_object *args = NULL;
	json_object *sample = NULL;
};

static bool read_config(afb_api_t api, json_object *obj, sensor_config &config)
{
	bool ok = true;
	json_object *item;

	if (!get(api, obj, "uid", item, json_type_string, true))
		ok = false;
	else
		config.uid = json_object_get_string(item);

	if (!get(api, obj, "type", item, json_type_string, true))
		ok = false;
	else
		config.type = json_object_get_string(item);

	if (!get(api, obj, "register", item, json_type_null, true))
		ok = false;
	else
		config.reg = item;

	if (!get(api, obj, "format", item, json_type_string, true))
		ok = false;
	else
		config.format = json_object_get_string(item);

	if (!get(api, obj, "size", item, json_type_int, true))
		ok = false;
	else
		config.size = json_object_get_int(item);

	if (!get(api, obj, "info", item, json_type_string, false))
		ok = false;
	else
		config.info = json_object_get_string(item);

	if (!get(api, obj, "info", item, json_type_string, false))
		ok = false;
	else
		config.info = json_object_get_string(item);

	if (!get(api, obj, "args", item, json_type_null, false))
		ok = false;
	else
		config.args = item;

	if (!get(api, obj, "sample", item, json_type_null, false))
		ok = false;
	else
		config.sample = item;

	return ok;
}

void CANopenSensor::sensorDynRequest(afb_req_t request, unsigned nparams, afb_data_t const params[])
{
	// retrieve action handle from request and execute the request
	CANopenSensor *sensor = reinterpret_cast<CANopenSensor*>(afb_req_get_vcbdata(request));
	sensor->request(request, nparams, params);
}

CANopenSensor::CANopenSensor(CANopenSlaveDriver &driver, json_object *sensorJ)
	: m_driver{driver}
	, m_verb{}
{
	int err = 0;
	int idx;
	afb_auth_t *authent = NULL;
	sensor_config config;

	// should already be allocated
	assert(sensorJ);

	// set default values
	m_sample = nullptr;

	if (!read_config(*this, sensorJ, config)) {
		APITHROW(*this, "failed to parse sensor config %s", json_object_to_json_string(sensorJ));
	}
	m_uid = config.uid;
	m_format = config.format;
	m_size = config.size;
	m_info = config.info;
	m_sample = config.sample;

	// Get sensor register and sub register from the parsed register
	try
	{
		idx = get_data_int32(config.reg);
	}
	catch (std::runtime_error &e)
	{
		APITHROW(*this, "sensor %s, error at register convert %s: %s", m_uid, json_object_to_json_string(config.reg), e.what());
	}
	m_subRegister = (uint8_t)(idx & 0x0ff);
	m_register = (uint16_t)((idx >> 8) & 0x0ffff);

	// create autentification for sensor
	if (config.privilege)
	{
		authent = &m_auth;
		m_auth.type = afb_auth_Permission;
		m_auth.text = config.privilege;
	}

	// load Encoder
	CANopenEncoder *coEncoder = &CANopenEncoder::instance();

	// Get the appropriate read/write callbacks
	try
	{
		m_function = coEncoder->getfunctionCB(config.type, m_size);
	}
	catch (std::out_of_range &)
	{
		APITHROW(*this, "sensor %s, unknown type %s for size %d", m_uid, config.type, m_size);
	}

	// Get the encode formater
	if (m_function.writeCB || m_function.writeAsyncCB)
	{
		try
		{
			m_encode = coEncoder->getEncodeFormaterCB(m_format);
		}
		catch (std::out_of_range &)
		{
			m_encode = nullptr;
			APITHROW(*this, "sensor %s, unknown format %s", m_uid, m_format);
		}
		if (!m_function.writeCB)
			m_function.writeCB = write_sync;
	}

	// Get the decode Formater
	if (m_function.readCB || m_function.readAsyncCB)
	{
		try
		{
			m_decode = coEncoder->getDecodeFormaterCB(m_format);
		}
		catch (std::out_of_range &)
		{
			APITHROW(*this, "sensor %s, could not find sensor decode formater %s", m_uid, m_format);
		}

		// register the event
		err = afb_api_new_event(*this, m_uid, &m_event);
		if (err < 0)
		{
			m_decode = nullptr;
			APITHROW(*this, "sensor %s, fail to create event", m_uid);
		}
	}

	memset(&m_currentVal, 0, sizeof m_currentVal);

	// create the verb for the sensor

	m_verb.reserve(2 + m_driver.uid_length() + strlen(m_uid));
	(m_verb.append(m_driver.uid()) += '/').append(m_uid);
	err = afb_api_add_verb(*this, m_verb.c_str(), m_info, sensorDynRequest, reinterpret_cast<void*>(this), authent, 0, 0);
	if (err)
	{
		APITHROW(*this, "sensor %s, fail to register verb %s", m_uid, m_verb.c_str());
	}
}

void CANopenSensor::request(afb_req_t request, unsigned nparams, afb_data_t const params[])
{
	json_object *queryJ;
	const char *action;
	json_object *obj;
	json_object *dataJ = nullptr;
	afb_data_t data;
	int err;
	enum { None, Read, Write, Subscribe, Unsubscribe } act = None;

	// get the JSON object
	err = afb_req_param_convert(request, 0, AFB_PREDEFINED_TYPE_JSON_C, &data);
	if (err < 0) {
		REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "conversion to JSON failed %d", err);
		return;
	}
	queryJ = reinterpret_cast<json_object*>(afb_data_ro_pointer(data));

	// parse request
	if (!get(request, queryJ, "action", obj, json_type_string, true)) {
		REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "bad or missing action");
		return;
	}
	action = json_object_get_string(obj);
	dataJ = json_object_object_get(queryJ, "data");

	// parse the action
	if (!strcasecmp(action, "WRITE"))
		act = Write;
	else if (!strcasecmp(action, "READ"))
		act = Read;
	else if (!strcasecmp(action, "SUBSCRIBE"))
		act = Subscribe;
	else if (!strcasecmp(action, "UNSUBSCRIBE"))
		act = Unsubscribe;

	// check validity of action
	if  (act == None)
	{
		REQFAIL(request, AFB_ERRNO_INVALID_REQUEST,
			"unknown action=%s rtu=%s sensor=%s query=%s", action, m_driver.uid(), m_uid, json_object_get_string(queryJ));
	}
	// check validity of the state
	else if (!m_driver.isup() && (act == Write || act == Read)) {
		REQFAIL(request, AFB_ERRNO_BAD_API_STATE, "slave %s is not running", m_driver.uid());
	}

	// process write
	else if (act == Write)
	{
		// synchronous read
		try {
			err = write(dataJ);
			if (err)
			{
				REQFAIL(request, AFB_ERRNO_NOT_AVAILABLE, "No write function available for %s: %s", m_driver.uid(), m_uid);
			}
			else
			{
				// everything looks good let's respond
				afb_req_reply(request, 0, 0, NULL);
			}
		}
		catch (lely::canopen::SdoError &e)
		{
			REQFAIL(request, AFB_ERRNO_GENERIC_FAILURE, "Fail to write slave=%s sensor=%s: %s", m_driver.uid(), m_uid, e.what());
		}
	}

	// below methods are bound to read capability
	else if (!m_decode)
	{
		REQFAIL(request, AFB_ERRNO_NOT_AVAILABLE, "No read function available for %s: %s", m_driver.uid(), m_uid);
	}
	else if (act == Read)
	{
		if (m_function.readCB)
		{
			// synchronous read
			try {
				m_currentVal = m_function.readCB(this);
				afb_req_reply_json_c_hold(request, 0, m_decode(m_currentVal, this));
			}
			catch (lely::canopen::SdoError &e)
			{
				REQFAIL(request, AFB_ERRNO_GENERIC_FAILURE, "Fail to read slave=%s sensor=%s: %s", m_driver.uid(), m_uid, e.what());
			}
		}
		else
		{
			// asynchronous read
			afb_req_addref(request);
			m_function.readAsyncCB(this).then(
				m_driver,
				[request, this](lely::canopen::SdoFuture<COdataType> f) {
					try
					{
						// getting the value can raise the exception
						m_currentVal = f.get().value();
						afb_req_reply_json_c_hold(request, 0, m_decode(m_currentVal, this));
					}
					catch (std::exception &e)
					{
						afb_req_reply_string_f(request, AFB_ERRNO_INTERNAL_ERROR, "read error: %s", e.what());
					}
					afb_req_unref(request);
				}
			);
		}
	}

	// implement the subscriptions
	else if (act == Subscribe)
	{
		err = m_driver.addSensorEvent(this);
		if (err >= 0)
		{
			err = afb_req_subscribe(request, m_event);
		}
		if (err >= 0)
		{
			afb_req_reply(request, 0, 0, NULL);
		}
		else
		{
			REQFAIL(request, AFB_ERRNO_GENERIC_FAILURE, "Fail to subscribe slave=%s sensor=%s", m_driver.uid(), m_uid);
		}
	}
	else if (act == Unsubscribe)
	{
		err = afb_req_unsubscribe(request, m_event);
		if (err >= 0)
		{
			afb_req_reply(request, 0, 0, NULL);
		}
		else
		{
			REQFAIL(request, AFB_ERRNO_GENERIC_FAILURE, "Fail to subscribe slave=%s sensor=%s", m_driver.uid(), m_uid);
		}
	}
}

void CANopenSensor::push()
{
	json_object *jval = m_decode(m_currentVal, this);
	afb_data_t dval = afb_data_json_c_hold(jval);
	int sts = afb_event_push(m_event, 1, &dval);
	if (sts <= 0)
	{
		if (sts < 0)
		{
			AFB_API_ERROR(m_driver, "event push error slave=%s sensor=%s", m_driver.uid(), m_uid);
		}
		m_driver.delSensorEvent(this);
	}
}

void CANopenSensor::readThenPush()
{
	if (m_function.readCB)
	{
		// synchronous read
		try {
			m_currentVal = m_function.readCB(this);
			push();
		}
		catch (lely::canopen::SdoError &e)
		{
			AFB_API_ERROR(*this, "Fail to read slave=%s sensor=%s: %s", m_driver.uid(), m_uid, e.what());
		}
	}
	else
	{
		// asynchronous read
		m_function.readAsyncCB(this).then(
			m_driver,
			[this](lely::canopen::SdoFuture<COdataType> f) {
				// getting the value can raise the exception
				try
				{
					m_currentVal = f.get().value();
					push();
				}
				catch(std::exception &e)
				{
					AFB_API_ERROR(m_driver, "event push error slave=%s sensor=%s", m_driver.uid(), m_uid);
				}
			}
		);
	}
}

void CANopenSensor::write_sync(CANopenSensor *sensor, COdataType data)
{
	sensor->m_function.writeAsyncCB(sensor, data);
}

int CANopenSensor::write(json_object *output)
{
	if (!m_encode)
		return AFB_ERRNO_NOT_AVAILABLE;
	m_currentVal = m_encode(output, this);
	m_function.writeCB(this, m_currentVal);
	return 0;
}

char *CANopenSensor::info()
{
	char *formatedInfo;
	asprintf(&formatedInfo, "%s/%s [%s%s%s]",
					m_driver.uid(), m_uid,
					m_encode ? "WRITE" : "",
					m_encode && m_decode ? "|" : "",
					m_decode ? "READ|SUBSCRIBE|UNSUBSCRIBE" : ""
				);
	return formatedInfo;
}

json_object *CANopenSensor::infoJ()
{
	json_object *result;
	char *infostr = info();

	rp_jsonc_pack(&result, "{ss ss* ss* s{s[s*s*s*s*] ss} sO*}",
					"uid", m_uid,
					"info", infostr,
					"verb", m_verb.c_str(),
					"usage",
						"action",
							m_encode ? "write" : NULL,
							m_decode ? "read" : NULL,
							m_decode ? "subscribe" : NULL,
							m_decode ? "unsubscribe" : NULL,
						"data", m_format,
					"sample", m_sample
	);

	free(infostr);
	return result;
}

void CANopenSensor::dump(std::ostream &os) const
{
	const char *i = "         ";
	os << i << "-- sensor --" << std::endl;
	os << i << "uid " << m_uid << std::endl;
	os << i << "verb " << m_verb << std::endl;
	os << i << "fmt " << m_format << std::endl;
	os << i << "size " << int(m_size) << std::endl;
	os << i << "reg " << int(m_register) << std::endl;
	os << i << "subreg " << int(m_subRegister) << std::endl;
}
