
#include "../include/strings.h"
#include "../include/die.h"
#include "../include/asmdata.h"

AssemblyData::AssemblyData() : builder(context) {
    assembling_constant_expression = false;
    constant_expression_depth = 0;
}

AssembleFunction* AssemblyData::addFunction(const std::string& mangled_function_name){
    functions.push_back( AssembleFunction(mangled_function_name) );
    return &functions[functions.size()-1];
}

AssembleFunction* AssemblyData::getFunction(const std::string& mangled_function_name){
    for(std::vector<AssembleFunction>::iterator i = functions.begin(); i != functions.end(); ++i){
        if(i->mangled_name == mangled_function_name){
            return &(*i);
        }
    }

    fail("AssembleFunction requested which doesn't exist inside AssemblyData instance");
    fail(SUICIDE);
    return NULL;
}

int AssemblyData::addVariableToFunction(const std::string& mangled_function_name, const std::string& var_name, const std::string& var_type, llvm::Value* alloca){
    // Adds variable assembly data to assembly function data

    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].mangled_name == mangled_function_name){
            functions[i].addVariable(var_name, var_type, alloca);
            return 0;
        }
    }

    fail("AssembleFunction requested which doesn't exist inside AssemblyData instance");
    fail(SUICIDE);
    return 1;
}

void AssemblyData::addGlobal(const std::string& name){
    globals.push_back( AssembleGlobal(name) );
}

AssembleGlobal* AssemblyData::findGlobal(const std::string& name){
    for(size_t i = 0; i != globals.size(); i++){
        if(globals[i].name == name){
            return &globals[i];
        }
    }

    return NULL;
}
