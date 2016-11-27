
#include <iostream>
#include "../include/errors.h"
#include "../include/validate.h"

bool validate_types(const std::string& a, const std::string& b, std::string* out){
    if(a == "" or b == "") return false;
    if(a == "void" or b == "void") return false;
    if(a == b){
        if(out != NULL) *out = a;
        return true;
    }
    if(a[0] == '*' and b == "ptr"){
        if(out != NULL) *out = a;
        return true;
    }
    if(b[0] == '*' and a == "ptr"){
        if(out != NULL) *out = b;
        return true;
    }

    return false;
}

bool validate_return_statement(Program& program, ReturnStatement& ret, Function& func){
    std::string valuetype;

    // Returning Void
    if(ret.value == NULL) return true;

    // Make sure types match
    if(!ret.value->getType(program, func, valuetype)){
        fail(INCOMPATIBLE_TYPES_VAGUE);
        return false;
    }
    if( !validate_types(valuetype, func.return_type, NULL) ){
        fail(INCOMPATIBLE_WITH_RETURN);
        return false;
    }

    return true;
}

bool validate_declareas_statement(Program& program, DeclareAsStatement& declareas, Function& func){
    std::string valuetype;

    if(!declareas.value->getType(program, func, valuetype)){
        fail(INCOMPATIBLE_TYPES_VAGUE);
        return false;
    }

    if( !validate_types(valuetype, declareas.type, NULL) ){
        fail(INCOMPATIBLE_EXPRESSION);
        return false;
    }

    return true;
}

bool validate_assign_statement(Program& program, AssignStatement& assignto, const Variable& var, Function& func){
    std::string valuetype;

    if(!assignto.value->getType(program, func, valuetype)){
        fail(INCOMPATIBLE_TYPES_VAGUE);
        return false;
    }

    if( !validate_types(valuetype, var.type, NULL) ){
        fail(INCOMPATIBLE_EXPRESSION);
        return false;
    }

    return true;
}
