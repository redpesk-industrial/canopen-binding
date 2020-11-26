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

// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/
#include "CANopenEncoder.hpp"
#include "CANopenGlue.hpp"

#ifndef ERROR
    #define ERROR -1
#endif

CANopenEncoder::CANopenEncoder() {}

/// @brief Return singleton instance of configuration object.
CANopenEncoder& CANopenEncoder::instance()
{
	static CANopenEncoder encoder;
	return encoder;
}

CANopenEncodeCbS CANopenEncoder::getfunctionCB(std::string type, int size){
    CANopenEncodeCbS fn;

    try{
        fn = encodingTable.at(type).at(size);
    } catch(std::out_of_range& e){
        throw e;
    }
    return fn;
}

coEncodeCB CANopenEncoder::getEncodeFormateurCB(std::string format){
    coEncodeCB ef;
    try{
        ef = coEncodeFormateurTable.at(format);
    } catch(std::out_of_range& e){
        throw e;
    }
    return ef;
}

coDecodeCB CANopenEncoder::getDecodeFormateurCB(std::string format){
    coDecodeCB df;
    try{
        df = coDecodeFormateurTable.at(format);
    } catch(std::out_of_range& e){
        throw e;
    }
    return df;
}

// add an encoder formater to the list of availables encoders
int CANopenEncoder::addEncodeFormateur(std::string uid, coEncodeCB encodeCB){
    auto [it, result] = coEncodeFormateurTable.emplace(std::make_pair(uid, encodeCB));
    if(!result){
        throw(std::runtime_error(std::string("entree " + it->first + " could not be added because it alredy exist\n")));
        return ERROR;
    }
    return 0;
}

// add an antier list of encoder formater to the list of availables encoders
int CANopenEncoder::addEncodeFormateur(std::map<std::string, coEncodeCB> newEncodeFormaterTable){
    int err = 0;
    std::string errWhat;
    for(auto & newEncoder : newEncodeFormaterTable){
        try{
            err += addEncodeFormateur(newEncoder.first, newEncoder.second);
        }catch(std::runtime_error& e){
            errWhat += e.what();
        }
    }
    if(err){
        throw(std::runtime_error(errWhat));
        return err;
    }
    return 0;
 }

// add an decoder formater to the list of availables decoders
int CANopenEncoder::addDecodeFormateur(std::string uid, coDecodeCB decodeCB){
    auto[it, result] = coDecodeFormateurTable.emplace(std::make_pair(uid, decodeCB));
    std::cout << "CANopenEncoder DEBUG : entree '" << uid << "' added at coDecodeFormateurTable : '" << it->first << "''" << it->second << "' resutl => " << result << " table size : " << coDecodeFormateurTable.size() << std::endl;
    if(!result) return ERROR;
    return 0;
}

// add an antier list of decoder formater to the list of availables decoders
int CANopenEncoder::addDecodeFormateur(std::map<std::string, coDecodeCB> newDecodeFormaterTable){
    int err = 0;
    std::string errWhat;
    for(auto & newDecoder : newDecodeFormaterTable){
        try{
            err += addDecodeFormateur(newDecoder.first, newDecoder.second);
        }catch(std::runtime_error& e){
            errWhat += e.what();
        }
    }
    if(err){
        throw(std::runtime_error(errWhat));
        return err;
    }
    return 0;
}

COdataType CANopenEncoder::encodeInt(json_object * dataJ, CANopenSensor* sensor){
    COdataType data;
    data.tInt = get_data_int(dataJ);
    return data;
}

COdataType CANopenEncoder::encodeDouble(json_object * dataJ, CANopenSensor* sensor){
    COdataType data;
    data.tDouble = (int64_t)get_data_double(dataJ);
    return data;
}

COdataType CANopenEncoder::encodeString(json_object * dataJ, CANopenSensor* sensor){
    COdataType data;
    data.tString = json_object_get_string(dataJ);
    return data;
}

json_object * CANopenEncoder::decodeInt(COdataType data, CANopenSensor* sensor){
    return json_object_new_int(data.tInt);
}

json_object * CANopenEncoder::decodeUint(COdataType data, CANopenSensor* sensor){
    return json_object_new_int64(data.tInt);
}

json_object * CANopenEncoder::decodeDouble(COdataType data, CANopenSensor* sensor){
    return json_object_new_double((double)data.tDouble);
}

json_object * CANopenEncoder::decodeString(COdataType data, CANopenSensor* sensor){
    return json_object_new_string(data.tString);
}

int CANopenEncoder::coSDOwrite8bits(CANopenSensor* sensor, COdataType data){
    sensor->slave()->AsyncWrite<uint8_t>(sensor->reg(), sensor->subReg(), (uint8_t)data.tInt);
    return 0;
}

int CANopenEncoder::coSDOwrite16bits(CANopenSensor* sensor, COdataType data){
    sensor->slave()->AsyncWrite<uint16_t>(sensor->reg(), sensor->subReg(), (uint16_t)data.tInt);
    return 0;
}

int CANopenEncoder::coSDOwrite32bits(CANopenSensor* sensor, COdataType data){
    sensor->slave()->AsyncWrite<uint32_t>(sensor->reg(), sensor->subReg(), (uint32_t)data.tInt);
    return 0;
}

int CANopenEncoder::coSDOwrite64bits(CANopenSensor* sensor, COdataType data){
    sensor->slave()->AsyncWrite<uint64_t>(sensor->reg(), sensor->subReg(), (uint64_t)data.tDouble);
    return 0;
}

int CANopenEncoder::coSDOwriteString(CANopenSensor* sensor, COdataType data){
    sensor->slave()->AsyncWrite<::std::string>(sensor->reg(), sensor->subReg(), data.tString);
    return 0;
}

COdataType CANopenEncoder::coSDOread8bits(CANopenSensor* sensor){
    COdataType val;
    val.tInt = sensor->slave()->Wait(sensor->slave()->AsyncRead<uint8_t>(sensor->reg(), sensor->subReg()));
    return val;
}

COdataType CANopenEncoder::coSDOread16bits(CANopenSensor* sensor){
    COdataType val;
    val.tInt = sensor->slave()->Wait(sensor->slave()->AsyncRead<uint16_t>(sensor->reg(), sensor->subReg()));
    return val;
}

COdataType CANopenEncoder::coSDOread32bits(CANopenSensor* sensor){
    COdataType val;
    val.tInt = sensor->slave()->Wait(sensor->slave()->AsyncRead<uint32_t>(sensor->reg(), sensor->subReg()));
    return val;
}

COdataType CANopenEncoder::coSDOread64bits(CANopenSensor* sensor){
    COdataType val;
    val.tDouble = sensor->slave()->Wait(sensor->slave()->AsyncRead<uint64_t>(sensor->reg(), sensor->subReg()));
    return val;
}

COdataType CANopenEncoder::coSDOreadString(CANopenSensor* sensor){
    COdataType val;
    val.tString = sensor->slave()->Wait(sensor->slave()->AsyncRead<::std::string>(sensor->reg(), sensor->subReg())).c_str();
    return val;
}

int CANopenEncoder::coPDOwrite8bits(CANopenSensor* sensor, COdataType data){
    sensor->slave()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint8_t)data.tInt;
    return 0;
}

int CANopenEncoder::coPDOwrite16bits(CANopenSensor* sensor, COdataType data){
    sensor->slave()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint16_t)data.tInt;
    return 0;
}

int CANopenEncoder::coPDOwrite32bits(CANopenSensor* sensor, COdataType data){
    sensor->slave()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint32_t)data.tInt;
    return 0;
}

COdataType CANopenEncoder::coPDOread8bits(CANopenSensor* sensor){
    COdataType val;
    val.tInt = (uint8_t)sensor->slave()->rpdo_mapped[sensor->reg()][sensor->subReg()];
    return val;
}

COdataType CANopenEncoder::coPDOread16bits(CANopenSensor* sensor){
    COdataType val;
    val.tInt = (uint16_t)sensor->slave()->rpdo_mapped[sensor->reg()][sensor->subReg()];
    return val;
}

COdataType CANopenEncoder::coPDOread32bits(CANopenSensor* sensor){
    COdataType val;
    val.tInt = (uint32_t)sensor->slave()->rpdo_mapped[sensor->reg()][sensor->subReg()];
    return val;
}


const std::map<int, CANopenEncodeCbS> CANopenEncoder::SDOfunctionCBs {
    {1,{coSDOread8bits , coSDOwrite8bits}},
    {2,{coSDOread16bits, coSDOwrite16bits}},
    {4,{coSDOread32bits, coSDOwrite32bits}},
    {5,{coSDOreadString, coSDOwriteString}}
};

const std::map<int, CANopenEncodeCbS> CANopenEncoder::RPDOfunctionCBs {
    {1, {coPDOread8bits , nullptr}},
    {2, {coPDOread16bits, nullptr}},
    {4, {coPDOread32bits, nullptr}}
};

const std::map<int, CANopenEncodeCbS> CANopenEncoder::TPDOfunctionCBs {
    {1,{nullptr, coPDOwrite8bits }},
    {2,{nullptr, coPDOwrite16bits}},
    {4,{nullptr, coPDOwrite32bits}}
};

const std::map<std::string, std::map<int, CANopenEncodeCbS>> CANopenEncoder::encodingTable {
    {"SDO" , SDOfunctionCBs },
    {"TPDO", TPDOfunctionCBs},
    {"RPDO", RPDOfunctionCBs}
};

std::map<std::string, coEncodeCB> CANopenEncoder::coEncodeFormateurTable {
    {"int"   , encodeInt   },
    {"uint"  , encodeInt   },
    {"double", encodeDouble},
    {"string", encodeString}
};

std::map<std::string, coDecodeCB> CANopenEncoder::coDecodeFormateurTable {
    {"int"   , decodeInt   },
    {"uint"  , decodeUint  },
    {"double", decodeDouble},
    {"string", decodeString}
};