
#ifndef LLC_H_INCLUDED
#define LLC_H_INCLUDED

#include "../include/asmcontext.h"

int native_build_module(AssembleContext& context, std::string bitcode_filename, std::string output_filename);

#endif // LLC_H_INCLUDED
