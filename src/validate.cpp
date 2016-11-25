
#include <iostream>
#include "../include/errors.h"
#include "../include/validate.h"

bool validate_return_statement(Program& program, const ReturnStatement& ret, Function& func){
    std::string valuetype;

    if(!ret.value->getType(program, func, valuetype)){
        fail(INCOMPATIBLE_TYPES);
        return false;
    }

    if(valuetype != func.return_type){
        fail(INCOMPATIBLE_WITH_RETURN);
        return false;
    }

    return true;
}

bool validate_declareas_statement(Program& program, const DeclareAsStatement& declareas, Function& func){
    std::string valuetype;

    if(!declareas.value->getType(program, func, valuetype)){
        fail(INCOMPATIBLE_TYPES);
        return false;
    }

    if(valuetype != declareas.type){
        fail(INCOMPATIBLE_EXPRESSION);
        return false;
    }

    return true;
}

bool validate_assign_statement(Program& program, const AssignStatement& assignto, const Variable& var, Function& func){
    std::string valuetype;

    if(!assignto.value->getType(program, func, valuetype)){
        fail(INCOMPATIBLE_TYPES);
        return false;
    }

    if(valuetype != var.type){
        fail(INCOMPATIBLE_EXPRESSION);
        return false;
    }

    return true;
}
