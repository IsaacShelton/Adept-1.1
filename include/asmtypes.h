
#ifndef ASMTYPES_H_INCLUDED
#define ASMTYPES_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

struct AssembleVariable {
    std::string name;
    std::string type;
    llvm::Value* variable;

    AssembleVariable();
    AssembleVariable(const std::string&, const std::string&, llvm::Value*);
};

struct AssembleGlobal {
    std::string name;
    llvm::Value* variable;

    AssembleGlobal();
    AssembleGlobal(const std::string&);
};

struct AssembleFunction {
    std::string mangled_name;
    std::vector<AssembleVariable> variables;
    std::vector<llvm::Value*> multi_return_pointers;

    llvm::Type* return_type;
    llvm::BasicBlock* entry;
    llvm::BasicBlock* body;
    llvm::BasicBlock* quit;
    llvm::Value* exitval;
    llvm::Value* va_args;

    AssembleFunction();
    AssembleFunction(const std::string&);
    void addVariable(const std::string&, const std::string&, llvm::Value*);
    llvm::Value* getVariableValue(const std::string&);
    AssembleVariable* findVariable(const std::string&);
};

#endif // ASMTYPES_H_INCLUDED
