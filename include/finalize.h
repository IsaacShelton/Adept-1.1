
#ifndef FINALIZE_H_INCLUDED
#define FINALIZE_H_INCLUDED

#include "../include/parse.h"
#include "../include/errors.h"
#include "../include/config.h"
#include "../include/tokens.h"
#include "../include/asmdata.h"

int finalize(AssemblyData& assemble, Config& config, Program& program, ErrorHandler& errors);
int shutdown();

#endif // FINALIZE_H_INCLUDED
