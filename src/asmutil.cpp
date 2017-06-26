
#include "../include/asmutil.h"

void create_assembly_globals(AssemblyData& assembly_data, const std::vector<Global>& ast_globals){
    assembly_data.globals.resize(ast_globals.size());
    size_t i = 0;

    for(const Global& global : ast_globals){
        assembly_data.globals[i++] = AssembleGlobal(global.name);
    }
}
