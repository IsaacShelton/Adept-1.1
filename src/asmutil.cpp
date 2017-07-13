
#include "../include/die.h"
#include "../include/asmutil.h"
#include "../include/asmtypes.h"
#include "../include/mangling.h"

void create_assembly_globals(AssemblyData& assembly_data, const std::vector<Global>& ast_globals){
    assembly_data.globals.resize(ast_globals.size());
    size_t i = 0;

    for(const Global& global : ast_globals){
        assembly_data.globals[i++] = AssembleGlobal(global.name);
    }
}

void create_assembly_functions(AssemblyData& assembly_data, Program& program){
    ensure(assembly_data.functions.size() == 0);

    std::vector<Class>& ast_classes = program.classes;
    std::vector<Function>& ast_functions = program.functions;
    std::vector<AssembleFunction>& export_functions = assembly_data.functions;
    size_t ast_classes_method_count = 0;
    size_t index = 0;

    for(const Class& klass : ast_classes){
        if(klass.is_imported) continue;
        ast_classes_method_count += klass.methods.size();
    }

    export_functions.resize(ast_functions.size() + ast_classes_method_count);

    for(const Function& func : ast_functions){
        export_functions[index++].mangled_name = (func.is_external) ? func.name : mangle(program, func);
    }

    for(const Class& klass : ast_classes){
        if(klass.is_imported) continue;

        for(const Function& method : klass.methods){
            export_functions[index++].mangled_name = mangle(klass, method);
        }
    }
}
