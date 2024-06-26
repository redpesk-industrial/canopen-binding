#define AFB_BINDING_VERSION 4

#include <afb-helpers4/ctl-lib-plugin.h>

#include <rp-utils/rp-jsonc.h>

#include "CANopenSensor.hpp"
#include "CANopenGlue.hpp"
#include "CANopenEncoder.hpp"

/* Identify itself */
CTL_PLUGIN_DECLARE("king_pigeon", "CANOPEN plugin for king pigeon");

// set up a new bool array based on a json bool array
COdataType newBoolArrayfromArray(json_object *dataJ)
{
	uint32_t value = 0;
	unsigned i, n = (unsigned)json_object_array_length(dataJ);
	for (i = 0; i < n; i++)
		if (json_object_get_boolean(json_object_array_get_idx(dataJ, i)))
			value |= ((uint32_t)1) << i;
	return COdataType(value);
}

// set up a new bool array based on a json int
COdataType newBoolArrayfromInt(json_object *dataJ)
{
	return COdataType(json_object_get_int(dataJ));
}

// Modify a bool array using a bool value and an integer mask
COdataType setBoolArrayFromIntMask(COdataType data, bool val, int mask)
{
	uint32_t x = data;
	if (val)
		x |= mask;
	else
		x &= 0xFF ^ mask;
	return COdataType(x);
}

// Modify a bool array using a bool value and a json bool array mask
COdataType setBoolArrayFromArrayMask(COdataType data, bool val, json_object *maskJ)
{
	uint32_t x = data;
	unsigned i, n = (unsigned)json_object_array_length(maskJ);
	if (val)
	{
		for (i = 0; i < n; i++)
			if (json_object_get_boolean(json_object_array_get_idx(maskJ, i)))
				x |= 1 << i;
	}
	else
	{
		for (i = 0; i < n; i++)
			if (json_object_get_boolean(json_object_array_get_idx(maskJ, i)))
				x &= (0b11111111 ^ (1 << i));
	}
	return COdataType(x);
}

// set upe a json bool 4 entree array based on an integer corresponding to 4 Kingpigeon DIN
static json_object *kingpigeon_4_bool_array_decode(COdataType data, CANopenSensor *sensor) noexcept
{
	uint32_t x = data;
	json_object *outputJ = json_object_new_array();
	json_object_array_add(outputJ, json_object_new_boolean(json_bool(x & 0b00000001)));
	json_object_array_add(outputJ, json_object_new_boolean(json_bool(x & 0b00000010)));
	json_object_array_add(outputJ, json_object_new_boolean(json_bool(x & 0b00000100)));
	json_object_array_add(outputJ, json_object_new_boolean(json_bool(x & 0b00001000)));
	return outputJ;
}

// set up a json int 2 entree array based on a 32bits unsigned integer corresponding to 2 Kingpigeon 16bits AIN
static json_object *kingpigeon_2_int_array_decode(COdataType data, CANopenSensor *sensor) noexcept
{
	uint32_t x = data;
	json_object *outputJ = json_object_new_array();
	json_object_array_add(outputJ, json_object_new_int(x & 0x0000FFFF));
	json_object_array_add(outputJ, json_object_new_int((x & 0xFFFF0000) >> 16));
	return outputJ;
}

// set up an integer bool array based on a json object
static COdataType kingpigeon_bool_array_encode(json_object *dataJ, CANopenSensor *sensor) noexcept
{

	// set output based on a bool array on an integer regardless of it's previous value
	if (json_object_is_type(dataJ, json_type_array))
		return newBoolArrayfromArray(dataJ);
	if (json_object_is_type(dataJ, json_type_int))
		return newBoolArrayfromInt(dataJ);

	json_object *val = nullptr;
	json_object *mask = nullptr;
	COdataType data;
	data = sensor->currentVal();

	int err = rp_jsonc_unpack(dataJ, "{so s?o!}",
					"val", &val,
					"mask", &mask);

	if (err)
		return data;

	// if no mask is specify set output based on a bool array on an integer regardless of it's previous value
	if (!mask)
	{
		switch (json_object_get_type(val))
		{
		case json_type_int:
			return newBoolArrayfromInt(val);
		case json_type_array:
			return newBoolArrayfromArray(val);
		default:
			return data;
		}
	}

	// set output to specified value only at specified bits pointed by the (bool array or int) mask
	if (json_object_is_type(val, json_type_boolean))
	{
		switch (json_object_get_type(mask))
		{
		case json_type_int:
			return setBoolArrayFromIntMask(data, (bool)json_object_get_boolean(val), json_object_get_int(mask));
		case json_type_array:
			return setBoolArrayFromArrayMask(data, (bool)json_object_get_boolean(val), mask);
		default:
			return data;
		}
	}
	// if something goes wrong keep output as it was before
	return data;
}

std::vector<std::pair<std::string, coDecodeCB>> kingpigeonDecodeFormatersTable{
	//uid              decoding CB
	{"kp_4-boolArray", kingpigeon_4_bool_array_decode},
	{"kp_2-intArray", kingpigeon_2_int_array_decode}
};

extern "C"
int canopenDeclareCodecs(afb_api_t api, CANopenEncoder *coEncoder)
{
	// add a all list of decode formaters
	int err = coEncoder->addDecodeFormater(kingpigeonDecodeFormatersTable);
	if (err)
		AFB_API_WARNING(api, "Kingpigeon-plugin ERROR : fail to add %d entree to decode formater table", err);
	// add a single encoder
	err = coEncoder->addEncodeFormater("kp_4-boolArray", kingpigeon_bool_array_encode);
	if (err)
		AFB_API_WARNING(api, "Kingpigeon-plugin ERROR : fail to add 'kp_4-boolArray' entree to encode formater table");

	return 0;
}
