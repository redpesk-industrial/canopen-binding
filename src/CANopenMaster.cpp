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

//to pass lely loop event fd to redpesk
#include <systemd/sd-event.h>

#include <string.h>

#include "CANopenMaster.hpp"
#include "CANopenSlaveDriver.hpp"

static int ServiceCANopenMasterHandler(sd_event_source *, int, uint32_t, void *userdata)
{
	lely::ev::Poll poll(static_cast<ev_poll_t *>(userdata));
	poll.wait(0);
	return 0;
}

char *fullPathToDCF(afb_api_t api, char *dcfFile)
{
	// We load 1st file others are just warnings
	size_t p_length;
	char *filepath = NULL;
	const char *filename;
	char *fullpath;
	const char *envConfig;

	envConfig = getenv("CONTROL_CONFIG_PATH");
	if (!envConfig)
	{
		AFB_API_NOTICE(api, "Using default environnement config path : %s", CONTROL_CONFIG_PATH);
		envConfig = CONTROL_CONFIG_PATH;
	}
	else
		AFB_API_NOTICE(api, "Found environnement config path : %s", envConfig);

	asprintf(&fullpath, "%s:%s/etc", envConfig, GetBindingDirPath(api));
	AFB_API_NOTICE(api, "DCF config directory : %s", fullpath);

	json_object *responseJ = ScanForConfig(fullpath, CTL_SCAN_RECURSIVE, dcfFile, "");

	for (int index = 0; index < json_object_array_length(responseJ); index++)
	{
		json_object *entryJ = json_object_array_get_idx(responseJ, index);

		int err = wrap_json_unpack(entryJ, "{s:s, s:s !}",
					   "fullpath", &fullpath,
					   "filename", &filename);
		if (err)
		{
			AFB_API_ERROR(api, "Invalid DCF entry= %s", json_object_get_string(entryJ));
		}

		if (index == 0)
		{
			p_length = strlen(fullpath) + 1 + strlen(filename);
			filepath = (char *)malloc(p_length + 1);

			strncpy(filepath, fullpath, p_length);
			strncat(filepath, "/", p_length - strlen(filepath));
			strncat(filepath, filename, p_length - strlen(filepath));
		}
		else
		{
			AFB_API_WARNING(api, "DCF file found but not used : %s/%s", fullpath, filename);
		}
	}

	json_object_put(responseJ);
	return filepath;
}

CANopenMaster::CANopenMaster(afb_api_t api, json_object *rtuJ, uint8_t nodId /*= 1*/):
	m_poll{m_ctx},
	m_loop{m_poll},
	m_exec{m_loop.get_executor()},
	m_timer{m_poll, m_exec, CLOCK_MONOTONIC},
	m_chan{m_poll, m_exec}
{
	int err = 0;
	json_object *slavesJ = NULL;

	assert(rtuJ);
	assert(api);

	err = wrap_json_unpack(rtuJ, "{ss,s?s,ss,s?s,s?i,so !}",
			       "uid", &m_uid,
			       "info", &m_info,
			       "uri", &m_uri,
			       "dcf", &m_dcf,
			       "nodId", &m_nodId,
			       "slaves", &slavesJ);
	if (err)
	{
		AFB_API_ERROR(api, "Fail to parse rtu JSON : (%s)", json_object_to_json_string(rtuJ));
		return;
	}

	// Find the path of the master description file
	m_dcf = fullPathToDCF(api, m_dcf);
	if (!m_dcf)
		return;
	AFB_API_NOTICE(api, "found DCF file at %s", m_dcf);

	// load and connect CANopen controleur
	try
	{
		// On linux physical can, the default TX queue length is 10. CanController sets
		// it to 128 if it is too small, which requires the CAP_NET_ADMIN capability
		// (the reason for this is to ensure proper blocking and polling behavior, see
		// section 3.4 in https://rtime.felk.cvut.cz/can/socketcan-qdisc-final.pdf).
		// There are two ways to avoid the need for sudo:
		// * Increase the size of the transmit queue : ip link set can0 txqueuelen 128
		// * Set CanController TX queue length to linux default : CanController ctrl("can0", 10)
		//   but this may cause frames to be dropped.
		m_ctrl = std::make_shared<lely::io::CanController>(m_uri);
	}
	catch (std::system_error &e)
	{
		AFB_API_ERROR(api, "CANopenLoadOne: fail to connect can uid=%s uri=%s '%s'", m_uid, m_uri, e.what());
		return;
	}
	m_chan.open(*m_ctrl);
	m_master = std::make_shared<lely::canopen::AsyncMaster>(m_timer, m_chan, m_dcf, "", m_nodId);
	if (!m_chan.is_open())
	{
		AFB_API_ERROR(api, "CANopenLoadOne: fail to connect can uid=%s uri=%s", m_uid, m_uri);
		return;
	}

	// loop on slaves
	json_object *slaveNodId;
	if (json_object_is_type(slavesJ, json_type_array))
	{
		int count = (int)json_object_array_length(slavesJ);
		m_slaves = std::vector<std::shared_ptr<CANopenSlaveDriver>>((size_t)count);
		for (int idx = 0; idx < count; idx++)
		{
			json_object *slaveJ = json_object_array_get_idx(slavesJ, idx);
			//std::cout << "DEBUG : Slave Json = " << json_object_get_string(slaveJ) << "\n";
			json_object_object_get_ex(slaveJ, "nodId", &slaveNodId);
			m_slaves[idx] = std::make_shared<CANopenSlaveDriver>(m_exec, *m_master, api, slaveJ, json_object_get_int(slaveNodId));
		}
	}
	else
	{
		m_slaves = std::vector<std::shared_ptr<CANopenSlaveDriver>>((size_t)1);
		json_object_object_get_ex(slavesJ, "nodId", &slaveNodId);
		m_slaves[0] = std::make_shared<CANopenSlaveDriver>(m_exec, *m_master, api, slavesJ, json_object_get_int(slaveNodId));
	}

	// start the master
	m_master->Reset();

	// Add lely fd to AFB demon event loop
	struct sd_event_source *event_source = nullptr;
	auto userdata = const_cast<void *>(static_cast<const void *>(static_cast<ev_poll_t *>(m_poll.get_poll())));
	err = sd_event_add_io(afb_daemon_get_event_loop(), &event_source, m_poll.get_fd(), EPOLLIN, ServiceCANopenMasterHandler, userdata);
	if (err == 0)
		m_isRunning = true;
}

json_object *CANopenMaster::infoJ()
{
	json_object *master_info, *responseJ = json_object_new_object();
	char *formatedInfo;
	char isRunningS[7];
	if (m_isRunning)
		strcpy(isRunningS, "true");
	else
		strcpy(isRunningS, "false");
	asprintf(&formatedInfo, "uri: '%s', nodId: %d, isRunning: %s, info: '%s', object dictionary: %s", m_uri, m_nodId, isRunningS, m_info, m_dcf);

	json_object_object_add(responseJ, "Master_info", json_object_new_string(formatedInfo));
	json_object *slavesJ = json_object_new_array();
	for (auto slave : m_slaves)
	{
		json_object_array_add(slavesJ, slave->infoJ());
	}
	json_object_object_add(responseJ, "Slaves", slavesJ);

	wrap_json_pack(&master_info, "{ss ss so}",
		       "uid", m_uid,
		       "info", formatedInfo,
		       "verbs", slavesJ);
	return master_info;
}

json_object *CANopenMaster::statusJ()
{
	json_object *master_status;
	int err = wrap_json_pack(&master_status, "{ss si sb ss}",
				 "uri", m_uri,
				 "nodId", m_nodId,
				 "isRunning", m_isRunning,
				 "ObjectDictionary", m_dcf);
	if (err)
		master_status = json_object_new_string("Master Status ERROR");
	return master_status;
}

json_object *CANopenMaster::slaveListInfo(json_object *array)
{
	//json_object * slave_list_info;
	for (auto slave : m_slaves)
	{
		json_object_array_add(array, slave->infoJ());
		//json_object_object_add(slave_list_info, NULL, slave->infoJ());
		//slave_list_info = wrap_json_object_add(slave_list_info, slave->infoJ());
	}
	return array;
}

// void CANopenMaster::reset(){
//     m_master->Reset();
// }
