
#include <iostream>
#include "../include/strings.h"
#include "../include/strings.h"
#include "../include/asmtypes.h"

AssembleVariable::AssembleVariable(){}
AssembleVariable::AssembleVariable(const std::string& name, const std::string& type, llvm::Value* variable){
    this->name = name;
    this->type = type;
    this->variable = variable;
}

AssembleGlobal::AssembleGlobal(){}
AssembleGlobal::AssembleGlobal(const std::string& name){
    this->name = name;
}

AssembleFunction::AssembleFunction(const std::string& mangled_name){
    this->mangled_name = mangled_name;
}
void AssembleFunction::addVariable(const std::string& name, const std::string& type, llvm::Value* variable){
    variables.push_back( AssembleVariable(name, type, variable) );
}
llvm::Value* AssembleFunction::getVariableValue(const std::string& name){
    for(size_t i = 0; i != variables.size(); i++){
        if(variables[i].name == name){
            return variables[i].variable;
        }
    }

    return NULL;
}
AssembleVariable* AssembleFunction::findVariable(const std::string& name){
    for(size_t i = 0; i != variables.size(); i++){
        if(variables[i].name == name){
            return &variables[i];
        }
    }

    return NULL;
}
