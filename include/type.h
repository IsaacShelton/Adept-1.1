
#ifndef TYPE_H_INCLUDED
#define TYPE_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <vector>
#include <string>

struct Type {
    std::string name;
    llvm::Type* type;

    Type(std::string, llvm::Type*);
};

#endif // TYPE_H_INCLUDED
