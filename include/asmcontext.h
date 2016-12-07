
#ifndef ASMCONTEXT_H_INCLUDED
#define ASMCONTEXT_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

// Context structure for assemble.h

struct AssembleFunction {
    llvm::Type* return_type;
    llvm::BasicBlock* entry;
    llvm::BasicBlock* body;
    llvm::BasicBlock* quit;
    llvm::Value* exitval;
};

struct AssembleContext {
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    AssembleContext();
    int find_func_data(const std::string&, AssembleFunction*);
};

#endif // ASMCONTEXT_H_INCLUDED
