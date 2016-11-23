
#ifndef JIT_H_INCLUDED
#define JIT_H_INCLUDED

#include <string>
#include "asmcontext.h"

void jit_init();
int jit_main(AssembleContext&, std::string&);

#endif // JIT_H_INCLUDED
