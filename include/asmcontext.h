
#ifndef ASMCONTEXT_H_INCLUDED
#define ASMCONTEXT_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

// Context structure for assemble.h

struct AssembleContext {
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    AssembleContext();
};

#endif // ASMCONTEXT_H_INCLUDED
