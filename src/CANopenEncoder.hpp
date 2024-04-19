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
#include <json-c/json.h>
#include <lely/coapp/sdo.hpp>

class CANopenSensor;

#ifndef CODATASZ
# define CODATASZ 64
#endif
#ifndef CODATADBL
# define CODATADBL 0
#endif


// Container for multiple type variable
class COdataType
{
#if CODATASZ == 64
	using u_t = uint64_t;
	using i_t = int64_t;
#elif CODATASZ == 32
	using u_t = uint32_t;
	using i_t = int32_t;
#else
# error "unexpected CODATASZ value"
#endif
	union
	{
		u_t u;
		const char *str;
	} v_;
	enum {
		t_void,
		t_u,
		t_str
	} t_;
public:
	void reset() { t_ = t_void; }
	COdataType() : t_{t_void} {}
	COdataType(int8_t v) : t_{t_u}, v_{.u{u_t(i_t(v))}} {}
	COdataType(uint8_t v) : t_{t_u}, v_{.u{u_t(v)}} {}
	COdataType(int16_t v) : t_{t_u}, v_{.u{u_t(i_t(v))}} {}
	COdataType(uint16_t v) : t_{t_u}, v_{.u{u_t(v)}} {}
	COdataType(int32_t v) : t_{t_u}, v_{.u{u_t(i_t(v))}} {}
	COdataType(uint32_t v) : t_{t_u}, v_{.u{u_t(v)}} {}
#if CODATASZ >= 64
	COdataType(int64_t v) : t_{t_u}, v_{.u{u_t(v)}} {}
	COdataType(uint64_t v) : t_{t_u}, v_{.u{v}} {}
#endif
	COdataType(const char *v) : t_{t_str}, v_{.str{v}} {}
	operator int8_t() const { return int8_t(v_.u); }
	operator uint8_t() const { return uint8_t(v_.u); }
	operator int16_t() const { return int16_t(v_.u); }
	operator uint16_t() const { return uint16_t(v_.u); }
	operator int32_t() const { return int32_t(v_.u); }
	operator uint32_t() const { return uint32_t(v_.u); }
	operator int64_t() const { return int64_t(v_.u); }
	operator uint64_t() const { return uint64_t(v_.u); }
	operator const char*() const { return v_.str; }
};

// Struct containing Read and Write functions callbacks
struct CANopenEncodeCbS
{
	COdataType (*readCB)(CANopenSensor *sensor);
	void (*writeCB)(CANopenSensor *sensor, COdataType data);
	lely::canopen::SdoFuture<COdataType> (*readAsyncCB)(CANopenSensor *sensor);
	lely::canopen::SdoFuture<void> (*writeAsyncCB)(CANopenSensor *sensor, COdataType data);
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
	const CANopenEncodeCbS &getfunctionCB(std::string type, int size);

	// return encode/decode formater callback based on it's uid
	coEncodeCB getEncodeFormaterCB(std::string encode);
	coDecodeCB getDecodeFormaterCB(std::string decode);

	// add an encoder formater to the list of availables encoders
	int addEncodeFormater(std::string uid, coEncodeCB encodeCB);
	int addEncodeFormater(const std::vector<std::pair<std::string, coEncodeCB>> &encodeFormaterTable);

	// add an decoder formater to the list of availables encoders
	int addDecodeFormater(std::string uid, coDecodeCB decodeCB);
	int addDecodeFormater(const std::vector<std::pair<std::string, coDecodeCB>> &decodeFormaterTable);

	// Built in Encoding Formaters
	static COdataType encodeInt(json_object *dataJ, CANopenSensor *sensor = nullptr);
	static COdataType encodeString(json_object *dataJ, CANopenSensor *sensor = nullptr);
#if CODATADBL
	static COdataType encodeDouble(json_object *dataJ, CANopenSensor *sensor = nullptr);
#endif

	// Built in Decoding Formaters
	static json_object *decodeInt(COdataType data, CANopenSensor *sensor = nullptr);
	static json_object *decodeUint(COdataType data, CANopenSensor *sensor = nullptr);
	static json_object *decodeString(COdataType data, CANopenSensor *sensor = nullptr);
#if CODATADBL
	static json_object *decodeDouble(COdataType data, CANopenSensor *sensor = nullptr);
#endif

private:
	CANopenEncoder(); ///< Private constructor for singleton implementation

	// SDO encoding functions
	static void coSDOwrite8bits(CANopenSensor *sensor, COdataType data);
	static void coSDOwrite16bits(CANopenSensor *sensor, COdataType data);
	static void coSDOwrite32bits(CANopenSensor *sensor, COdataType data);
	static void coSDOwriteString(CANopenSensor *sensor, COdataType data);
#if CODATASZ >= 64
	static void coSDOwrite64bits(CANopenSensor *sensor, COdataType data);
#endif

	// SDO encoding functions
	static lely::canopen::SdoFuture<void> coSDOwriteAsync8bits(CANopenSensor *sensor, COdataType data);
	static lely::canopen::SdoFuture<void> coSDOwriteAsync16bits(CANopenSensor *sensor, COdataType data);
	static lely::canopen::SdoFuture<void> coSDOwriteAsync32bits(CANopenSensor *sensor, COdataType data);
	static lely::canopen::SdoFuture<void> coSDOwriteAsyncString(CANopenSensor *sensor, COdataType data);
#if CODATASZ >= 64
	static lely::canopen::SdoFuture<void> coSDOwriteAsync64bits(CANopenSensor *sensor, COdataType data);
#endif

	// SDO decoding functions
	static lely::canopen::SdoFuture<COdataType> coSDOreadAsync8bits(CANopenSensor *sensor);
	static lely::canopen::SdoFuture<COdataType> coSDOreadAsync32bits(CANopenSensor *sensor);
	static lely::canopen::SdoFuture<COdataType> coSDOreadAsyncString(CANopenSensor *sensor);
	static lely::canopen::SdoFuture<COdataType> coSDOreadAsync16bits(CANopenSensor *sensor);
#if CODATASZ >= 64
	static lely::canopen::SdoFuture<COdataType> coSDOreadAsync64bits(CANopenSensor *sensor);
#endif

	// PDO encoding functions
	static void coPDOwrite8bits(CANopenSensor *sensor, COdataType data);
	static void coPDOwrite16bits(CANopenSensor *sensor, COdataType data);
	static void coPDOwrite32bits(CANopenSensor *sensor, COdataType data);

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
	static std::map<std::string, coEncodeCB> coEncodeFormaterTable;
	static std::map<std::string, coDecodeCB> coDecodeFormaterTable;
};

#endif //_CANOPENENCODER_INCLUDE_
