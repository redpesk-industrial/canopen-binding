
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