
#ifndef ADEPT_H_INCLUDED
#define ADEPT_H_INCLUDED

#include "../include/parse.h"
#include "../include/errors.h"
#include "../include/config.h"
#include "../include/tokens.h"
#include "../include/asmcontext.h"

int finalize(Configuration& config, AssembleContext& assemble, ErrorHandler& errors);

#endif // ADEPT_H_INCLUDED
