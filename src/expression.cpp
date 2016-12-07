
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
llvm::Value* OperatorExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    std::string type_name;
    std::string left_typename;
    std::string right_typename;
    llvm::Value* left_value = left->assemble(program, func, context, &left_typename);
    llvm::Value* right_value = right->assemble(program, func, context, &right_typename);

    if(left_value == NULL or right_value == NULL) return NULL;
    if(assemble_merge_types(context, left_typename, right_typename, &left_value, &right_value, &type_name) != 0){
        fail( INCOMPATIBLE_TYPES(left_typename, right_typename) );
        return NULL;
    }
    if(expr_type != NULL) *expr_type = type_name;

    if(type_name == "int"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateAdd(left_value, right_value, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateSub(left_value, right_value, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateMul(left_value, right_value, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateSDiv(left_value, right_value, "divtmp");
        default:
            std::cerr << "Operation " << operation << " isn't implemented in OperatorExp::assemble" << std::endl;
            return NULL;
        }
    }
    else if(type_name == "double"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateFAdd(left_value, right_value, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateFSub(left_value, right_value, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateFMul(left_value, right_value, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateFDiv(left_value, right_value, "divtmp");
        default:
            std::cerr << "Operation " << operation << " isn't implemented in OperatorExp::assemble" << std::endl;
            return NULL;
        }
    }
    else if(type_name == "float"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateFAdd(left_value, right_value, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateFSub(left_value, right_value, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateFMul(left_value, right_value, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateFDiv(left_value, right_value, "divtmp");
        default:
            std::cerr << "Operation " << operation << " isn't implemented in OperatorExp::assemble" << std::endl;
            return NULL;
        }
    }
    else {
        std::cerr << "Invalid use of operator '" << operation << "' on type '" << type_name << "'" << std::endl;
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

ByteExp::ByteExp(){}
ByteExp::ByteExp(int8_t val){
    value = val;
}
ByteExp::~ByteExp(){}
llvm::Value* ByteExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "byte";
    return llvm::ConstantInt::get(context.context, llvm::APInt(8, value, true));
}
std::string ByteExp::toString(){
    return to_str(value) + "sb";
}
PlainExp* ByteExp::clone(){
    return new ByteExp(*this);
}

ShortExp::ShortExp(){}
ShortExp::ShortExp(int16_t val){
    value = val;
}
ShortExp::~ShortExp(){}
llvm::Value* ShortExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "short";
    return llvm::ConstantInt::get(context.context, llvm::APInt(16, value, true));
}
std::string ShortExp::toString(){
    return to_str(value) + "ss";
}
PlainExp* ShortExp::clone(){
    return new ShortExp(*this);
}

IntegerExp::IntegerExp(){}
IntegerExp::IntegerExp(int32_t val){
    value = val;
}
IntegerExp::~IntegerExp(){}
llvm::Value* IntegerExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "int";
    return llvm::ConstantInt::get(context.context, llvm::APInt(32, value, true));
}
std::string IntegerExp::toString(){
    return to_str(value) + "si";
}
PlainExp* IntegerExp::clone(){
    return new IntegerExp(*this);
}

LongExp::LongExp(){}
LongExp::LongExp(int64_t val){
    value = val;
}
LongExp::~LongExp(){}
llvm::Value* LongExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "long";
    return llvm::ConstantInt::get(context.context, llvm::APInt(64, value, true));
}
std::string LongExp::toString(){
    return to_str(value) + "sl";
}
PlainExp* LongExp::clone(){
    return new LongExp(*this);
}

UnsignedByteExp::UnsignedByteExp(){}
UnsignedByteExp::UnsignedByteExp(uint8_t val){
    value = val;
}
UnsignedByteExp::~UnsignedByteExp(){}
llvm::Value* UnsignedByteExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "ubyte";
    return llvm::ConstantInt::get(context.context, llvm::APInt(8, value, false));
}
std::string UnsignedByteExp::toString(){
    return to_str(value) + "ub";
}
PlainExp* UnsignedByteExp::clone(){
    return new UnsignedByteExp(*this);
}

UnsignedShortExp::UnsignedShortExp(){}
UnsignedShortExp::UnsignedShortExp(uint16_t val){
    value = val;
}
UnsignedShortExp::~UnsignedShortExp(){}
llvm::Value* UnsignedShortExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "ushort";
    return llvm::ConstantInt::get(context.context, llvm::APInt(16, value, false));
}
std::string UnsignedShortExp::toString(){
    return to_str(value) + "us";
}
PlainExp* UnsignedShortExp::clone(){
    return new UnsignedShortExp(*this);
}

UnsignedIntegerExp::UnsignedIntegerExp(){}
UnsignedIntegerExp::UnsignedIntegerExp(uint32_t val){
    value = val;
}
UnsignedIntegerExp::~UnsignedIntegerExp(){}
llvm::Value* UnsignedIntegerExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "uint";
    return llvm::ConstantInt::get(context.context, llvm::APInt(32, (uint64_t) value, false));
}
std::string UnsignedIntegerExp::toString(){
    return to_str(value) + "ui";
}
PlainExp* UnsignedIntegerExp::clone(){
    return new UnsignedIntegerExp(*this);
}

UnsignedLongExp::UnsignedLongExp(){}
UnsignedLongExp::UnsignedLongExp(uint64_t val){
    value = val;
}
UnsignedLongExp::~UnsignedLongExp(){}
llvm::Value* UnsignedLongExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "ulong";
    return llvm::ConstantInt::get(context.context, llvm::APInt(64, value, false));
}
std::string UnsignedLongExp::toString(){
    return to_str(value) + "ul";
}
PlainExp* UnsignedLongExp::clone(){
    return new UnsignedLongExp(*this);
}


FloatExp::FloatExp(){}
FloatExp::FloatExp(float val){
    value = val;
}
FloatExp::~FloatExp(){}
llvm::Value* FloatExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "float";
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(context.context), value);
}
std::string FloatExp::toString(){
    return to_str(value) + "f";
}
PlainExp* FloatExp::clone(){
    return new FloatExp(*this);
}

DoubleExp::DoubleExp(){}
DoubleExp::DoubleExp(double val){
    value = val;
}
DoubleExp::~DoubleExp(){}
llvm::Value* DoubleExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "double";
    return llvm::ConstantFP::get(context.context, llvm::APFloat(value));
}
std::string DoubleExp::toString(){
    return to_str(value) + "d";
}
PlainExp* DoubleExp::clone(){
    return new DoubleExp(*this);
}

StringExp::StringExp(){}
StringExp::StringExp(const std::string& val){
    value = val;
}
StringExp::~StringExp(){}
llvm::Value* StringExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
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
    if(expr_type != NULL) *expr_type = "*byte";
    return const_string;
}
std::string StringExp::toString(){
    return "\"" + value + "\"";
}
PlainExp* StringExp::clone(){
    return new StringExp(*this);
}

WordExp::WordExp(){}
WordExp::WordExp(const std::string& val){
    value = val;
}
WordExp::~WordExp(){}
llvm::Value* WordExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        fail( UNDECLARED_VARIABLE(value) );
        return NULL;
    }

    if(expr_type != NULL) *expr_type = var.type;
    return context.builder.CreateLoad(var.variable, value.c_str());
}
std::string WordExp::toString(){
    return value;
}
PlainExp* WordExp::clone(){
    return new WordExp(*this);
}

AddrWordExp::AddrWordExp(){}
AddrWordExp::AddrWordExp(const std::string& val){
    value = val;
}
AddrWordExp::~AddrWordExp(){}
llvm::Value* AddrWordExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        fail( UNDECLARED_VARIABLE(value) );
        return NULL;
    }

    if(expr_type != NULL) *expr_type = "*" + var.type;
    return var.variable;
}
std::string AddrWordExp::toString(){
    return "&" + value;
}
PlainExp* AddrWordExp::clone(){
    return new AddrWordExp(*this);
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
llvm::Value* LoadExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    std::string pointer_typename;
    llvm::Value* pointer_value = value->assemble(program, func, context, &pointer_typename);
    if(pointer_value == NULL) return NULL;

    if(pointer_typename[0] != '*' or !pointer_value->getType()->isPointerTy()){
        fail("Can't dereference non-pointer type '" + pointer_typename + "'");
        return NULL;
    }

    if(expr_type != NULL) *expr_type = pointer_typename.substr(1, pointer_typename.length()-1);
    return context.builder.CreateLoad(pointer_value, "loadtmp");
}
std::string LoadExp::toString(){
    return "*" + value->toString();
}
PlainExp* LoadExp::clone(){
    return new LoadExp(*this);
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
llvm::Value* CallExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    llvm::Function* target = context.module->getFunction(name);
    External func_data;

    if (!target){
        fail( UNDECLARED_FUNC(name) );
        return NULL;
    }

    if(program.find_func(name, &func_data) != 0) return NULL;
    assert(func_data.arguments.size() == target->arg_size());

    // If argument mismatch error.
    if (target->arg_size() != args.size()){
        std::cerr << "Incorrect function arguments size for for '" << name << "', returning NULL" << std::endl;
        return NULL;
    }

    llvm::Value* expr_value;
    std::string expr_typename;
    llvm::Type* expected_arg_type;
    std::vector<llvm::Value*> value_args;

    for(size_t i = 0, e = args.size(); i != e; ++i) {
        expr_value = args[i]->assemble(program, func, context, &expr_typename);
        if(expr_value == NULL) return NULL;

        if(program.find_type(func_data.arguments[i], &expected_arg_type) != 0){
            fail( UNDECLARED_TYPE(func_data.arguments[i]) );
            return NULL;
        }

        if(assemble_merge_types_oneway(context, expr_typename, func_data.arguments[i], &expr_value, expected_arg_type, NULL) != 0){
            fail( INCOMPATIBLE_TYPES(expr_typename, func_data.arguments[i]) );
            return NULL;
        }

        value_args.push_back(expr_value);
    }

    if(expr_type != NULL) *expr_type = func_data.return_type;
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

MemberExp::MemberExp(){}
MemberExp::MemberExp(PlainExp* v, const std::string& m){
    value = v;
    member = m;
}
MemberExp::~MemberExp(){
    //delete value;
}
llvm::Value* MemberExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    int index;
    Structure target;
    std::string type_name;
    llvm::Value* member_index;
    llvm::Value* data = value->assemble(program, func, context, &type_name);
    if(data == NULL) return NULL;

    if(program.find_struct(type_name, &target) != 0){
        fail( UNDECLARED_STRUCT(type_name) );
        return NULL;
    }

    if(target.find_index(member, &index) != 0){
        fail( UNDECLARED_MEMBER(member, target.name) );
        return NULL;
    }

    llvm::Type* alloc_type;
    if(program.find_type(type_name, &alloc_type) != 0){
        fail( UNDECLARED_TYPE(type_name) );
        return NULL;
    }

    // TODO: Only allocate for non-variables
    llvm::AllocaInst* alloc = context.builder.CreateAlloca(alloc_type, 0, "alloctmp");
    context.builder.CreateStore(data, alloc);

    std::vector<llvm::Value*> indices(2);
    member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, true));
    indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
    indices[1] = member_index;

    llvm::Value* member_ptr = context.builder.CreateGEP(alloc_type, alloc, indices, "memberptr");
    llvm::Value* loaded_member = context.builder.CreateLoad(member_ptr, "loadtmp");

    if(expr_type != NULL) *expr_type = target.members[index].type;
    return loaded_member;
}
std::string MemberExp::toString(){
    return value->toString() + ":" + member;
}
PlainExp* MemberExp::clone(){
    return new MemberExp(*this);
}
