
#ifndef ASMDATA_H_INCLUDED
#define ASMDATA_H_INCLUDED

#include <string>
#include <vector>
#include "asmtypes.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

struct AssemblyData {
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::vector<AssembleType> types;
    std::vector<AssembleFunction> functions;
    std::vector<AssembleGlobal> globals;
    bool assembling_constant_expression;
    int constant_expression_depth;
    llvm::BasicBlock* break_point;
    llvm::BasicBlock* continue_point;

    // Assembly Context
    AssembleFunction* current_function;

    AssemblyData();
    AssembleFunction* getFunction(const std::string&);
    int addVariableToFunction(const std::string&, const std::string&, const std::string&, llvm::Value*);
    void addGlobal(const std::string&);
    AssembleGlobal* findGlobal(const std::string&);
};

#endif // ASMDATA_H_INCLUDED
