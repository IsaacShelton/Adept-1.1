
#ifndef ASSEMBLE_H_INCLUDED
#define ASSEMBLE_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "config.h"
#include "parse.h"
#include "asmcontext.h"

void assemble_init();
int assemble(AssembleContext& context, Configuration& config, Program& program);
int assemble_structure(AssembleContext& context, Configuration& config, Program& program, Structure& structure);
int assemble_function(AssembleContext& context, Configuration& config, Program& program, Function& func);
int assemble_external(AssembleContext& context, Configuration& config, Program& program, External& func);
int assemble_statement(AssembleContext& context, Configuration& config, Program& program, Function& func, Statement& statement);

#endif // ASSEMBLE_H_INCLUDED
