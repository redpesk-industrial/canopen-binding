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

#include "CANopenMaster.hpp"

#include <string.h>

#include <rp-utils/rp-jsonc.h>
#include <rp-utils/rp-path-search.h>

#include "CANopenSlaveDriver.hpp"

#include "utils/utils.hpp"

CANopenMaster::CANopenMaster(CANopenExec &exec)
	: m_exec(exec)
{}

CANopenMaster::~CANopenMaster()
{
}

int CANopenMaster::init(json_object *rtuJ, rp_path_search_t *paths)
{
	int err = 0;
	json_object *slavesJ = NULL;

	assert(rtuJ);
	const char *dcfFile, *uri;
	char *dcf;
	int nodId;

	err = rp_jsonc_unpack(rtuJ, "{ss,s?s,ss,s?s,s?i,so !}",
			       "uid", &m_uid,
			       "info", &m_info,
			       "uri", &uri,
			       "dcf", &dcfFile,
			       "nodId", &nodId,
			       "slaves", &slavesJ);
	if (err)
	{
		AFB_API_ERROR(m_exec, "Fail to parse rtu JSON : (%s)", json_object_to_json_string(rtuJ));
		return -1;
	}

	// locate the master DCF file
	dcf = findFile(dcfFile, paths);
	if (!dcf) {
		AFB_API_ERROR(m_exec, "Could not find config file \"%s\"", dcfFile);
		return -1;
	}
	AFB_API_NOTICE(m_exec, "found DCF file at %s", dcf);

	m_can = m_exec.open(uri, dcf, nodId);
	free(dcf);

	// loop on slaves
	int sid;
	unsigned idx = 0, count;
	json_object *slaveNodId, *slaveJ;
	if (json_object_is_type(slavesJ, json_type_array))
	{
		count = (unsigned)json_object_array_length(slavesJ);
		if (count == 0)
		{
			AFB_API_ERROR(m_exec, "empty slaves array uid=%s uri=%s", m_uid, uri);
			return -1;
		}
		slaveJ = json_object_array_get_idx(slavesJ, 0);
	}
	else
	{
		count = 1;
		slaveJ = slavesJ;
	}
	for (;;)
	{
		try
		{
			AFB_API_DEBUG(m_exec, "creation of slave %s", json_object_to_json_string(slaveJ));
			if (!json_object_object_get_ex(slaveJ, "nodId", &slaveNodId))
				throw std::invalid_argument(std::string("id of slave is missing in ") + json_object_to_json_string(slaveJ));
			if (!json_object_is_type(slaveNodId, json_type_int))
				throw std::invalid_argument(std::string("slave id must be an integer but is ") + json_object_to_json_string(slaveNodId));
			sid = json_object_get_int(slaveNodId);
			if (sid <= 0 || sid > 127)
				throw std::out_of_range(std::string("slave id must be in 1 ... 127 but is ") + std::to_string(sid));
			auto slave = std::make_shared<CANopenSlaveDriver>(*this, slaveJ, sid);
			m_slaves[slave->uid()] = slave;
		}
		catch (std::exception &e)
		{
			AFB_API_ERROR(m_exec, "creation of slave failed %s", e.what());
			return -1;
		}
		if (++idx >= count)
			break;
		slaveJ = json_object_array_get_idx(slavesJ, idx);
	}
	return 0;
}

int CANopenMaster::start()
{
	if (m_can) {
		m_can->start();
		m_isRunning = true;
	}
	return 0;
}

json_object *CANopenMaster::infoJ()
{
	json_object *master_info, *responseJ = json_object_new_object();
	char *formatedInfo;

	asprintf(&formatedInfo, "uri: '%s', nodId: %d, isRunning: %s, info: '%s', object dictionary: %s",
				m_can->uri(), m_can->nodId(), isRunning() ? "true" : "false", m_info, m_can->dcf());

	json_object_object_add(responseJ, "Master_info", json_object_new_string(formatedInfo));
	json_object *slavesJ = json_object_new_array();
	for (auto slave : m_slaves)
	{
		json_object_array_add(slavesJ, slave.second->infoJ());
	}
	json_object_object_add(responseJ, "Slaves", slavesJ);

	rp_jsonc_pack(&master_info, "{ss ss so}",
		       "uid", m_uid,
		       "info", formatedInfo,
		       "verbs", slavesJ);
	return master_info;
}

json_object *CANopenMaster::statusJ()
{
	json_object *master_status;
	int err = rp_jsonc_pack(&master_status, "{ss si sb ss}",
				 "uri", m_can->uri(),
				 "nodId", (int)m_can->nodId(),
				 "isRunning", isRunning(),
				 "ObjectDictionary", m_can->dcf());
	if (err)
		master_status = json_object_new_string("Master Status ERROR");
	return master_status;
}

json_object *CANopenMaster::slaveListInfo(json_object *array)
{
	for (auto it : m_slaves)
	{
		json_object_array_add(array, it.second->infoJ());
	}
	return array;
}

void CANopenMaster::dump(std::ostream &os) const
{
	const char *i = "";
	os << i << "--- master ---" << std::endl;
	os << i << "id " << uid() << std::endl;
	os << i << "run? " << (m_isRunning ? "yes" : "no") << std::endl;
	os << i << m_info << std::endl;
	m_can->dump(os);
	for (auto it : m_slaves) {
		it.second->dump(os);
		os << std::endl;
	}
}

int CANopenMasterSet::add(json_object *cfg, rp_path_search_t *paths)
{
	int status;

	// Get the master canopen config, should be an object
	if (!json_object_is_type(cfg, json_type_object)) {
		AFB_API_ERROR(exec_, "Wrong CANopen descriptor");
		return AFB_ERRNO_BAD_STATE;
	}

	// Load CANopen network configuration and start
	CANopenMaster *master = new CANopenMaster(exec_);
	if (master == nullptr) {
		AFB_API_ERROR(exec_, "Out of memory");
		return AFB_ERRNO_OUT_OF_MEMORY;
	}

	// init and start
	status = master->init(cfg, paths);
	if (status < 0) {
		AFB_API_ERROR(exec_, "Initialization failed");
		return AFB_ERRNO_GENERIC_FAILURE;
	}

	// record
	masters_[master->uid()] = std::shared_ptr<CANopenMaster>(master);
	return 0;
}

int CANopenMasterSet::start()
{
	for (auto master : masters_) {
		int status = master.second->start();
		if (status < 0) {
			AFB_API_ERROR(exec_, "Start error");
			return AFB_ERRNO_GENERIC_FAILURE;
		}

		// check it runs
		if (!master.second->isRunning()) {
			AFB_API_ERROR(exec_, "initialization failed");
			return AFB_ERRNO_GENERIC_FAILURE;
		}
	}
	return 0;
}

json_object *CANopenMasterSet::statusJ()
{
	json_object *status = nullptr;
	if (masters_.size() == 1)
		status = (*(masters_.begin())).second->statusJ();
	else
	{
		status = json_object_new_array();
		for(auto master : masters_)
			json_object_array_add(status, master.second->statusJ());
	}
	return status;
}

void CANopenMasterSet::slaveListInfo(json_object *groups)
{
	for(auto master : masters_)
		master.second->slaveListInfo(groups);
}

void CANopenMasterSet::dump(std::ostream &os) const
{
	for(auto master : masters_)
		master.second->dump(os);
}
