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


#include "CANopenExec.hpp"
#include "CANopenMaster.hpp"
#include "CANopenEncoder.hpp"

#include <iostream>

#include <rp-utils/rp-jsonc.h>
#include <rp-utils/rp-path-search.h>


class coConfig;

// main entry is called right after binding is loaded with dlopen
extern "C" int afbBindingEntry(afb_api_t rootapi, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *api_data);

class coConfig
{
	/** the API */
	afb_api_t rootapi_;

	/** the API */
	afb_api_t api_;

	/** meta data from controller */
	ctl_metadata_t metadata_;

	/** plugins from controller */
	plugin_store_t plugins_ = PLUGIN_STORE_INITIAL;

	/** on-start controller actions */
	ctl_actionset_t onstart_ = CTL_ACTIONSET_INITIALIZER;

	/** on-events controller actions */
	ctl_actionset_t onevent_ = CTL_ACTIONSET_INITIALIZER;

	/** holder for the configuration */
	json_object *config_;

	/** Executor */
	CANopenExec exec_;

	/** masters canopen buses */
	CANopenMasterSet masters_;

	/** path search */
	rp_path_search_t *paths_ = nullptr;

	/// constructor for initializing instance with known values
	coConfig(afb_api_t rootapi, json_object *config)
		: rootapi_{rootapi}
		, api_{rootapi}
		, config_{json_object_get(config)}
		, exec_{rootapi}
		, masters_{exec_}
		{}

	/// destructor
	~coConfig() {
		ctl_actionset_free(&onstart_);
		ctl_actionset_free(&onevent_);
		plugin_store_drop_all(&plugins_);
		json_object_put(config_);
	}

	/// initialization
	int init()
	{
		int rc, status = 0;

		// init instance for searching files
		rc = rp_path_search_make_dirs(&paths_, "${CANOPENPATH}:${AFB_ROOTDIR}/etc:${AFB_ROOTDIR}/plugins:.");
		if (rc < 0) {
			AFB_API_ERROR(rootapi_, "failed to initialize path search");
			status = rc;
		}

		// read controller sections configs (canopen section is read from
		// apicontrolcb after api creation)
		rc = ctl_subread_metadata(&metadata_, config_, false);
		if (rc < 0) {
			AFB_API_ERROR(rootapi_, "failed to read metadata section");
			status = rc;
		}

		// read plugins
		rc = ctl_subread_plugins(&plugins_, config_, paths_, "plugins");
		if (rc < 0) {
			AFB_API_ERROR(rootapi_, "failed to read plugins section");
			status = rc;
		}

		// initialize the encoders and decoders of plugins
		rc = plugin_store_iter(plugins_, _init_plugin_codecs_, reinterpret_cast<void*>(this));
		if (rc < 0) {
			AFB_API_ERROR(rootapi_, "failed to record plugins codecs");
			status = rc;
		}

		// read onstart section
		rc = ctl_subread_actionset(&onstart_, config_, "onstart");
		if (rc < 0) {
			AFB_API_ERROR(rootapi_, "failed to read onstart section");
			status = rc;
		}

		// read events section
		rc = ctl_subread_actionset(&onevent_, config_, "events");
		if (rc < 0) {
			AFB_API_ERROR(rootapi_, "failed to read events section");
			status = rc;
		}
masters_.dump(std::cerr);
		// creates the api
		if (status == 0) {
			rc = afb_create_api(&api_, metadata_.api, metadata_.info, 1, _control_, reinterpret_cast<void*>(this));
			if (rc < 0) {
				AFB_API_ERROR(rootapi_, "creation of api %s failed", metadata_.api);
				status = rc;
			}
		}
masters_.dump(std::cerr);
		// lock json config in ram
		return status;
	}

	// declares the encoders and decoders of one plugin
	static int _init_plugin_codecs_(void *closure, const plugin_t *plugin)
	{
		// get pointer to the function declaring codecs
		void *ptr = plugin_get_object(plugin, "canopenDeclareCodecs");
		if (ptr == NULL)
			return 0; // none exists, not an error, plugins can serve numerous purposes

		// cast pointers to their real expected type
		int(*dclfun)(afb_api_t,CANopenEncoder*) = reinterpret_cast<int(*)(afb_api_t,CANopenEncoder*)>(ptr);
		coConfig *config = reinterpret_cast<coConfig*>(closure);

		// invoke the declaring function
		int rc = dclfun(config->api_, &CANopenEncoder::instance());
		if (rc < 0)
			AFB_API_ERROR(config->api_, "Declaration of codec failed for plugin %s", plugin_name(plugin));
		return rc;
	}

	// tiny wrapper to enter instance method
	static int _control_(afb_api_t api, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *closure)
	{
		return reinterpret_cast<coConfig*>(closure)->control(api, ctlid, ctlarg);
	}

	// main binding control
	int control(afb_api_t api, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg)
	{
		json_object *cfg;
		int status = 0;
		unsigned idx, cnt;

		switch (ctlid) {
		case afb_ctlid_Root_Entry:
			// should be never happen
			AFB_API_ERROR(rootapi_, "canopen root_entry call after api creation");
			return AFB_ERRNO_NOT_AVAILABLE;

		// let process canopen config as part of binding config
		case afb_ctlid_Pre_Init:
			exec_.set(api_ = api);

			// explain dependecies of API as required by metadata
			status = ctl_set_requires(&metadata_, api);
			if (status < 0) {
				AFB_API_ERROR(api, "canopen mandatory api dependencies not satisfied");
				return status;
			}

			// add static controls verbs
			cnt = sizeof common_verbs / sizeof *common_verbs;
			for (idx = 0 ; idx < cnt ; idx++) {
				status = afb_api_add_verb(api,
							common_verbs[idx].name, common_verbs[idx].info, common_verbs[idx].callback,
							reinterpret_cast<void*>(this), 0, 0, 0);
				if (status < 0) {
					AFB_API_ERROR(api, "Registering static verb %s failed", common_verbs[idx].name);
					return status;
				}
			}

			// add the event handlers
			status = ctl_actionset_add_events(&onevent_, api, plugins_, reinterpret_cast<void*>(this));
			if (status < 0) {
				AFB_API_ERROR(api, "Registering event handlers failed");
				return status;
			}

			// Get the canopen config
			if (!json_object_object_get_ex(config_, "canopen", &cfg)) {
				AFB_API_ERROR(api, "No 'canopen' entry in configuration");
				return AFB_ERRNO_GENERIC_FAILURE;
			}

			// create the masters
			status = rp_jsonc_optarray_until(cfg, _add_master_, reinterpret_cast<void*>(this));
			if (status < 0)
				return status;

			// start
			status = exec_.start();
			status = masters_.start();
			if (status < 0)
				return status;

			// success of pre-initialization
			break;

		/* called for init */
		case afb_ctlid_Init:

			// execute on start actions
			status = ctl_actionset_exec(&onstart_, api, plugins_, reinterpret_cast<void*>(this));
			if (status < 0) {
				AFB_API_ERROR(api, "canopen fail register sensors actions");
				return status;
			}
			break;

		/* called when required classes are ready */
		case afb_ctlid_Class_Ready:
			break;

		/* called when an event is not handled */
		case afb_ctlid_Orphan_Event:
			AFB_API_NOTICE(api, "canopen received unexpected event:%s", ctlarg->orphan_event.name);
			break;

		/** called when shuting down */
		case afb_ctlid_Exiting:
			break;
		}
		return 0;
	}

	// tiny wrapper to configuring masters
	static int _add_master_(void *closure, json_object *cfg)
	{
		return reinterpret_cast<coConfig*>(closure)->add_master(cfg);
	}

	// main binding control
	int add_master(json_object *cfg)
	{
		return masters_.add(cfg, paths_);
	}

	// implementation of ping
	static void _ping_(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		afb_data_array_addref(nparams, params);
		afb_req_reply(request, 0, nparams, params);
	}

	// tiny wrapper to enter instance method
	static void _info_(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		coConfig *current = reinterpret_cast<coConfig*>(afb_req_get_vcbdata(request));
		current->info(request, nparams, params);
	}

	// implement info
	void info(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		int err;
		unsigned idx, cnt;
		json_object *response = NULL, *global_info = NULL, *admin_info = NULL,
					*static_verbs_info = NULL, *verb_info = NULL,
					*groups = NULL;

		err = rp_jsonc_pack(&global_info, "{ss ss* ss* ss* sO}",
					"uid", metadata_.uid,
					"info", metadata_.info,
					"version", metadata_.version,
					"author", metadata_.author,
					"status", masters_.statusJ());
		if (err)
			global_info = json_object_new_string("global info ERROR !");

		static_verbs_info = json_object_new_array();
		cnt = sizeof common_verbs / sizeof *common_verbs;
		for (idx = 0; idx < cnt ; idx++)
		{
			err = rp_jsonc_pack(&verb_info, "{ss ss* ss*}",
						"uid", common_verbs[idx].name,
						"info", common_verbs[idx].info,
						"author", "IoT.bzh");
			if (err)
				verb_info = json_object_new_string("static verb info ERROR !");
			json_object_array_add(static_verbs_info, verb_info);
		}

		err = rp_jsonc_pack(&admin_info, "{ss ss sO}",
					"uid", "admin",
					"info", "verbs related to administration of this binding",
					"verbs", static_verbs_info);
		if (err)
			admin_info = json_object_new_string("admin info ERROR !");

		groups = json_object_new_array();
		json_object_array_add(groups, admin_info);
		masters_.slaveListInfo(groups);

		err = rp_jsonc_pack(&response, "{so so}",
					"metadata", global_info,
					"groups", groups);
		if (err)
			err = AFB_ERRNO_INTERNAL_ERROR;
		afb_req_reply_json_c_hold(request, err, response);
	}

	// Structure for describing static verbs
	struct sverbdsc
		{
			const char *name;
			const char *info;
			void (*callback)(afb_req_t, unsigned, afb_data_t const[]);
		};

	// Declare array of static verb not depending on CANopen json config file
	static const sverbdsc common_verbs[2];

	// the entry point
	friend int afbBindingEntry(afb_api_t rootapi, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *closure);
};

// Declare array of static verb not depending on CANopen json config file
const coConfig::sverbdsc coConfig::common_verbs[2] = {
	{ .name = "ping", .info = "CANopen API ping test", .callback = coConfig::_ping_ },
	{ .name = "info", .info = "display info about the binding", .callback = coConfig::_info_ },
};

coConfig *last_global_coconfig;

// main entry is called right after binding is loaded with dlopen
extern "C"
int afbBindingEntry(afb_api_t rootapi, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *closure)
{
	// this root entry is only called once for the main initialization
	if (ctlid != afb_ctlid_Root_Entry) {
		AFB_API_ERROR(rootapi, "Unexpected control API call %d", (int)ctlid);
		return -1;
	}

	// instanciate
	coConfig *config = new coConfig(rootapi, ctlarg->root_entry.config);
	if (config == nullptr) {
		AFB_API_ERROR(rootapi, "Out of memory");
		return -1;
	}

last_global_coconfig=config;

	// initialize now
	int rc = config->init();
	if (rc < 0)
		delete config;
	return rc;
}
