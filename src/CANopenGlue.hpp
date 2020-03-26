#ifndef _CANOPENGLUE_HEADER_
#define _CANOPENGLUE_HEADER_

#include <iostream>
#include <wrap-json.h>

typedef enum{
    DATA_TYPE_NULL,
    DATA_TYPE_DECIMAL,
    DATA_TYPE_STR_DECIMAL,
    DATA_TYPE_HEX,
    DATA_TYPE_STRING
} data_type;

data_type get_data_type(std::string data);
data_type get_data_type(json_object * dataJ);
int get_data_int(json_object * dataJ);
double get_data_double(json_object * dataJ);

#endif /* _CANOPENGLUE_HEADER_ */