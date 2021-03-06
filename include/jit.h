
#ifndef JIT_H_INCLUDED
#define JIT_H_INCLUDED

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"

#include <string>
#include "config.h"
#include "program.h"
#include "asmdata.h"

void jit_init();
int jit_run(Config* config, AssemblyData&, const std::string&, std::vector<ModuleDependency>& dependencies, std::string&, std::vector<llvm::GenericValue> args = std::vector<llvm::GenericValue>(0));
int jit_run_main(Config* config, AssemblyData&, std::vector<ModuleDependency>& dependencies, int&);

#endif // JIT_H_INCLUDED
