
#ifndef TYPE_H_INCLUDED
#define TYPE_H_INCLUDED

#include <vector>
#include <string>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

struct Type {
    std::string name;
    llvm::Type* type;

    Type(const Type&);
    Type(std::string, llvm::Type*);
};

#endif // TYPE_H_INCLUDED
