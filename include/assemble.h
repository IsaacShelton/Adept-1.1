
#ifndef ASSEMBLE_H_INCLUDED
#define ASSEMBLE_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "parse.h"
#include "config.h"
#include "asmcontext.h"

int assemble(AssembleContext&, Configuration&, Program&, ErrorHandler& errors);
int build(AssembleContext&, Configuration&, Program&);

int assemble_globals_batch(AssembleContext* context, const Configuration* config, const Program* program, Global* globals, size_t globals_count);
int assemble_externals_batch(AssembleContext* context, const Configuration* config, const Program* program, External* globals, size_t globals_count);

int assemble_structure(AssembleContext&, Configuration&, Program&, Structure&);
int assemble_class(AssembleContext&, Configuration&, Program&, Class&);
int assemble_class_body(AssembleContext&, Configuration&, Program&, Class&);
int assemble_function(AssembleContext&, Configuration&, Program&, Function&);
int assemble_function_body(AssembleContext&, Configuration&, Program&, Function&);
int assemble_method(AssembleContext&, Configuration&, Program&, Class&, Function&);
int assemble_method_body(AssembleContext&, Configuration&, Program&, Class&, Function&);
int assemble_external(AssembleContext*, const Configuration*, const Program*, const External*);
int assemble_global(AssembleContext*, const Configuration*, const Program*, Global*);

void assemble_merge_conditional_types(AssembleContext&, Program&, std::string&, llvm::Value**);
int assemble_merge_types(AssembleContext&, Program&, std::string, std::string, llvm::Value**, llvm::Value**, std::string*);
int assemble_merge_types_oneway(AssembleContext&, Program&, std::string, std::string, llvm::Value**, llvm::Type*, std::string*);
bool assemble_types_mergeable(Program&, std::string, std::string);

#endif // ASSEMBLE_H_INCLUDED
