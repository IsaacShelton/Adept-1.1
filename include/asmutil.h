
#ifndef ASMUTIL_H_INCLUDED
#define ASMUTIL_H_INCLUDED

#include "asmdata.h"
#include "program.h"

void create_assembly_globals(AssemblyData& assembly_data, const std::vector<Global>& ast_globals);
void create_assembly_functions(AssemblyData& assembly_data, Program&);

#endif // ASMUTIL_H_INCLUDED
