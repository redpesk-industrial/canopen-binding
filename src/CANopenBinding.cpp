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


#include "CANopenExec.hpp"
#include "CANopenMasterSet.hpp"
#include "CANopenMaster.hpp"
#include "CANopenSlaveDriver.hpp"
#include "CANopenSensor.hpp"
#include "CANopenEncoder.hpp"
#include "xchg/CANopenXchg.h"
#include "utils/jsonc.hpp"

#include <iostream>
#include <regex>
#include <system_error>

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
		{ memset(&metadata_,0,sizeof metadata_); }

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
		rc = canopen_xchg_init();
		if (rc < 0) {
			AFB_API_ERROR(rootapi_, "failed to initialize canopen types");
			status = rc;
		}

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

//masters_.dump(std::cerr);
		// creates the api
		if (status == 0) {
			rc = afb_create_api(&api_, metadata_.api, metadata_.info, 1, _control_, reinterpret_cast<void*>(this));
			if (rc < 0) {
				AFB_API_ERROR(rootapi_, "creation of api %s failed", metadata_.api);
				status = rc;
			}
		}
//masters_.dump(std::cerr);
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

	// tiny wrapper to enter instance method
	static void _status_(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		coConfig *current = reinterpret_cast<coConfig*>(afb_req_get_vcbdata(request));
		current->status(request, nparams, params);
	}

	// implement status
	void status(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		json_object *status = masters_.statusJ();
		if (status != NULL)
			afb_req_reply_json_c_hold(request, 0, status);
		else
			afb_req_reply(request, AFB_ERRNO_INTERNAL_ERROR, 0, NULL);
	}

	// tiny wrapper to enter instance method
	static void _subscribe_(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		coConfig *current = reinterpret_cast<coConfig*>(afb_req_get_vcbdata(request));
		current->subunsub(request, nparams, params, true);
	}

	// tiny wrapper to enter instance method
	static void _unsubscribe_(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		coConfig *current = reinterpret_cast<coConfig*>(afb_req_get_vcbdata(request));
		current->subunsub(request, nparams, params, false);
	}

	// implement subscribe or unsubscribe
	void subunsub(afb_req_t request, unsigned nparams, afb_data_t const params[], bool sub)
	{
		afb_data_t data;
		json_object *args, *item;
		const char *pattern;
		size_t count, nidx;
		int size;
		int rc, nrc;
		int (CANopenSensor::*func)(afb_req_t) = sub ? &CANopenSensor::subscribe : &CANopenSensor::unsubscribe;

		if (nparams == 0) {
			count = 1;
			item = NULL;
		}
		else {
			// get the JSON object
			rc = afb_req_param_convert(request, 0, AFB_PREDEFINED_TYPE_JSON_C, &data);
			if (rc < 0) {
				REQFAIL(request, AFB_ERRNO_INVALID_REQUEST, "conversion to JSON failed %d", rc);
				return;
			}

			args = reinterpret_cast<json_object*>(afb_data_ro_pointer(data));
			if(json_object_is_type(args, json_type_array)) {
				count = json_object_array_length(args);
				item = count ? json_object_array_get_idx(args, 0) : NULL;
			}
			else {
				count = 1;
				item = args;
			}
		}

		rc = 0;
		for (nidx = 1 ; ; nidx++) {
			nrc = 0;
			if (item == NULL && nidx == 1)
				pattern = ".*";
			else if (json_object_is_type(item, json_type_string))
				pattern = json_object_get_string(item);
			else {
				AFB_REQ_ERROR(request, "pattern isn't a string %s", json_object_to_json_string(item));
				nrc = AFB_ERRNO_INVALID_REQUEST;
			}
			if (nrc == 0) {
				try {
					struct {
						std::basic_regex<char> re;
						afb_req_t request;
						int (CANopenSensor::*func)(afb_req_t);
						int rc;
					} data = {
						std::basic_regex<char>(pattern),
						request,
						func,
						0
					};
					masters_.foreach([&data](const char *masterid, CANopenMaster &master){
						master.foreach([&data](const char *slaveid, CANopenSlaveDriver &slave){
							slave.foreach([&data](const char *sensorid, CANopenSensor &sensor){
								if (std::regex_match(sensorid, data.re)) {
									int rc = (sensor.*data.func)(data.request);
									if (rc < 0) {
										AFB_REQ_ERROR(data.request, "sub/unsub error for %s", sensorid);
										data.rc = rc;
									}
								}
							});
						});
					});
					nrc = data.rc;
				}
				catch(...) {
					AFB_REQ_ERROR(request, "Bad pattern %s", pattern);
					nrc = AFB_ERRNO_INVALID_REQUEST;
				}
			}
			if (nrc < 0)
				rc = nrc;
			if (nidx == count)
				break;
			item = json_object_array_get_idx(args, nidx);
		}
		afb_req_reply(request, rc < 0 ? AFB_ERRNO_GENERIC_FAILURE : 0, 0, NULL);
	}

	// tiny wrapper to enter instance method
	static void _get_(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		coConfig *current = reinterpret_cast<coConfig*>(afb_req_get_vcbdata(request));
		current->get(request, nparams, params);
	}

	// get of set of values
	void get(afb_req_t request, unsigned nparams, afb_data_t const params[])
	{
		int rc;
		unsigned idx, count;
		size_t size;
		canopen_xchg_v1_req_t *arr_req;
		canopen_xchg_v1_value_t *arr_val;
		afb_data_t dreq, dval;

		// get request array
		if (nparams < 1) {
			AFB_REQ_ERROR(request, "missing parameter");
			goto inval;
		}
		rc = afb_req_param_convert(request, 0, canopen_xchg_v1_req_type, &dreq);
		if (rc < 0) {
			AFB_REQ_ERROR(request, "invalid first parameter type");
			goto inval;
		}
		rc = afb_data_get_constant(dreq, (void**)&arr_req, &size);
		if (rc < 0 || arr_req == NULL || size == 0) {
			AFB_REQ_ERROR(request, "invalid first parameter value");
			goto inval;
		}
		count = size / sizeof *arr_req;

		// get the returned value array
		if (nparams == 1) {
			size = count * sizeof *arr_val;
			rc = afb_create_data_alloc(&dval, canopen_xchg_v1_value_type, (void**)&arr_val, size);
			if (rc < 0) {
				AFB_REQ_ERROR(request, "allocation of result failed");
				rc = AFB_ERRNO_OUT_OF_MEMORY;
				goto error;
			}
		}
		else {
			rc = afb_req_param_convert(request, 1, canopen_xchg_v1_value_type, &dval);
			if (rc < 0) {
				AFB_REQ_ERROR(request, "invalid second parameter type");
				goto inval;
			}
			rc = afb_data_get_mutable(dval, (void**)&arr_val, &size);
			if (rc < 0 || arr_val == NULL || size != count * sizeof *arr_val) {
				AFB_REQ_ERROR(request, "invalid second parameter value");
				goto inval;
			}
			afb_data_addref(dval);
		}

		// makes the result
		try {
			canopen_xchg_v1_req_t *req = arr_req;
			canopen_xchg_v1_value_t *val = arr_val;
			for (idx = 0 ; idx < count ; idx++, req++, val++) {
				val->u64 = 0;
				CANopenMaster *master = masters_[req->itf];
				if (master != nullptr) {
					std::error_code ec;
					using ConstSubObject = lely::canopen::BasicMaster::ConstSubObject;
					ConstSubObject csobj =
						req->tpdo ? master->tpdo(req->id, req->reg, req->subreg)
							: master->rpdo(req->id, req->reg, req->subreg);
					switch(req->type) {
					case canopen_xchg_u8:  val->u8  = csobj.Read<  int8_t>(ec); break;
					case canopen_xchg_i8:  val->i8  = csobj.Read< uint8_t>(ec); break;
					case canopen_xchg_u16: val->u16 = csobj.Read< int16_t>(ec); break;
					case canopen_xchg_i16: val->i16 = csobj.Read<uint16_t>(ec); break;
					case canopen_xchg_u32: val->u32 = csobj.Read< int32_t>(ec); break;
					case canopen_xchg_i32: val->i32 = csobj.Read<uint32_t>(ec); break;
					case canopen_xchg_u64: val->u64 = csobj.Read< int64_t>(ec); break;
					case canopen_xchg_i64: val->i64 = csobj.Read<uint64_t>(ec); break;
					}
					if (ec)
						AFB_REQ_ERROR(request,
							"can't get (itf %d id %d reg %d.%d): %s",
							(int)req->itf, (int)req->id, (int)req->reg, (int)req->subreg,
							ec.message().c_str());
				}
				AFB_REQ_DEBUG(request, "idx%u getting itf %d id %d reg %d.%d %llx",
					idx, (int)req->itf, (int)req->id, (int)req->reg, (int)req->subreg, (long long)val->i64);
			}
			afb_data_notify_changed(dval);
			return afb_req_reply(request, 0, 1, &dval);
		}
		catch (std::exception &e) {
			AFB_REQ_ERROR(request, "at %u exception catched: %s", idx, e.what());
			afb_data_unref(dval);
			rc = AFB_ERRNO_INTERNAL_ERROR;
			goto error;
		}

	inval:	rc = AFB_ERRNO_INVALID_REQUEST;
	error:	return afb_req_reply(request, rc, 0, NULL);
	}

	// Structure for describing static verbs
	struct sverbdsc
		{
			const char *name;
			const char *info;
			void (*callback)(afb_req_t, unsigned, afb_data_t const[]);
		};

	// Declare array of static verb not depending on CANopen json config file
	static const sverbdsc common_verbs[6];

	// the entry point
	friend int afbBindingEntry(afb_api_t rootapi, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *closure);
};

// Declare array of static verb not depending on CANopen json config file
const coConfig::sverbdsc coConfig::common_verbs[6] = {
	{ .name = "ping", .info = "CANopen API ping test", .callback = coConfig::_ping_ },
	{ .name = "info", .info = "info about the binding", .callback = coConfig::_info_ },
	{ .name = "status", .info = "status of the binding", .callback = coConfig::_status_ },
	{ .name = "subscribe", .info = "subscribe to pattern event", .callback = coConfig::_subscribe_ },
	{ .name = "unsubscribe", .info = "unsubscribe to pattern event", .callback = coConfig::_unsubscribe_ },
	{ .name = "get", .info = "get a set of value", .callback = coConfig::_get_ },
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
