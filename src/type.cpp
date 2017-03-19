
/*
    This file shouldn't contain any 'type.o's
*/

#include <iostream>
#include "../include/type.h"
#include "../include/parse.h"

Type::Type(const Type& other){
    name = other.name;
    type = other.type;
}

Type::Type(std::string n, llvm::Type* t){
    name = n;
    type = t;
}
