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

#include <algorithm>

#include "CANopenGlue.hpp"

data_type get_data_type(std::string str){
    if(std::all_of(str.begin(), str.end(), ::isdigit)){
        return DATA_TYPE_STR_DECIMAL;
    }
    if((str.compare(0,2,"0x") || str.compare(0,2,"0X")) && std::all_of(str.begin()+2, str.end(), ::isxdigit)){
        return DATA_TYPE_HEX;
    }
    return DATA_TYPE_STRING;
}

data_type get_data_type(json_object * dataJ){
    if(json_object_is_type(dataJ, json_type_string)){
        return get_data_type(json_object_get_string(dataJ));
    }
    if(json_object_is_type(dataJ, json_type_int) || json_object_is_type(dataJ, json_type_double)){
        return DATA_TYPE_DECIMAL;
    }
    return DATA_TYPE_NULL;

}

int get_data_int(json_object * dataJ){
    switch(get_data_type(dataJ)){
        case DATA_TYPE_HEX :
            return stoi(std::string(json_object_get_string(dataJ)).substr(2),0,16);
        case DATA_TYPE_STR_DECIMAL :
            return stoi(std::string(json_object_get_string(dataJ)));
        case DATA_TYPE_DECIMAL :
            return json_object_get_int(dataJ);
        default :
            throw std::runtime_error("data " + (std::string)json_object_to_json_string(dataJ) + " not handeled by get_data_int");
            return 0;
    }
}

double get_data_double(json_object * dataJ){
    switch(get_data_type(dataJ)){
        case DATA_TYPE_HEX :
            return stod(std::string(json_object_get_string(dataJ)));
        case DATA_TYPE_STR_DECIMAL :
            return stod(std::string(json_object_get_string(dataJ)));
        case DATA_TYPE_DECIMAL :
            return json_object_get_double(dataJ);
        default :
            throw std::runtime_error("data " + (std::string)json_object_to_json_string(dataJ) + " not handeled by get_data_double");
            return 0;
    }
}