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

#include "CANopenMasterSet.hpp"



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

void CANopenMasterSet::foreach(const std::function<void(const char*,CANopenMaster&)> &fun)
{
	for(auto master : masters_)
		fun(master.first, *master.second);
}
