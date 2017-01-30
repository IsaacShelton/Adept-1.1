
#include "../include/attribute.h"

AttributeInfo::AttributeInfo(){
    is_public = false;
    is_static = false;
}
AttributeInfo::AttributeInfo(bool attr_public){
    is_public = attr_public;
    is_static = false;
}
AttributeInfo::AttributeInfo(bool attr_public, bool attr_static){
    is_public = attr_public;
    is_static = attr_static;
}
