
#include "../include/attribute.h"

AttributeInfo::AttributeInfo(){
    is_public = false;
    is_external = false;
    is_static = false;
    is_packed = false;
    is_stdcall = false;
}
AttributeInfo::AttributeInfo(bool attr_public){
    is_public = attr_public;
    is_external = false;
    is_static = false;
    is_packed = false;
    is_stdcall = false;
}
void AttributeInfo::reset(){
    is_public = false;
    is_external = false;
    is_static = false;
    is_packed = false;
    is_stdcall = false;
}
