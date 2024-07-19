/*
 Copyright (C) 2015-2024 IoT.bzh Company

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

#ifndef _ServiceCANopenMasterSet_INCLUDE_
#define _ServiceCANopenMasterSet_INCLUDE_

#include <memory>
#include <functional>
#include <vector>

#include "CANopenMaster.hpp"
#include "utils/cstrmap.hpp"

/**
 * The class CANopenMaster holds a CANopen bus connection
 */
class CANopenMasterSet
{
public:
	CANopenMasterSet(CANopenExec &exec) : exec_{exec}, masters_{}, imasters_{4} {}
	int add(json_object *cfg, rp_path_search_t *paths);
	int start();
	json_object *statusJ();
	void slaveListInfo(json_object *groups);
	void dump(std::ostream &os) const;
	void foreach(const std::function<void(const char*,CANopenMaster&)> &fun);
	CANopenMaster *operator [] (unsigned index) const;

private:
	/// the single execution handler
	CANopenExec &exec_;

	/** masters canopen buses */
	cstrmap<std::shared_ptr<CANopenMaster>> masters_;

	/** masters canopen buses */
	std::vector<std::shared_ptr<CANopenMaster>> imasters_;
};

#endif /* _ServiceCANopenMasterSet_INCLUDE_ */
