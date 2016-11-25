
#include <iostream>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
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

#include "../include/errors.h"
#include "../include/tokens.h"
#include "../include/strings.h"
#include "../include/program.h"
#include "../include/expression.h"

PlainExp::~PlainExp(){}

OperatorExp::OperatorExp(){
    operation = 0;
    left = NULL;
    right = NULL;
}
OperatorExp::OperatorExp(const OperatorExp& other){
    operation = other.operation;
    left = other.left->clone();
    right = other.right->clone();
}
OperatorExp::OperatorExp(uint16_t o, PlainExp* l, PlainExp* r){
    operation = o;
    left = l;
    right = r;
}
OperatorExp::~OperatorExp(){
    delete left;
    delete right;
}
llvm::Value* OperatorExp::assemble(Program& program, Function& func, AssembleContext& context){
    std::string type; // Type should have been already validated before this call
    llvm::Value *L = left->assemble(program, func, context);
    llvm::Value *R = right->assemble(program, func, context);

    if(L == NULL or R == NULL) return NULL;
    if(!left->getType(program, func, type)) return NULL;

    if(type == "int"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateAdd(L, R, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateSub(L, R, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateMul(L, R, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateSDiv(L, R, "divtmp");
        default:
            std::cerr << "Operation " << operation << " isn't implemented in OperatorExp::assemble" << std::endl;
            return NULL;
        }
    }
    else if(type == "double"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateFAdd(L, R, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateFSub(L, R, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateFMul(L, R, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateFDiv(L, R, "divtmp");
        default:
            std::cerr << "Operation " << operation << " isn't implemented in OperatorExp::assemble" << std::endl;
            return NULL;
        }
    }
    else if(type == "float"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateFAdd(L, R, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateFSub(L, R, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateFMul(L, R, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateFDiv(L, R, "divtmp");
        default:
            std::cerr << "Operation " << operation << " isn't implemented in OperatorExp::assemble" << std::endl;
            return NULL;
        }
    }
    else {
        std::cerr << "Invalid use of operator on type '" << type << "'" << std::endl;
        return NULL;
    }
}
std::string OperatorExp::toString(){
    std::string operator_str;

    switch(operation){
    case TOKENID_ADD:
        operator_str = "+";
        break;
    case TOKENID_SUBTRACT:
        operator_str = "-";
        break;
    case TOKENID_MULTIPLY:
        operator_str = "*";
        break;
    case TOKENID_DIVIDE:
        operator_str = "/";
        break;
    default:
        operator_str = "<unknown operator>";
        break;
    }

    return "(" + left->toString() + " " + operator_str + " " + right->toString() + ")";
}
PlainExp* OperatorExp::clone(){
    return new OperatorExp(*this);
}
bool OperatorExp::getType(Program& program, Function& func, std::string& type){
    // Returns true on success

    std::string lefttype;
    std::string righttype;

    if(!left->getType(program, func, lefttype)) return false;
    if(!right->getType(program, func, righttype)) return false;
    if(lefttype != righttype) return false;

    type = lefttype;
    return true;
}

IntegerExp::IntegerExp(){}
IntegerExp::IntegerExp(int32_t val){
    value = val;
}
IntegerExp::~IntegerExp(){}
llvm::Value* IntegerExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(32, value, true));
}
std::string IntegerExp::toString(){
    return to_str(value);
}
PlainExp* IntegerExp::clone(){
    return new IntegerExp(*this);
}
bool IntegerExp::getType(Program& program, Function& func, std::string& type){
    type = "int";
    return true;
}

DoubleExp::DoubleExp(){}
DoubleExp::DoubleExp(double val){
    value = val;
}
DoubleExp::~DoubleExp(){}
llvm::Value* DoubleExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantFP::get(context.context, llvm::APFloat(value));
}
std::string DoubleExp::toString(){
    return to_str(value);
}
PlainExp* DoubleExp::clone(){
    return new DoubleExp(*this);
}
bool DoubleExp::getType(Program& program, Function& func, std::string& type){
    type = "double";
    return true;
}

WordExp::WordExp(){}
WordExp::WordExp(std::string val){
    value = val;
}
WordExp::~WordExp(){}
llvm::Value* WordExp::assemble(Program& program, Function& func, AssembleContext& context){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        fail( UNDECLARED_VARIABLE(value) );
        return NULL;
    }

    return context.builder.CreateLoad(var.variable, value.c_str());
}
std::string WordExp::toString(){
    return value;
}
PlainExp* WordExp::clone(){
    return new WordExp(*this);
}
bool WordExp::getType(Program& program, Function& func, std::string& type){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        fail( UNDECLARED_VARIABLE(value) );
        return false;
    }

    type = var.type;
    return true;
}

CallExp::CallExp(){}
CallExp::CallExp(const CallExp& other){
    name = other.name;
    for(PlainExp* e : other.args) args.push_back( e->clone() );
}
CallExp::CallExp(std::string n, const std::vector<PlainExp*>& a){
    name = n;
    args = a;
}
CallExp::~CallExp(){
    for(PlainExp* e : args){
        delete e;
    }
}
llvm::Value* CallExp::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Function* target = context.module->getFunction(name);
    if (!target){
        fail( UNDECLARED_FUNC(name) );
        return NULL;
    }

    // If argument mismatch error.
    if (target->arg_size() != args.size()){
        std::cerr << "Incorrect function arguments size for for '" << name << "', returning NULL" << std::endl;
        return NULL;
    }

    std::vector<llvm::Value*> value_args;
    for(size_t i = 0, e = args.size(); i != e; ++i) {
        value_args.push_back( args[i]->assemble(program, func, context) );
        if (!value_args.back()) return NULL;
    }

    return context.builder.CreateCall(target, value_args, "calltmp");
}
std::string CallExp::toString(){
    std::string args_str;

    for(size_t i = 0; i != args.size(); i++){
        args_str += args[i]->toString();
        if(i + 1 != args.size()) args_str += ", ";
    }

    return name + "(" + args_str + ")";
}
PlainExp* CallExp::clone(){
    return new CallExp(*this);
}
bool CallExp::getType(Program& program, Function& func, std::string& type){
    External target;

    if(program.find_func(name, &target) != 0) return false;
    type = target.return_type;

    return true;
}

MemberExp::MemberExp(){}
MemberExp::MemberExp(PlainExp* v, const std::string& m){
    value = v;
    member = m;
}
MemberExp::~MemberExp(){
    //delete value;
}
llvm::Value* MemberExp::assemble(Program& program, Function& func, AssembleContext& context){
    std::string type_name;
    Structure target;
    int index;

    if(!value->getType(program, func, type_name)){
        std::cerr << "warning returning NULL" << std::endl;
        return NULL;
    }

    if(program.find_struct(type_name, &target) != 0){
        fail( UNDECLARED_STRUCT(type_name) );
        return NULL;
    }

    if(target.find_index(member, &index) != 0){
        fail( UNDECLARED_MEMBER(member, target.name) );
        return NULL;
    }

    llvm::Value* member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, true));
    llvm::Value* data = value->assemble(program, func, context);

    llvm::Type* alloc_type;
    if(program.find_type(type_name, &alloc_type) != 0){
        fail( UNDECLARED_TYPE(type_name) );
        return NULL;
    }

    llvm::AllocaInst* alloc = context.builder.CreateAlloca(alloc_type, 0, "alloctmp");
    context.builder.CreateStore(data, alloc);

    std::vector<llvm::Value*> indices(2);
    indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
    indices[1] = member_index;

    llvm::Value* member_ptr = context.builder.CreateGEP(alloc_type, alloc, indices, "memberptr");
    llvm::Value* loaded_member = context.builder.CreateLoad(member_ptr, "loadtmp");

    return loaded_member;
}
std::string MemberExp::toString(){
    return value->toString() + ":" + member;
}
PlainExp* MemberExp::clone(){
    return new MemberExp(*this);
}
bool MemberExp::getType(Program& program, Function& func, std::string& type){
    std::string type_name;
    Structure target;
    int index;

    if(!value->getType(program, func, type_name)){
        return false;
    }

    if(program.find_struct(type_name, &target) != 0){
        fail( UNDECLARED_STRUCT(type_name) );
        return false;
    }

    if(target.find_index(member, &index) != 0){
        fail( UNDECLARED_MEMBER(member, target.name) );
        return false;
    }

    type = target.members[index].type;
    return true;
}
