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

#include "CANopenSlaveDriver.hpp"

#include <string.h>

#include "CANopenSensor.hpp"
#include "CANopenMaster.hpp"
#include "CANopenGlue.hpp"

#include <rp-utils/rp-jsonc.h>

#include "utils/jsonc.hpp"


static afb_auth_t auth_admin = afb::auth_permission("superadmin");

struct slave_config
{
	const char *uid = NULL;
	const char *info = NULL;
	json_object *onconf = NULL;
	json_object *sensors = NULL;
};


static bool read_config(afb_api_t api, json_object *obj, slave_config &config)
{
	bool ok = true;

	if (!get(api, obj, "uid", config.uid))
		ok = false;

	if (!get(api, obj, "sensors", config.sensors, true, json_type_array))
		ok = false;

	if (!get(api, obj, "info", config.info, false))
		ok = false;

	if (!get(api, obj, "onconf", config.onconf, false, json_type_array))
		ok = false;

	return ok;
}

CANopenSlaveDriver::CANopenSlaveDriver(
	CANopenMaster &master,
	json_object *slaveJ,
	uint8_t nodId
)
	: lely::canopen::BasicDriver(master, master, nodId)
	, m_master{master}
	, m_api{master}
	, m_sensors{}
{
	int err = 0;
	json_object *obj;
	char *adminCmd;
	unsigned idx, count;
	slave_config config;

	if (!read_config(m_api, slaveJ, config))
		return;

	m_uid = config.uid;
	m_info = config.info;
	m_onconfJ = config.onconf;
	m_uid_len = strlen(m_uid);

	err = asprintf(&adminCmd, "%s/%s", uid(), "superadmin");
	if (err < 0)
		throw std::runtime_error(std::string("Fail to create superadmin verb for sensor ") + m_uid);

	err = afb_api_add_verb(m_api, adminCmd, m_info, OnRequest, this, &auth_admin, 0, 0);
	if (err)
		throw std::runtime_error(std::string("Failed to register superadmin verb ") + adminCmd);

#if 1
	err = afb_api_add_verb(m_api, uid(), m_info, OnRequest, this, nullptr, 0, 0);
	if (err)
		throw std::runtime_error(std::string("Failed to register superadmin verb ") + adminCmd);
#endif

	// loop on sensors
	count = (unsigned)json_object_array_length(config.sensors);
	for (idx = 0 ; idx < count ; idx++)
	{
		obj = json_object_array_get_idx(config.sensors, idx);
		AFB_API_DEBUG(m_api, "creation of sensor %s", json_object_to_json_string(obj));
		std::shared_ptr<CANopenSensor> sensor = std::make_shared<CANopenSensor>(*this, obj);
		m_sensors[sensor->uid()] = sensor;
	}
}

void CANopenSlaveDriver::OnRequest(afb_req_t request, unsigned nparams, afb_data_t const params[])
{
	// retrieve action handle from request and execute the request
	CANopenSlaveDriver *slave = reinterpret_cast<CANopenSlaveDriver*>(afb_req_get_vcbdata(request));
	slave->request(request, nparams, params);
}

void CANopenSlaveDriver::request(afb_req_t request, unsigned nparams, afb_data_t const params[])
{

	const char *action;
	afb_data_t data;
	json_object *dataJ = nullptr;
	json_object *regJ;
	json_object *valJ;
	json_object *queryJ;
	int size;
	int err;

	// get the JSON object
	err = afb_req_param_convert(request, 0, AFB_PREDEFINED_TYPE_JSON_C, &data);
	if (err < 0) {
		REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "conversion to JSON failed %d", err);
		return;
	}
	queryJ = reinterpret_cast<json_object*>(afb_data_ro_pointer(data));

	if (!get(request, queryJ, "action", action)) {
		REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "bad or missing action");
		return;
	}
	dataJ = json_object_object_get(queryJ, "data");

	if (!strcasecmp(action, "WRITE"))
	{
		// get write parameters
		err = rp_jsonc_unpack(dataJ, "{so so si!}",
				       "reg", &regJ,
				       "val", &valJ,
				       "size", &size);
		if (err)
		{
			goto invalid_request;
		}

		// decode register
		try
		{
			afb_req_addref(request);
			int32_t reg = get_data_int32(regJ);
			int32_t val = get_data_int32(valJ);
			uint8_t subIdx = (uint8_t)(reg & 0x0ff);
			uint16_t idx = (uint16_t)((reg >> 8) & 0x00ffff);
			lely::canopen::SdoFuture<void> f;
			switch (size)
			{
			case 1:
				f = AsyncWrite<uint8_t>(idx, subIdx, (uint8_t)val);
				break;
			case 2:
				f = AsyncWrite<uint16_t>(idx, subIdx, (uint16_t)val);
				break;
			case 3:
			case 4:
				f = AsyncWrite<uint32_t>(idx, subIdx, (uint32_t)val);
				break;
			default:
				throw std::invalid_argument("Invalid write size (should be 1, 2, 3 or 4)");
			}
			f.then(GetExecutor(), [this, request, idx, subIdx](lely::canopen::SdoFuture<void> f) {
				auto r = f.get();
				if (r.has_error())
				{
					REQFAIL(request, AFB_ERRNO_GENERIC_FAILURE, "Async write of slave %s [0x%x]:[0x%x] failed", uid(), idx, subIdx);
				}
				else
				{
					afb_req_reply(request, 0, 0, NULL);
				}
				afb_req_unref(request);
			});
		}
		catch (std::exception &e)
		{
			REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "Write request error %s for %s, %s",
											e.what(), uid(), json_object_to_json_string(dataJ));
			afb_req_unref(request);
		}
	}
	else if (!strcasecmp(action, "READ"))
	{
		// get read parameters
		err = rp_jsonc_unpack(dataJ, "{so !}",
				       "reg", &regJ);
		if (err)
		{
			goto invalid_request;
		}

		// decode register
		try
		{
			afb_req_addref(request);
			int32_t reg = get_data_int32(regJ);
			uint8_t subIdx = (uint8_t)(reg & 0x0ff);
			uint16_t idx = (uint16_t)((reg >> 8) & 0x00ffff);
			lely::canopen::SdoFuture<uint32_t> f = AsyncRead<uint32_t>(idx, subIdx);
			f.then(GetExecutor(), [this, request, idx, subIdx](lely::canopen::SdoFuture<uint32_t> f) {
				auto r = f.get();
				if (r.has_error())
				{
					REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "Async read of slave %s [0x%x]:[0x%x] failed", uid(), idx, subIdx);
				}
				else
				{
					uint32_t v = (uint32_t)r.value();
					AFB_REQ_DEBUG(request, "Async read of slave %s [0x%x]:[0x%x] returned 0x%x", uid(), idx, subIdx, v);
					afb_req_reply_json_c_hold(request, 0, json_object_new_int64(v));
				}
				afb_req_unref(request);
			});
		}
		catch (std::runtime_error &e)
		{
			REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "Invalid %s register %s => %s (%s)", action, uid(), json_object_get_string(regJ), e.what());
			afb_req_unref(request);
		}
	}
	else
	{
		REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "Invalid action %s  rtu=%s query=%s", action, uid(), json_object_get_string(queryJ));
	}
	return;

invalid_request:
	REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "Invalid 'request' rtu=%s query=%s", uid(), json_object_get_string(queryJ));
}

// IMPORTANT : use this funtion only in the driver exec
int CANopenSlaveDriver::addSensorEvent(CANopenSensor *sensor)
{
	try
	{
		m_sensorEventQueue.insert(m_sensorEventQueue.end(), sensor);
	}
	catch (const std::exception &)
	{
		return -1;
	}
	return 0;
}

// IMPORTANT : use this funtion only int the driver exec
int CANopenSlaveDriver::delSensorEvent(CANopenSensor *sensor)
{
	try
	{
		for (auto q = m_sensorEventQueue.begin(); q != m_sensorEventQueue.end();)
		{
			if (!strcasecmp((*q)->uid(), sensor->uid()))
			{
				q = m_sensorEventQueue.erase(q);
			}
			else
				q++;
		}
	}
	catch (const std::exception &)
	{
		return -1;
	}
	return 0;
}

const char *CANopenSlaveDriver::info()
{
	char *formatedInfo;
	asprintf(&formatedInfo, "slave: '%s', nodId: %d, info: '%s'", uid(), id(), m_info);
	return formatedInfo;
}

json_object *CANopenSlaveDriver::infoJ()
{
	json_object *responseJ;
	json_object *sensorsJ = json_object_new_array();
	for (auto sensor : m_sensors)
	{
		json_object_array_add(sensorsJ, sensor.second->infoJ());
	}
	int err = rp_jsonc_pack(&responseJ, "{ss, ss*, s{ss si} so*}",
				 "uid", uid(),
				 "info", m_info,
				 "status",
					"slave", uid(),
					"nodId", id(),
				 "verbs", sensorsJ);
	if (err)
		responseJ = json_object_new_string("Slave info ERROR !");
	return responseJ;
}

// This function gets called every time a value is written to the local object dictionary of the master
void CANopenSlaveDriver::OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept
{
#if 0
	AFB_API_DEBUG(*this, "-- on RPDO write %s:%04x:%u --", uid(), (unsigned)idx, (unsigned)subidx);
#endif
	// check in the sensor event list
	for (auto sensor : m_sensorEventQueue)
	{
		// If the sensor match, read it and push the event to afb
		if (idx == sensor->reg() && subidx == sensor->subReg())
		{
			sensor->readThenPush();
			break;
		}
	}
}

void CANopenSlaveDriver::OnHeartbeat(bool occurred) noexcept
{
	AFB_API_DEBUG(*this, "-- on heart beat %s:%s --", uid(), occurred ? "true" : "false");
	if (m_connected == occurred)
		AFB_API_NOTICE(*this, "heartbeat %s", occurred ? "timeout, disconnect" : "connect");
	m_connected = !occurred;
}

// This function gets called when the boot-up process of the slave completes.
void CANopenSlaveDriver::OnBoot(lely::canopen::NmtState nmtState, char es, const ::std::string &str) noexcept
{
	char c[2] = { es, 0 };
	AFB_API_DEBUG(*this, "-- on boot %s:%d:%s:%s --", uid(), (int)nmtState, es?c:"nul", str.c_str());

	// if master cycle period is null or undefined set it to 100ms
	int val = master[0x1006][0];
	if (val <= 0)
		master[0x1006][0] = UINT32_C(100000);
}

void CANopenSlaveDriver::dump(std::ostream &os) const
{
	const char *i = "      ";
	os << i << "-- channel --" << std::endl;
	os << i << "uid " << uid() << std::endl;
	os << i << "up? " << (isup() ? "yes" : "no") << std::endl;
	os << i << "id " << id() << std::endl;
	os << i << "netid " << netid() << std::endl;
	os << i << "info " << info() << std::endl;
	for (auto it : m_sensors) {
		it.second->dump(os);
	}
}


// This function gets called during the boot-up process for the slave.
void CANopenSlaveDriver::OnConfig(::std::function<void(::std::error_code ec)> res) noexcept
{
	AFB_API_DEBUG(*this, "-- on config %s --", uid());

	m_connected = false;
	doStartAction(0, [this,res](::std::error_code ec){
		m_connected = true;
		res(ec);
	});
	m_connected = true;
}

void CANopenSlaveDriver::doStartAction(int idxcnf, ::std::function<void(::std::error_code ec)> res) noexcept
{
	const char *actionInfo = "no info action";
	int reg;
	int size;
	int32_t data;
	int err;
	json_object *dataJ;
	json_object *regJ;
	json_object *conf = nullptr;

	// what is to be evaluated now
	if (m_onconfJ)
	{
		if (json_object_is_type(m_onconfJ, json_type_array))
		{
			if (idxcnf < (int)json_object_array_length(m_onconfJ))
				conf = json_object_array_get_idx(m_onconfJ, idxcnf);
		}
		else if (idxcnf == 0)
		{
			conf = m_onconfJ;
		}
	}

	// nothing? ok return without error
	if (!conf) {
		res({});
		return;
	}
	idxcnf++;

	// extract the config
	AFB_API_DEBUG(*this, "%s: StartConfig: (%s)", uid(), json_object_to_json_string(conf));
	err = rp_jsonc_unpack(conf, "{s?s,so,si,so}",
			       "info", &actionInfo,
			       "register", &regJ,
			       "size", &size,
			       "data", &dataJ);
	if (err)
	{
		AFB_API_ERROR(*this, "%s: Fail to parse slave JSON : (%s)", uid(), json_object_to_json_string(conf));
		return doStartAction(idxcnf, res);
	}

	// Get register and sub register from the parsed register
	try
	{
		reg = get_data_int32(regJ);
		data = get_data_int32(dataJ);
	}
	catch (std::runtime_error &e)
	{
		AFB_API_ERROR(*this, "%s: %s (when %s)", uid(), e.what(), actionInfo);
		return doStartAction(idxcnf, res);
	}

	uint16_t idx = ((uint32_t)reg & 0x00ffff00) >> 8;
	uint8_t subIdx = (uint32_t)reg & 0x000000ff;
	AFB_API_NOTICE(*this, "%s: %s (%x > [%x.%x])", uid(), actionInfo, data, idx, subIdx);

	try
	{
		lely::canopen::SdoFuture<void> f;
		switch (size)
		{
		case 1:
			f = AsyncWrite<uint8_t>(idx, subIdx, (uint8_t)data);
			break;
		case 2:
			f = AsyncWrite<uint16_t>(idx, subIdx, (uint16_t)data);
			break;
		case 3:
		case 4:
			f = AsyncWrite<uint32_t>(idx, subIdx, (uint32_t)data);
			break;
		default:
			AFB_API_ERROR(
			    *this,
			    "%s: invalid size %d. Available size (in byte) are 1, 2, 3 or 4",
			    uid(), size);
			return;
		}
		f.then(GetExecutor(), [this,actionInfo,data,idx,subIdx,idxcnf,res](lely::canopen::SdoFuture<void> f){
			auto& result = f.get();
		        if (result.has_error()) {
				try {
					::std::rethrow_exception(result.error());
				} catch (const ::std::system_error& e) {
					AFB_API_ERROR(*this, "%s: error (%x > [%x.%x]), %s", uid(), data, idx, subIdx, e.what());
				} catch (...) {
					// Ignore exceptions we cannot handle.
					AFB_API_ERROR(*this, "%s: error (%x > [%x.%x])", uid(), data, idx, subIdx);
				}
		        }
			else
			{
				AFB_API_DEBUG(*this, "%s: DONE (%s, %x > [%x.%x])", uid(), actionInfo, data, idx, subIdx);
			}
			return doStartAction(idxcnf, res);
		});
	}
	catch (lely::canopen::SdoError &e)
	{
		AFB_API_ERROR(
		    *this,
		    "%s: could not configure register [0x%x][0x%x] with value 0X%x \nwhat() : %s",
		    uid(), idx, subIdx, (uint32_t)data, e.what());
		return doStartAction(idx + 1, res);
	}
}

