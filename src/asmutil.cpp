
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

    std::vector<Struct>& ast_structs = program.structures;
    std::vector<Function>& ast_functions = program.functions;
    std::vector<AssembleFunction>& export_functions = assembly_data.functions;
    size_t ast_structs_method_count = 0;
    size_t index = 0;

    for(const Struct& structure : ast_structs){
        if(structure.flags & STRUCT_IMPORTED) continue;
        ast_structs_method_count += structure.methods.size();
    }

    export_functions.resize(ast_functions.size() + ast_structs_method_count);

    for(const Function& func : ast_functions){
        export_functions[index++].mangled_name = (func.flags & FUNC_EXTERNAL) ? func.name : mangle(program, func);
    }

    for(const Struct& structure : ast_structs){
        if(structure.flags & STRUCT_IMPORTED) continue;

        for(const Function& method : structure.methods){
            export_functions[index++].mangled_name = mangle(structure, method);
        }
    }
}
