
#ifndef ASSEMBLE_H_INCLUDED
#define ASSEMBLE_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "parse.h"
#include "config.h"
#include "asmcontext.h"

int assemble(AssembleContext&, Configuration&, Program&);
int build(AssembleContext&, Configuration&);

int assemble_structure(AssembleContext&, Configuration&, Program&, Structure&);
int assemble_function(AssembleContext&, Configuration&, Program&, Function&);
int assemble_function_body(AssembleContext&, Configuration&, Program&, Function&);
int assemble_external(AssembleContext&, Configuration&, Program&, External&);
int assemble_statement(AssembleContext&, Configuration&, Program&, Function&, AssembleFunction&, Statement&);

int assemble_merge_types(AssembleContext&, const std::string&, const std::string&, llvm::Value**, llvm::Value**, std::string*);
int assemble_merge_types_oneway(AssembleContext&, const std::string&, const std::string&, llvm::Value**, llvm::Type*, std::string*);

#endif // ASSEMBLE_H_INCLUDED
