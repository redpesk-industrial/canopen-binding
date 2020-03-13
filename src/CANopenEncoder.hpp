#include <map>
#include <iostream>
#include <json-c>

class CANopenEncoder
{
private:
    static json_object coEncode_bool(std::string);
    static json_object coEncode_uint8(std::string);
    static json_object coEncode_uint16(std::string);
    static json_object coEncode_uint32(std::string);
    static json_object coEncode_uint(std::string);
    static json_object coEncode_char(std::string);
    static json_object coEncode_short(std::string);
    static json_object coEncode_int(std::string);
    static json_object coEncode_long(std::string);
    static json_object coEncode_double(std::string);
    static json_object coEncode_float(std::string);
    static json_object coEncode_string(std::string);

public:
    CANopenEncoder(/* args */);
    typedef json_object (*TypeCB)(std::string);
    static std::map<std::string, TypeCB> avalableTypeCBs {
        {"bool", coEncode_bool},
        {"uint8", coEncode_uint8},
        {"uint16", coEncode_uint16},
        {"uint32", coEncode_uint32},
        {"uint", coEncode_uint},
        {"char", coEncode_char},
        {"int", coEncode_short},
        {"short", coEncode_int},
        {"long", coEncode_long},
        {"double", coEncode_double},
        {"float", coEncode_float},
        {"string", coEncode_string},
    };
    ~CANopenEncoder();
};

CANopenEncoder::CANopenEncoder(/* args */)
{
}

CANopenEncoder::~CANopenEncoder()
{
}
