
#ifndef ASSEMBLE_H_INCLUDED
#define ASSEMBLE_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "parse.h"
#include "config.h"
#include "native.h"
#include "search.h"
#include "asmdata.h"

int build(AssemblyData&, Config&, Program&);
int build_program(AssemblyData&, Config&, Program&, ErrorHandler&);
int build_buildscript(AssemblyData&, Config&, Program&, ErrorHandler&);
int build_dependencies(AssemblyData*, Program*, ModuleBuildOptions*, ErrorHandler*, ModuleDependency**, size_t);

int assemble(AssemblyData&, Config&, Program&, ErrorHandler& errors);

int assemble_globals_batch(AssemblyData* context, const Config* config, const Program* program, Global* globals, size_t globals_count);
int assemble_externals_batch(AssemblyData* context, const Config* config, const Program* program, External* globals, size_t globals_count);

int assemble_struct_skeletons(AssemblyData&, Config&, Program&);
int assemble_function_skeletons(AssemblyData&, Config&, Program&);
int assemble_function_bodies(AssemblyData*, Config*, Program*);
int assemble_struct_method_skeletons(AssemblyData&, Config&, Program&);
int assemble_struct_method_bodies(AssemblyData*, Config*, Program*);

void assemble_merge_conditional_types(AssemblyData&, Program&, std::string&, llvm::Value**);
int assemble_merge_types(AssemblyData&, Program&, std::string, std::string, llvm::Value**, llvm::Value**, std::string*);
int assemble_merge_types_oneway(AssemblyData&, Program&, std::string, std::string, llvm::Value**, llvm::Type*, std::string*);
bool assemble_types_mergeable(Program&, std::string, std::string);

#endif // ASSEMBLE_H_INCLUDED
