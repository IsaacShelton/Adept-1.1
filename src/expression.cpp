
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
#include "../include/mangling.h"
#include "../include/expression.h"

PlainExp::PlainExp(){}
PlainExp::PlainExp(const PlainExp& other){
    errors = other.errors;
}
PlainExp::PlainExp(ErrorHandler& err){
    errors = err;
}
PlainExp::~PlainExp(){}
llvm::Value* PlainExp::assemble_immutable(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    // Call this method instead of 'PlainExp::assemble' to get an immutable value
    // because 'PlainExp::assemble' is not guarenteed to return an immutable result

    llvm::Value* val = this->assemble(program, func, context, expr_type);
    if(val == NULL) return NULL;

    if(this->is_mutable){
        val = context.builder.CreateLoad(val, "loadtmp");
    }

    return val;
}

OperatorExp::OperatorExp(ErrorHandler& err){
    operation = 0;
    left = NULL;
    right = NULL;
    is_mutable = false;
    errors = err;
}
OperatorExp::OperatorExp(const OperatorExp& other) : PlainExp(other) {
    operation = other.operation;
    left = other.left->clone();
    right = other.right->clone();
    is_mutable = false;
}
OperatorExp::OperatorExp(uint16_t o, PlainExp* l, PlainExp* r, ErrorHandler& err){
    operation = o;
    left = l;
    right = r;
    is_mutable = false;
    errors = err;
}
OperatorExp::~OperatorExp(){
    delete left;
    delete right;
}
llvm::Value* OperatorExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    std::string type_name;
    std::string left_typename;
    std::string right_typename;
    llvm::Value* left_value = left->assemble_immutable(program, func, context, &left_typename);
    llvm::Value* right_value = right->assemble_immutable(program, func, context, &right_typename);

    if(left_value == NULL or right_value == NULL) return NULL;
    if(assemble_merge_types(context, left_typename, right_typename, &left_value, &right_value, &type_name) != 0){
        errors.panic( INCOMPATIBLE_TYPES(left_typename, right_typename) );
        return NULL;
    }
    if(expr_type != NULL) *expr_type = type_name;
    if(type_name == "") return NULL;

    if(type_name == "int" or type_name == "uint" or type_name == "short" or type_name == "ushort"
       or type_name == "long" or type_name == "ulong" or type_name == "byte"
       or type_name == "ubyte" or type_name == "bool" or type_name == "ptr" or type_name[0] == '*'){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateAdd(left_value, right_value, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateSub(left_value, right_value, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateMul(left_value, right_value, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateSDiv(left_value, right_value, "divtmp");
        case TOKENID_MODULUS:
            return context.builder.CreateSRem(left_value, right_value, "remtmp");
        case TOKENID_EQUALITY:
            *expr_type = "bool";
            return context.builder.CreateICmpEQ(left_value, right_value, "cmptmp");
        case TOKENID_INEQUALITY:
            *expr_type = "bool";
            return context.builder.CreateICmpNE(left_value, right_value, "cmptmp");
        case TOKENID_LESS:
            *expr_type = "bool";
            return context.builder.CreateICmpSLT(left_value, right_value, "cmptmp");
        case TOKENID_GREATER:
            *expr_type = "bool";
            return context.builder.CreateICmpSGT(left_value, right_value, "cmptmp");
        case TOKENID_LESSEQ:
            *expr_type = "bool";
            return context.builder.CreateICmpSLE(left_value, right_value, "cmptmp");
        case TOKENID_GREATEREQ:
            *expr_type = "bool";
            return context.builder.CreateICmpSGE(left_value, right_value, "cmptmp");
        case TOKENID_AND:
            if(type_name != "bool"){
                errors.panic("Operands to operator 'and' must have a type of 'bool'");
                return NULL;
            }
            return context.builder.CreateAnd(left_value, right_value, "andtmp");
        case TOKENID_OR:
            if(type_name != "bool"){
                errors.panic("Operands to operator 'and' must have a type of 'bool'");
                return NULL;
            }
            return context.builder.CreateOr(left_value, right_value, "ortmp");
        default:
            std::cerr << "Unknown Operation #" << operation << " in expression" << std::endl;
            return NULL;
        }
    }
    else if(type_name == "float" or type_name == "double"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateFAdd(left_value, right_value, "addtmp");
        case TOKENID_SUBTRACT:
            return context.builder.CreateFSub(left_value, right_value, "subtmp");
        case TOKENID_MULTIPLY:
            return context.builder.CreateFMul(left_value, right_value, "multmp");
        case TOKENID_DIVIDE:
            return context.builder.CreateFDiv(left_value, right_value, "divtmp");
        case TOKENID_MODULUS:
            return context.builder.CreateFRem(left_value, right_value, "remtmp");
        case TOKENID_EQUALITY:
            *expr_type = "bool";
            return context.builder.CreateFCmpOEQ(left_value, right_value, "cmptmp");
        case TOKENID_INEQUALITY:
            *expr_type = "bool";
            return context.builder.CreateFCmpONE(left_value, right_value, "cmptmp");
        default:
            std::cerr << "Unknown Operation #" << operation << " in expression" << std::endl;
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
    case TOKENID_EQUALITY:
        operator_str = "==";
        break;
    case TOKENID_INEQUALITY:
        operator_str = "!=";
    case TOKENID_AND:
        operator_str = "and";
        break;
    case TOKENID_OR:
        operator_str = "or";
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

BoolExp::BoolExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
BoolExp::BoolExp(bool val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
BoolExp::BoolExp(const BoolExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
}
BoolExp::~BoolExp(){}
llvm::Value* BoolExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "bool";
    return llvm::ConstantInt::get(context.context, llvm::APInt(1, value, true));
}
std::string BoolExp::toString(){
    return (value ? "true" : "false");
}
PlainExp* BoolExp::clone(){
    return new BoolExp(*this);
}

ByteExp::ByteExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
ByteExp::ByteExp(int8_t val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
ByteExp::ByteExp(const ByteExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

ShortExp::ShortExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
ShortExp::ShortExp(int16_t val, ErrorHandler& err){
    is_mutable = false;
    value = val;
    errors = err;
}
ShortExp::ShortExp(const ShortExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

IntegerExp::IntegerExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
IntegerExp::IntegerExp(int32_t val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
IntegerExp::IntegerExp(const IntegerExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

LongExp::LongExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
LongExp::LongExp(int64_t val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
LongExp::LongExp(const LongExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

UnsignedByteExp::UnsignedByteExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
UnsignedByteExp::UnsignedByteExp(uint8_t val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
UnsignedByteExp::UnsignedByteExp(const UnsignedByteExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

UnsignedShortExp::UnsignedShortExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
UnsignedShortExp::UnsignedShortExp(uint16_t val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
UnsignedShortExp::UnsignedShortExp(const UnsignedShortExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

UnsignedIntegerExp::UnsignedIntegerExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
UnsignedIntegerExp::UnsignedIntegerExp(uint32_t val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
UnsignedIntegerExp::UnsignedIntegerExp(const UnsignedIntegerExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

UnsignedLongExp::UnsignedLongExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
UnsignedLongExp::UnsignedLongExp(uint64_t val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
UnsignedLongExp::UnsignedLongExp(const UnsignedLongExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

FloatExp::FloatExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
FloatExp::FloatExp(float val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
FloatExp::FloatExp(const FloatExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

DoubleExp::DoubleExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
DoubleExp::DoubleExp(double val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
DoubleExp::DoubleExp(const DoubleExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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

StringExp::StringExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
StringExp::StringExp(const std::string& val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
StringExp::StringExp(const StringExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
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
    if(expr_type != NULL) *expr_type = "*ubyte";
    return const_string;
}
std::string StringExp::toString(){
    return "\"" + value + "\"";
}
PlainExp* StringExp::clone(){
    return new StringExp(*this);
}

WordExp::WordExp(ErrorHandler& err){
    is_mutable = true;
    errors = err;
}
WordExp::WordExp(const std::string& val, ErrorHandler& err){
    value = val;
    is_mutable = true;
    errors = err;
}
WordExp::WordExp(const WordExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = true;
}
WordExp::~WordExp(){}
llvm::Value* WordExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        errors.panic( UNDECLARED_VARIABLE(value) );
        return NULL;
    }

    if(expr_type != NULL) *expr_type = var.type;
    return var.variable;
}
std::string WordExp::toString(){
    return value;
}
PlainExp* WordExp::clone(){
    return new WordExp(*this);
}

AddrWordExp::AddrWordExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
AddrWordExp::AddrWordExp(const std::string& val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
AddrWordExp::AddrWordExp(const AddrWordExp& other) : PlainExp(other) {
    value = other.value;
    is_mutable = false;
}
AddrWordExp::~AddrWordExp(){}
llvm::Value* AddrWordExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    Variable var;

    if(func.find_variable(value, &var) != 0){
        errors.panic( UNDECLARED_VARIABLE(value) );
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

LoadExp::LoadExp(ErrorHandler& err){
    is_mutable = true;
    errors = err;
}
LoadExp::LoadExp(PlainExp* val, ErrorHandler& err){
    value = val;
    is_mutable = true;
    errors = err;
}
LoadExp::LoadExp(const LoadExp& other) : PlainExp(other) {
    value = other.value->clone();
    is_mutable = true;
}
LoadExp::~LoadExp(){
    delete value;
}
llvm::Value* LoadExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    std::string pointer_typename;
    llvm::Value* pointer_value = value->assemble(program, func, context, &pointer_typename);
    if(pointer_value == NULL) return NULL;

    if(pointer_typename[0] != '*' or !pointer_value->getType()->isPointerTy()){
        errors.panic("Can't dereference non-pointer type '" + pointer_typename + "'");
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

IndexLoadExp::IndexLoadExp(ErrorHandler& err){
    is_mutable = true;
    errors = err;
}
IndexLoadExp::IndexLoadExp(PlainExp* val, PlainExp* idx, ErrorHandler& err){
    value = val;
    index = idx;
    is_mutable = true;
    errors = err;
}
IndexLoadExp::IndexLoadExp(const IndexLoadExp& other) : PlainExp(other) {
    value = other.value->clone();
    index = other.index->clone();
    is_mutable = true;
}
IndexLoadExp::~IndexLoadExp(){
    delete value;
}
llvm::Value* IndexLoadExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    std::string pointer_typename;
    llvm::Value* pointer_value = value->assemble(program, func, context, &pointer_typename);
    if(pointer_value == NULL) return NULL;

    if(!value->is_mutable){
        errors.panic("Can't use [] operator on immutable expression");
        return NULL;
    }

    if(pointer_typename[0] != '*' or !pointer_value->getType()->isPointerTy()){
        errors.panic("Can't dereference non-pointer type '" + pointer_typename + "'");
        return NULL;
    }

    std::string index_typename;
    llvm::Value* index_value = index->assemble_immutable(program, func, context, &index_typename);
    if(index_value == NULL) return NULL;

    if(index_typename != "int"){
        errors.panic("Expected 'int' type when using []");
        return NULL;
    }

    if(expr_type != NULL) *expr_type = pointer_typename.substr(1, pointer_typename.length()-1);

    std::vector<llvm::Value*> indices(1);
    indices[0] = index_value;

    pointer_value = context.builder.CreateInBoundsGEP(pointer_value, indices, "memberptr");
    return context.builder.CreateLoad(pointer_value, "loadtmp");
}
std::string IndexLoadExp::toString(){
    return value->toString() + "[" + index->toString() + "]";
}
PlainExp* IndexLoadExp::clone(){
    return new IndexLoadExp(*this);
}

CallExp::CallExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
CallExp::CallExp(const CallExp& other) : PlainExp(other) {
    name = other.name;
    for(PlainExp* e : other.args) args.push_back( e->clone() );
    is_mutable = false;
}
CallExp::CallExp(std::string n, const std::vector<PlainExp*>& a, ErrorHandler& err){
    name = n;
    args = a;
    is_mutable = false;
    errors = err;
}
CallExp::~CallExp(){
    for(PlainExp* e : args){
        delete e;
    }
}
llvm::Value* CallExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    External func_data;

    llvm::Value* expr_value;
    std::string expr_typename;
    llvm::Type* expected_arg_type;
    std::vector<llvm::Value*> argument_values;
    std::vector<std::string> argument_types;
    std::vector<llvm::Type*> argument_llvm_types;

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble(program, func, context, &expr_typename);
        if(expr_value == NULL) return NULL;

        if(program.find_type(expr_typename, &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(expr_typename) );
            return NULL;
        }

        argument_values.push_back(expr_value);
        argument_types.push_back(expr_typename);
        argument_llvm_types.push_back(expected_arg_type);
    }

    if(program.find_func(name, argument_types, &func_data) != 0){
        errors.panic( UNDECLARED_FUNC(name) );
        return NULL;
    }

    std::string final_name = (func_data.is_mangled) ? mangle(name, func_data.arguments) : name;
    llvm::Function* target = context.module->getFunction(final_name);
    if (!target){
        errors.panic( UNDECLARED_FUNC(name) );
        return NULL;
    }
    assert(func_data.arguments.size() == target->arg_size());

    if (target->arg_size() != args.size()){
        // NOTE: This error should never appear
        errors.panic("Incorrect function argument count for function '" + name + "'");
        return NULL;
    }

    for(size_t z = 0; z != argument_values.size(); z++){
        if(assemble_merge_types_oneway(context, argument_types[z], func_data.arguments[z], &argument_values[z], argument_llvm_types[z], NULL) != 0){
            // NOTE: This error should never occur
            errors.panic("Incorrect type for argument " + to_str(z+1) + " of function '" + name + "'\n    Definition: " + func_data.toString() +
                 "\n    Expected type '" + func_data.arguments[z] + "' but received type '" + argument_types[z] + "'");
            return NULL;
        }
    }

    *expr_type = func_data.return_type;
    return context.builder.CreateCall(target, argument_values, "calltmp");
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

MemberExp::MemberExp(ErrorHandler& err){
    errors = err;
    is_mutable = true;
}
MemberExp::MemberExp(PlainExp* v, const std::string& m, ErrorHandler& err){
    value = v;
    member = m;
    is_mutable = true;
    errors = err;
}
MemberExp::MemberExp(const MemberExp& other) : PlainExp(other) {
    value = other.value;
    member = other.member;
    is_mutable = true;
}
MemberExp::~MemberExp(){
    delete value;
}
llvm::Value* MemberExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    int index;
    Structure target;
    std::string type_name;
    llvm::Value* member_index;
    llvm::Value* data = value->assemble(program, func, context, &type_name);
    if(data == NULL) return NULL;

    if(!value->is_mutable){
        errors.panic("Can't use [] operator on immutable expression");
        return NULL;
    }

    if(type_name == ""){
        errors.panic("Undeclared type ''");
        return NULL;
    }
    else if(type_name[0] == '*'){
        data = context.builder.CreateLoad(data, "loadtmp");
        type_name = type_name.substr(1, type_name.length()-1);
    }

    if(program.find_struct(type_name, &target) != 0){
        errors.panic( UNDECLARED_STRUCT(type_name) );
        return NULL;
    }

    if(target.find_index(member, &index) != 0){
        errors.panic( UNDECLARED_MEMBER(member, target.name) );
        return NULL;
    }

    llvm::Type* alloc_type;
    if(program.find_type(type_name, &alloc_type) != 0){
        errors.panic( UNDECLARED_TYPE(type_name) );
        return NULL;
    }

    std::vector<llvm::Value*> indices(2);
    member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, true));
    indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
    indices[1] = member_index;

    llvm::Value* member_ptr = context.builder.CreateGEP(alloc_type, data, indices, "memberptr");
    if(expr_type != NULL) *expr_type = target.members[index].type;
    return member_ptr;
}
std::string MemberExp::toString(){
    return value->toString() + ":" + member;
}
PlainExp* MemberExp::clone(){
    return new MemberExp(*this);
}

NullExp::NullExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
NullExp::NullExp(const NullExp& other) : PlainExp(other) {
    is_mutable = false;
}
NullExp::~NullExp(){}
llvm::Value* NullExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    *expr_type = "ptr";
    return llvm::ConstantPointerNull::get( llvm::Type::getInt8PtrTy(context.context) );
}
std::string NullExp::toString(){
    return "null";
}
PlainExp* NullExp::clone(){
    return new NullExp(errors);
}

NotExp::NotExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
NotExp::NotExp(PlainExp* val, ErrorHandler& err){
    value = val;
    is_mutable = false;
    errors = err;
}
NotExp::NotExp(const NotExp& other) : PlainExp(other) {
    value = other.value->clone();
    is_mutable = false;
}
NotExp::~NotExp(){
    delete value;
}
llvm::Value* NotExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;
    *expr_type = "bool";

    if(type_name == "bool"){
        return context.builder.CreateNot(llvm_value, "nottmp");
    }
    else if(type_name == "byte"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else if(type_name == "ubyte"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else if(type_name == "short"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else if(type_name == "ushort"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else if(type_name == "int"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else if(type_name == "uint"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else if(type_name == "long"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else if(type_name == "ulong"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero, "cmptmp");
        return llvm_value;
    }
    else {
        errors.panic("Can't use 'not' operator on type '" + type_name + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
std::string NotExp::toString(){
    return "!" + value->toString();
}
PlainExp* NotExp::clone(){
    return new NotExp(value->clone(), errors);
}

CastExp::CastExp(ErrorHandler& err){
    is_mutable = false;
    errors = err;
}
CastExp::CastExp(PlainExp* val, std::string tgt_type, ErrorHandler& err){
    value = val;
    target_typename = tgt_type;
    is_mutable = false;
    errors = err;
}
CastExp::~CastExp(){
    delete value;
}
llvm::Value* CastExp::assemble(Program& program, Function& func, AssembleContext& context, std::string* expr_type){
    *expr_type = target_typename;

    if(target_typename == "bool"){
        return this->cast_to_bool(program, func, context);
    }
    else if(target_typename == "byte" or target_typename == "ubyte"){
        return this->cast_to_byte(program, func, context);
    }
    else if(target_typename == "short" or target_typename == "ushort"){
        return this->cast_to_short(program, func, context);
    }
    else if(target_typename == "int" or target_typename == "uint"){
        return this->cast_to_int(program, func, context);
    }
    else if(target_typename == "long" or target_typename == "ulong"){
        return this->cast_to_long(program, func, context);
    }

    errors.panic("Can't cast value to type '" + target_typename + "'");
    return NULL;
}
std::string CastExp::toString(){
    return "cast " + target_typename + "(" + value->toString() + ")";
}
PlainExp* CastExp::clone(){
    return new CastExp(value->clone(), target_typename, errors);
}
llvm::Value* CastExp::cast_to_bool(Program& program, Function& func, AssembleContext& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    if(type_name == "bool"){
        return llvm_value;
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty(), "trunctmp");
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty(), "trunctmp");
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty(), "trunctmp");
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty(), "trunctmp");
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_byte(Program& program, Function& func, AssembleContext& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt8Ty(), "zexttmp");
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return llvm_value;
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt8Ty(), "trunctmp");
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt8Ty(), "trunctmp");
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt8Ty(), "trunctmp");
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_short(Program& program, Function& func, AssembleContext& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt16Ty(), "zexttmp");
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt16Ty(), "zexttmp");
    }
    else if(type_name == "ushort" or type_name == "short"){
        return llvm_value;
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt16Ty(), "trunctmp");
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt16Ty(), "trunctmp");
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_int(Program& program, Function& func, AssembleContext& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt32Ty(), "zexttmp");
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt32Ty(), "zexttmp");
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt32Ty(), "zexttmp");
    }
    else if(type_name == "uint" or type_name == "int"){
        return llvm_value;
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt32Ty(), "trunctmp");
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_long(Program& program, Function& func, AssembleContext& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty(), "zexttmp");
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty(), "zexttmp");
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty(), "zexttmp");
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty(), "zexttmp");
    }
    else if(type_name == "ulong" or type_name == "long"){
        return llvm_value;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
