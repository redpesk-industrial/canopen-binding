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

#ifndef _CANOPENENCODER_INCLUDE_
#define _CANOPENENCODER_INCLUDE_

#include <map>
#include <iostream>
#include <json-c/json.h>

class CANopenSensor;

// Container for multiple type variable
union COdataType
{
	int error;
	int32_t tInt;
	int64_t tDouble;
	const char *tString;
};

// Struct containing Read and Write functions callbacks
struct CANopenEncodeCbS
{
	COdataType (*readCB)(CANopenSensor *sensor);
	int (*writeCB)(CANopenSensor *sensor, COdataType data);
};

// type definition for encode callback
typedef COdataType (*coEncodeCB)(json_object *dataJ, CANopenSensor *sensor);

// type definition for decode callback
typedef json_object *(*coDecodeCB)(COdataType data, CANopenSensor *sensor);

class CANopenEncoder
{
public:
	// Return singleton instance of configuration object.
	static CANopenEncoder &instance();

	// return the appropriate read and write function callback according to sensor size and type
	CANopenEncodeCbS getfunctionCB(std::string type, int size);

	// return encode/decode formater callback based on it's uid
	coEncodeCB getEncodeFormateurCB(std::string encode);
	coDecodeCB getDecodeFormateurCB(std::string decode);

	// add an encoder formater to the list of availables encoders
	int addEncodeFormateur(std::string uid, coEncodeCB encodeCB);
	int addEncodeFormateur(std::map<std::string, coEncodeCB> newEncodeFormaterTable);

	// add an decoder formater to the list of availables encoders
	int addDecodeFormateur(std::string uid, coDecodeCB decodeCB);
	int addDecodeFormateur(std::map<std::string, coDecodeCB> newDecodeFormaterTable);

	// Built in Encoding Formaters
	static COdataType encodeInt(json_object *dataJ, CANopenSensor *sensor = nullptr);
	static COdataType encodeDouble(json_object *dataJ, CANopenSensor *sensor = nullptr);
	static COdataType encodeString(json_object *dataJ, CANopenSensor *sensor = nullptr);

	// Built in Decoding Formaters
	static json_object *decodeInt(COdataType data, CANopenSensor *sensor = nullptr);
	static json_object *decodeUint(COdataType data, CANopenSensor *sensor = nullptr);
	static json_object *decodeDouble(COdataType data, CANopenSensor *sensor = nullptr);
	static json_object *decodeString(COdataType data, CANopenSensor *sensor = nullptr);

private:
	CANopenEncoder(); ///< Private constructor for singleton implementation

	// SDO encoding functions
	static int coSDOwrite8bits(CANopenSensor *sensor, COdataType data);
	static int coSDOwrite16bits(CANopenSensor *sensor, COdataType data);
	static int coSDOwrite32bits(CANopenSensor *sensor, COdataType data);
	static int coSDOwrite64bits(CANopenSensor *sensor, COdataType data);
	static int coSDOwriteString(CANopenSensor *sensor, COdataType data);
	// SDO decoding functions
	static COdataType coSDOread8bits(CANopenSensor *sensor);
	static COdataType coSDOread32bits(CANopenSensor *sensor);
	static COdataType coSDOread64bits(CANopenSensor *sensor);
	static COdataType coSDOreadString(CANopenSensor *sensor);
	static COdataType coSDOread16bits(CANopenSensor *sensor);
	// PDO encoding functions
	static int coPDOwrite8bits(CANopenSensor *sensor, COdataType data);
	static int coPDOwrite16bits(CANopenSensor *sensor, COdataType data);
	static int coPDOwrite32bits(CANopenSensor *sensor, COdataType data);
	// PDO decoding functions
	static COdataType coPDOread16bits(CANopenSensor *sensor);
	static COdataType coPDOread32bits(CANopenSensor *sensor);
	static COdataType coPDOread8bits(CANopenSensor *sensor);

	// read/write functions tables
	static const std::map<int, CANopenEncodeCbS> SDOfunctionCBs;
	static const std::map<int, CANopenEncodeCbS> RPDOfunctionCBs;
	static const std::map<int, CANopenEncodeCbS> TPDOfunctionCBs;
	static const std::map<std::string, std::map<int, CANopenEncodeCbS>> encodingTable;

	// formaters Tables
	static std::map<std::string, coEncodeCB> coEncodeFormateurTable;
	static std::map<std::string, coDecodeCB> coDecodeFormateurTable;
};

#endif //_CANOPENENCODER_INCLUDE_
