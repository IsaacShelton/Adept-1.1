
#include <iostream>
#include "../include/validate.h"

bool validate_return_statement(Program& program, const ReturnStatement& ret, Function& func){
    std::string valuetype;

    if(!ret.value->getType(program, func, valuetype)){
        std::cerr << "Expression contains incompatible types" << std::endl;
        return false;
    }

    if(valuetype != func.return_type){
        std::cerr << "Expression type is incompatible with return type" << std::endl;
        return false;
    }

    return true;
}
