
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

int build(AssemblyData&, Configuration&, Program&);
int build_program(AssemblyData&, Configuration&, Program&, ErrorHandler&);
int build_buildscript(AssemblyData&, Configuration&, Program&, ErrorHandler&);
int build_dependencies(AssemblyData*, Program*, ModuleBuildOptions*, ErrorHandler*, ModuleDependency**, size_t);

int assemble(AssemblyData&, Configuration&, Program&, ErrorHandler& errors);

int assemble_globals_batch(AssemblyData* context, const Configuration* config, const Program* program, Global* globals, size_t globals_count);
int assemble_externals_batch(AssemblyData* context, const Configuration* config, const Program* program, External* globals, size_t globals_count);

int assemble_class_skeletons(AssemblyData&, Configuration&, Program&);
int assemble_class_bodies(AssemblyData&, Configuration&, Program&);
int assemble_function_skeletons(AssemblyData&, Configuration&, Program&);
int assemble_function_bodies(AssemblyData&, Configuration&, Program&);
int assemble_method(AssemblyData&, Configuration&, Program&, Class&, Function&, bool);
int assemble_method_body(AssemblyData&, Configuration&, Program&, Class&, Function&);

void assemble_merge_conditional_types(AssemblyData&, Program&, std::string&, llvm::Value**);
int assemble_merge_types(AssemblyData&, Program&, std::string, std::string, llvm::Value**, llvm::Value**, std::string*);
int assemble_merge_types_oneway(AssemblyData&, Program&, std::string, std::string, llvm::Value**, llvm::Type*, std::string*);
bool assemble_types_mergeable(Program&, std::string, std::string);

#endif // ASSEMBLE_H_INCLUDED
