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
#include "utils/jsonc.hpp"

struct master_config
{
	const char *uid = NULL;
	const char *info = NULL;
	const char *uri = NULL;
	const char *dcf = NULL;
	json_object *slaves = NULL;
	uint16_t nodId = 255;
};

static bool read_config(afb_api_t api, json_object *obj, master_config &config)
{
	int intval;
	bool ok = true;

	if (!get(api, obj, "uid", config.uid))
		ok = false;

	if (!get(api, obj, "uri", config.uri))
		ok = false;

	if (!get(api, obj, "dcf", config.dcf))
		ok = false;

	if (!get(api, obj, "nodId", intval))
		ok = false;
	else {
		if (intval >= 1 && intval <= 254)
			config.nodId = (uint8_t)intval;
		else {
			AFB_API_ERROR(api, "invalid nodId %d in configuration object %s",
				intval, json_object_to_json_string(obj));
			ok = false;
		}
	}

	if (!get(api, obj, "slaves", config.slaves, true, json_type_array))
		ok = false;

	if (!get(api, obj, "info", config.info, false))
		ok = false;

	return ok;
}

CANopenMaster::CANopenMaster(CANopenExec &exec)
	: m_exec(exec)
{}

CANopenMaster::~CANopenMaster()
{
}

int CANopenMaster::init(json_object *rtuJ, rp_path_search_t *paths)
{
	char *dcf;
	int sid;
	unsigned idx, count;
	json_object *slaveNodId, *slaveJ;

	master_config config;

	// get the config
	if (!read_config(m_exec, rtuJ, config))
		return -1;

	// locate the master DCF file
	dcf = findFile(config.dcf, paths);
	if (!dcf) {
		AFB_API_ERROR(m_exec, "Could not find config file \"%s\"", config.dcf);
		return -1;
	}
	AFB_API_NOTICE(m_exec, "found DCF file at %s", dcf);

	m_uid = config.uid;
	m_nodId = config.nodId;
	m_info = config.info;
	m_can = m_exec.open(config.uri, dcf, config.nodId);
	free(dcf);

	// loop on slaves
	count = (unsigned)json_object_array_length(config.slaves);
	for (idx = 0 ; idx < count ; idx++) {
		try
		{
			slaveJ = json_object_array_get_idx(config.slaves, idx);
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
				m_can->uri(), m_nodId, isRunning() ? "true" : "false", m_info, m_can->dcf());

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
				 "nodId", (int)m_nodId,
				 "isRunning", isRunning(),
				 "ObjectDictionary", m_can->dcf());
	if (err)
		master_status = json_object_new_string("Master Status ERROR");
	return master_status;
}

json_object *CANopenMaster::slaveListInfo(json_object *array)
{
	for (auto it : m_slaves)
		json_object_array_add(array, it.second->infoJ());
	return array;
}

void CANopenMaster::dump(std::ostream &os) const
{
	const char *i = "";
	os << i << "--- master ---" << std::endl;
	os << i << "id " << m_uid << std::endl;
	os << i << "nodId " << m_nodId << std::endl;
	os << i << "run? " << (m_isRunning ? "yes" : "no") << std::endl;
	os << i << m_info << std::endl;
	m_can->dump(os);
	for (auto it : m_slaves) {
		it.second->dump(os);
		os << std::endl;
	}
}

