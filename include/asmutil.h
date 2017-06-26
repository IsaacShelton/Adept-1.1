
#ifndef ASMUTIL_H_INCLUDED
#define ASMUTIL_H_INCLUDED

#include "program.h"

void create_assembly_globals(AssemblyData& assembly_data, const std::vector<Global>& ast_globals);

#endif // ASMUTIL_H_INCLUDED
