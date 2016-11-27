
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
#include "llvm/IR/Constants.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"

#include "../include/errors.h"
#include "../include/tokens.h"
#include "../include/strings.h"
#include "../include/program.h"
#include "../include/assemble.h"
#include "../include/validate.h"
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
    std::string lefttype;
    std::string righttype;
    llvm::Value* L = left->assemble(program, func, context);
    llvm::Value* R = right->assemble(program, func, context);

    if(L == NULL or R == NULL) return NULL;
    if(!left->getType(program, func, lefttype)) return NULL;
    if(!right->getType(program, func, righttype)) return NULL;
    if(assemble_merge_types(context, lefttype, righttype, &L, &R, &type) != 0) return NULL;

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
        std::cerr << "Invalid use of operator '" << operation << "' on type '" << type << "'" << std::endl;
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
    if( !validate_types(lefttype, righttype, &type) ) return false;
    return true;
}

ByteExp::ByteExp(){}
ByteExp::ByteExp(int8_t val){
    value = val;
}
ByteExp::~ByteExp(){}
llvm::Value* ByteExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(8, value, true));
}
std::string ByteExp::toString(){
    return to_str(value);
}
PlainExp* ByteExp::clone(){
    return new ByteExp(*this);
}
bool ByteExp::getType(Program& program, Function& func, std::string& type){
    type = "byte";
    return true;
}

ShortExp::ShortExp(){}
ShortExp::ShortExp(int16_t val){
    value = val;
}
ShortExp::~ShortExp(){}
llvm::Value* ShortExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(16, value, true));
}
std::string ShortExp::toString(){
    return to_str(value);
}
PlainExp* ShortExp::clone(){
    return new ShortExp(*this);
}
bool ShortExp::getType(Program& program, Function& func, std::string& type){
    type = "short";
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

LongExp::LongExp(){}
LongExp::LongExp(int64_t val){
    value = val;
}
LongExp::~LongExp(){}
llvm::Value* LongExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(64, value, true));
}
std::string LongExp::toString(){
    return to_str(value);
}
PlainExp* LongExp::clone(){
    return new LongExp(*this);
}
bool LongExp::getType(Program& program, Function& func, std::string& type){
    type = "long";
    return true;
}

UnsignedByteExp::UnsignedByteExp(){}
UnsignedByteExp::UnsignedByteExp(uint8_t val){
    value = val;
}
UnsignedByteExp::~UnsignedByteExp(){}
llvm::Value* UnsignedByteExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(8, value, false));
}
std::string UnsignedByteExp::toString(){
    return to_str(value);
}
PlainExp* UnsignedByteExp::clone(){
    return new UnsignedByteExp(*this);
}
bool UnsignedByteExp::getType(Program& program, Function& func, std::string& type){
    type = "ubyte";
    return true;
}

UnsignedShortExp::UnsignedShortExp(){}
UnsignedShortExp::UnsignedShortExp(uint16_t val){
    value = val;
}
UnsignedShortExp::~UnsignedShortExp(){}
llvm::Value* UnsignedShortExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(16, value, false));
}
std::string UnsignedShortExp::toString(){
    return to_str(value);
}
PlainExp* UnsignedShortExp::clone(){
    return new UnsignedShortExp(*this);
}
bool UnsignedShortExp::getType(Program& program, Function& func, std::string& type){
    type = "ushort";
    return true;
}

UnsignedIntegerExp::UnsignedIntegerExp(){}
UnsignedIntegerExp::UnsignedIntegerExp(uint32_t val){
    value = val;
}
UnsignedIntegerExp::~UnsignedIntegerExp(){}
llvm::Value* UnsignedIntegerExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(32, value, false));
}
std::string UnsignedIntegerExp::toString(){
    return to_str(value);
}
PlainExp* UnsignedIntegerExp::clone(){
    return new UnsignedIntegerExp(*this);
}
bool UnsignedIntegerExp::getType(Program& program, Function& func, std::string& type){
    type = "uint";
    return true;
}

UnsignedLongExp::UnsignedLongExp(){}
UnsignedLongExp::UnsignedLongExp(uint64_t val){
    value = val;
}
UnsignedLongExp::~UnsignedLongExp(){}
llvm::Value* UnsignedLongExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::ConstantInt::get(context.context, llvm::APInt(64, value, false));
}
std::string UnsignedLongExp::toString(){
    return to_str(value);
}
PlainExp* UnsignedLongExp::clone(){
    return new LongExp(*this);
}
bool UnsignedLongExp::getType(Program& program, Function& func, std::string& type){
    type = "ulong";
    return true;
}


FloatExp::FloatExp(){}
FloatExp::FloatExp(float val){
    value = val;
}
FloatExp::~FloatExp(){}
llvm::Value* FloatExp::assemble(Program& program, Function& func, AssembleContext& context){
    return llvm::Value* v = llvm::ConstantFP::get(llvm::Type::getFloatTy(context.context), value);
}
std::string FloatExp::toString(){
    return to_str(value);
}
PlainExp* FloatExp::clone(){
    return new FloatExp(*this);
}
bool FloatExp::getType(Program& program, Function& func, std::string& type){
    type = "float";
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

StringExp::StringExp(){}
StringExp::StringExp(const std::string& val){
    value = val;
}
StringExp::~StringExp(){}
llvm::Value* StringExp::assemble(Program& program, Function& func, AssembleContext& context){
    // Constant Definitions
    llvm::Constant* string_data = llvm::ConstantDataArray::getString(context.context, value.c_str(), true);

    llvm::GlobalVariable* global_array = new llvm::GlobalVariable(*context.module, string_data->getType(), true, llvm::GlobalValue::PrivateLinkage, 0, "$.str");
    global_array->setAlignment(1);

    std::vector<llvm::Constant*> gep_indices(2);
    llvm::ConstantInt* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, 10));
    gep_indices[0] = zero;
    gep_indices[1] = zero;
    llvm::Constant* const_string = llvm::ConstantExpr::getGetElementPtr(string_data->getType(), global_array, gep_indices);

    // Global Variable Definitions
    global_array->setInitializer(string_data);
    return const_string;
}
std::string StringExp::toString(){
    return "\"" + value + "\"";
}
PlainExp* StringExp::clone(){
    return new StringExp(*this);
}
bool StringExp::getType(Program& program, Function& func, std::string& type){
    type = "*byte";
    return true;
}

WordExp::WordExp(){}
WordExp::WordExp(const std::string& val){
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

AddrWordExp::AddrWordExp(){}
AddrWordExp::AddrWordExp(const std::string& val){
    value = val;
}
AddrWordExp::~AddrWordExp(){}
llvm::Value* AddrWordExp::assemble(Program& program, Function& func, AssembleContext& context){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        fail( UNDECLARED_VARIABLE(value) );
        return NULL;
    }

    return var.variable;
}
std::string AddrWordExp::toString(){
    return "&" + value;
}
PlainExp* AddrWordExp::clone(){
    return new AddrWordExp(*this);
}
bool AddrWordExp::getType(Program& program, Function& func, std::string& type){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        fail( UNDECLARED_VARIABLE(value) );
        return false;
    }

    type = "*" + var.type;
    return true;
}

LoadExp::LoadExp(){}
LoadExp::LoadExp(PlainExp* val){
    value = val;
}
LoadExp::LoadExp(const LoadExp& other){
    value = other.value->clone();
}
LoadExp::~LoadExp(){
    delete value;
}
llvm::Value* LoadExp::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Value* pointer = value->assemble(program, func, context);
    if(pointer == NULL) return NULL;
    if(!pointer->getType()->isPointerTy()){
        fail("Cant't load type that is not a pointer");
        return NULL;
    }

    return context.builder.CreateLoad(pointer, "loadtmp");
}
std::string LoadExp::toString(){
    return "*" + value->toString();
}
PlainExp* LoadExp::clone(){
    return new LoadExp(*this);
}
bool LoadExp::getType(Program& program, Function& func, std::string& type){
    std::string valtype;
    if(!value->getType(program, func, valtype)) return false;
    if(valtype[0] != '*'){
        fail("Cant't load type that is not a pointer");
        return false;
    }

    type = valtype.substr(1, valtype.length()-1);
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
