
#include <iostream>
#include "../include/type.h"
#include "../include/parse.h"

Type::Type(std::string n, llvm::Type* t){
    name = n;
    type = t;
}
