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

#include "CANopenEncoder.hpp"

#include "CANopenSlaveDriver.hpp"
#include "CANopenSensor.hpp"
#include "CANopenGlue.hpp"

CANopenEncoder::CANopenEncoder() {}

/// @brief Return singleton instance of configuration object.
CANopenEncoder &CANopenEncoder::instance()
{
	static CANopenEncoder encoder;
	return encoder;
}

const CANopenEncodeCbS &CANopenEncoder::getfunctionCB(std::string type, int size)
{
	return encodingTable.at(type).at(size);
}

coEncodeCB CANopenEncoder::getEncodeFormaterCB(std::string format)
{
	return coEncodeFormaterTable.at(format);
}

coDecodeCB CANopenEncoder::getDecodeFormaterCB(std::string format)
{
	return coDecodeFormaterTable.at(format);
}

// add an encoder formater to the list of availables encoders
int CANopenEncoder::addEncodeFormater(std::string uid, coEncodeCB encodeCB)
{
	auto [it, result] = coEncodeFormaterTable.emplace(std::make_pair(uid, encodeCB));
	if (!result)
	{
		throw(std::runtime_error(std::string("entree " + it->first + " could not be added because it alredy exist\n")));
		return -1;
	}
	return 0;
}

// add an antier list of encoder formater to the list of availables encoders
int CANopenEncoder::addEncodeFormater(const std::vector<std::pair<std::string, coEncodeCB>> &encodeFormaterTable)
{
	int err = 0;
	std::string errWhat;
	for (auto &newEncoder : encodeFormaterTable)
	{
		try
		{
			err += addEncodeFormater(newEncoder.first, newEncoder.second);
		}
		catch (std::runtime_error &e)
		{
			errWhat += e.what();
		}
	}
	if (err)
	{
		throw(std::runtime_error(errWhat));
		return err;
	}
	return 0;
}

// add an decoder formater to the list of availables decoders
int CANopenEncoder::addDecodeFormater(std::string uid, coDecodeCB decodeCB)
{
	auto [it, result] = coDecodeFormaterTable.emplace(std::make_pair(uid, decodeCB));
	AFB_DEBUG("Entry '%s' added to coDecodeFormaterTable => %d of %d",
			uid.c_str(), (int)result, (int)coDecodeFormaterTable.size());
	if (!result)
		return -1;
	return 0;
}

// add an antier list of decoder formater to the list of availables decoders
int CANopenEncoder::addDecodeFormater(const std::vector<std::pair<std::string, coDecodeCB>> &decodeFormaterTable)
{
	int err = 0;
	std::string errWhat;
	for (auto &newDecoder : decodeFormaterTable)
	{
		try
		{
			err += addDecodeFormater(newDecoder.first, newDecoder.second);
		}
		catch (std::runtime_error &e)
		{
			errWhat += e.what();
		}
	}
	if (err)
	{
		throw(std::runtime_error(errWhat));
		return err;
	}
	return 0;
}

COdataType CANopenEncoder::encodeInt(json_object *dataJ, CANopenSensor *sensor)
{
	return COdataType(get_data_int32(dataJ));
}

#if CODATADBL
COdataType CANopenEncoder::encodeDouble(json_object *dataJ, CANopenSensor *sensor)
{
	return COdataType(get_data_int64(dataJ));
}
#endif

COdataType CANopenEncoder::encodeString(json_object *dataJ, CANopenSensor *sensor)
{
	return COdataType(json_object_get_string(dataJ));
}

json_object *CANopenEncoder::decodeInt(COdataType data, CANopenSensor *sensor)
{
	return json_object_new_int(int32_t(data));
}

json_object *CANopenEncoder::decodeUint(COdataType data, CANopenSensor *sensor)
{
	return json_object_new_int64(int64_t(data));
}

#if CODATADBL
json_object *CANopenEncoder::decodeDouble(COdataType data, CANopenSensor *sensor)
{
	return json_object_new_double((double)data.tInt64);
}
#endif

json_object *CANopenEncoder::decodeString(COdataType data, CANopenSensor *sensor)
{
	return json_object_new_string((const char*)data);
}

void CANopenEncoder::coSDOwrite8bits(CANopenSensor *sensor, COdataType data)
{
	coSDOwriteAsync8bits(sensor, data);
}

void CANopenEncoder::coSDOwrite16bits(CANopenSensor *sensor, COdataType data)
{
	coSDOwriteAsync16bits(sensor, data);
}

void CANopenEncoder::coSDOwrite32bits(CANopenSensor *sensor, COdataType data)
{
	coSDOwriteAsync32bits(sensor, data);
}

#if CODATASZ >= 64
void CANopenEncoder::coSDOwrite64bits(CANopenSensor *sensor, COdataType data)
{
	coSDOwriteAsync64bits(sensor, data);
}
#endif

void CANopenEncoder::coSDOwriteString(CANopenSensor *sensor, COdataType data)
{
	coSDOwriteAsyncString(sensor, data);
}

lely::canopen::SdoFuture<void> CANopenEncoder::coSDOwriteAsync8bits(CANopenSensor *sensor, COdataType data)
{
	return sensor->AsyncWrite<uint8_t>((uint8_t)data);
}

lely::canopen::SdoFuture<void> CANopenEncoder::coSDOwriteAsync16bits(CANopenSensor *sensor, COdataType data)
{
	return sensor->AsyncWrite<uint16_t>((uint16_t)data);
}

lely::canopen::SdoFuture<void> CANopenEncoder::coSDOwriteAsync32bits(CANopenSensor *sensor, COdataType data)
{
	return sensor->AsyncWrite<uint32_t>((uint32_t)data);
}

#if CODATASZ >= 64
lely::canopen::SdoFuture<void> CANopenEncoder::coSDOwriteAsync64bits(CANopenSensor *sensor, COdataType data)
{
	return sensor->AsyncWrite<uint64_t>((uint64_t)data);
}
#endif

lely::canopen::SdoFuture<void> CANopenEncoder::coSDOwriteAsyncString(CANopenSensor *sensor, COdataType data)
{
	return sensor->AsyncWrite<::std::string>(std::string((const char*)data));
}

lely::canopen::SdoFuture<COdataType> CANopenEncoder::coSDOreadAsync8bits(CANopenSensor *sensor)
{
	return sensor->AsyncRead<uint8_t>().then(
		*sensor, [](lely::canopen::SdoFuture<uint8_t> f){
			try {
				return COdataType(f.get().value());
			} catch(...) {
				return COdataType();
			}
		});
}

lely::canopen::SdoFuture<COdataType> CANopenEncoder::coSDOreadAsync16bits(CANopenSensor *sensor)
{
	return sensor->AsyncRead<uint16_t>().then(
		*sensor, [](lely::canopen::SdoFuture<uint16_t> f){
			try {
				return COdataType(f.get().value());
			} catch(...) {
				return COdataType();
			}
		});
}

lely::canopen::SdoFuture<COdataType> CANopenEncoder::coSDOreadAsync32bits(CANopenSensor *sensor)
{
	return sensor->AsyncRead<uint32_t>().then(
		*sensor, [](lely::canopen::SdoFuture<uint32_t> f){
			try {
				return COdataType(f.get().value());
			} catch(...) {
				return COdataType();
			}
		});
}

#if CODATASZ >= 64
lely::canopen::SdoFuture<COdataType> CANopenEncoder::coSDOreadAsync64bits(CANopenSensor *sensor)
{
	return sensor->AsyncRead<uint64_t>().then(
		*sensor, [](lely::canopen::SdoFuture<uint64_t> f){
			try {
				return COdataType(f.get().value());
			} catch(...) {
				return COdataType();
			}
		});
}
#endif

lely::canopen::SdoFuture<COdataType> CANopenEncoder::coSDOreadAsyncString(CANopenSensor *sensor)
{
	return sensor->AsyncRead<std::string>().then(
		*sensor, [](lely::canopen::SdoFuture<std::string> f){
			try {
				return COdataType(f.get().value().c_str());
			} catch(...) {
				return COdataType();
			}
		});
}

void CANopenEncoder::coPDOwrite8bits(CANopenSensor *sensor, COdataType data)
{
	sensor->driver()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint8_t)data;
}

void CANopenEncoder::coPDOwrite16bits(CANopenSensor *sensor, COdataType data)
{
	sensor->driver()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint16_t)data;
}

void CANopenEncoder::coPDOwrite32bits(CANopenSensor *sensor, COdataType data)
{
	sensor->driver()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint32_t)data;
}

COdataType CANopenEncoder::coPDOread8bits(CANopenSensor *sensor)
{
	return COdataType((uint8_t)sensor->driver()->rpdo_mapped[sensor->reg()][sensor->subReg()]);
}

COdataType CANopenEncoder::coPDOread16bits(CANopenSensor *sensor)
{
	return COdataType((uint16_t)sensor->driver()->rpdo_mapped[sensor->reg()][sensor->subReg()]);
}

COdataType CANopenEncoder::coPDOread32bits(CANopenSensor *sensor)
{
	return COdataType((uint32_t)sensor->driver()->rpdo_mapped[sensor->reg()][sensor->subReg()]);
}

const std::map<int, CANopenEncodeCbS> CANopenEncoder::SDOfunctionCBs{
    {1, {nullptr, coSDOwrite8bits, coSDOreadAsync8bits, coSDOwriteAsync8bits}},
    {2, {nullptr, coSDOwrite16bits, coSDOreadAsync16bits, coSDOwriteAsync16bits}},
    {4, {nullptr, coSDOwrite32bits, coSDOreadAsync32bits, coSDOwriteAsync32bits}},
    {5, {nullptr, coSDOwriteString, coSDOreadAsyncString, coSDOwriteAsyncString}}};

const std::map<int, CANopenEncodeCbS> CANopenEncoder::RPDOfunctionCBs{
    {1, {coPDOread8bits, nullptr, nullptr, nullptr}},
    {2, {coPDOread16bits, nullptr, nullptr, nullptr}},
    {4, {coPDOread32bits, nullptr, nullptr, nullptr}}};

const std::map<int, CANopenEncodeCbS> CANopenEncoder::TPDOfunctionCBs{
    {1, {nullptr, coPDOwrite8bits, nullptr, nullptr}},
    {2, {nullptr, coPDOwrite16bits, nullptr, nullptr}},
    {4, {nullptr, coPDOwrite32bits, nullptr, nullptr}}};

const std::map<std::string, std::map<int, CANopenEncodeCbS>> CANopenEncoder::encodingTable{
    {"SDO", SDOfunctionCBs},
    {"TPDO", TPDOfunctionCBs},
    {"RPDO", RPDOfunctionCBs}};

std::map<std::string, coEncodeCB> CANopenEncoder::coEncodeFormaterTable{
    {"int", encodeInt},
    {"uint", encodeInt},
#if CODATADBL
    {"double", encodeDouble},
#endif
    {"string", encodeString}};

std::map<std::string, coDecodeCB> CANopenEncoder::coDecodeFormaterTable{
    {"int", decodeInt},
    {"uint", decodeUint},
#if CODATADBL
    {"double", decodeDouble},
#endif
    {"string", decodeString}};
