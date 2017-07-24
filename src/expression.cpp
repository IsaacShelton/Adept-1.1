
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

#include "../include/die.h"
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
inline llvm::Value* PlainExp::assemble_immutable(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    // Call this method instead of 'PlainExp::assemble' to get an immutable value
    // because 'PlainExp::assemble' is not guarenteed to return an immutable result

    llvm::Value* val = this->assemble(program, func, context, expr_type);
    if(val == NULL) return NULL;

    if(this->flags & EXP_MUTABLE){
        return context.builder.CreateLoad(val);
    }


    return val;
}

OperatorExp::OperatorExp(ErrorHandler& err){
    operation = 0;
    left = NULL;
    right = NULL;
    flags = 0x00;
    errors = err;
}
OperatorExp::OperatorExp(const OperatorExp& other) : PlainExp(other) {
    operation = other.operation;
    left = other.left->clone();
    right = other.right->clone();
    flags = 0x00;
}
OperatorExp::OperatorExp(uint16_t o, PlainExp* l, PlainExp* r, ErrorHandler& err){
    operation = o;
    left = l;
    right = r;
    flags = 0x00;
    errors = err;
}
OperatorExp::~OperatorExp(){
    delete left;
    delete right;
}
llvm::Value* OperatorExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    std::string type_name;
    std::string left_typename;
    std::string right_typename;
    llvm::Value* left_value = left->assemble_immutable(program, func, context, &left_typename);
    llvm::Value* right_value = right->assemble_immutable(program, func, context, &right_typename);

    if(left_value == NULL or right_value == NULL) return NULL;
    if(assemble_merge_types(context, program, left_typename, right_typename, &left_value, &right_value, &type_name) != 0){
        errors.panic( INCOMPATIBLE_TYPES(left_typename, right_typename) );
        return NULL;
    }
    if(expr_type != NULL) *expr_type = type_name;
    if(type_name == "") return NULL;

    if(type_name[0] == 'u' && (type_name == "uint" or type_name == "ushort" or type_name == "ulong" or type_name == "ubyte")){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateAdd(left_value, right_value);
        case TOKENID_SUBTRACT:
            return context.builder.CreateSub(left_value, right_value);
        case TOKENID_MULTIPLY:
            return context.builder.CreateMul(left_value, right_value);
        case TOKENID_DIVIDE:
            return context.builder.CreateUDiv(left_value, right_value);
        case TOKENID_MODULUS:
            return context.builder.CreateURem(left_value, right_value);
        case TOKENID_EQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpEQ(left_value, right_value);
        case TOKENID_INEQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpNE(left_value, right_value);
        case TOKENID_LESS:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpULT(left_value, right_value);
        case TOKENID_GREATER:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpUGT(left_value, right_value);
        case TOKENID_LESSEQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpULE(left_value, right_value);
        case TOKENID_GREATEREQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpUGE(left_value, right_value);
        case TOKENID_AND:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        case TOKENID_OR:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        default:
            errors.panic("Unexpected operator '" + get_tokenid_syntax(operation) + "' in expression");
            return NULL;
        }
    }
    else if(type_name == "bool"){
        switch (operation) {
        case TOKENID_ADD:
            errors.panic("The addition operator '+' can't be used on booleans");
            return NULL;
        case TOKENID_SUBTRACT:
            errors.panic("The subtraction operator '-' can't be used on booleans");
            return NULL;
        case TOKENID_MULTIPLY:
            errors.panic("The multiplication operator '*' can't be used on booleans");
            return NULL;
        case TOKENID_DIVIDE:
            errors.panic("The division operator '/' can't be used on booleans");
            return NULL;
        case TOKENID_MODULUS:
            errors.panic("The modulus operator '%' can't be used on booleans");
            return NULL;
        case TOKENID_EQUALITY:
            return context.builder.CreateICmpEQ(left_value, right_value);
        case TOKENID_INEQUALITY:
            return context.builder.CreateICmpNE(left_value, right_value);
        case TOKENID_LESS:
            return context.builder.CreateICmpSLT(left_value, right_value);
        case TOKENID_GREATER:
            return context.builder.CreateICmpSGT(left_value, right_value);
        case TOKENID_LESSEQ:
            return context.builder.CreateICmpSLE(left_value, right_value);
        case TOKENID_GREATEREQ:
            return context.builder.CreateICmpSGE(left_value, right_value);
        case TOKENID_AND:
            return context.builder.CreateAnd(left_value, right_value);
        case TOKENID_OR:
            return context.builder.CreateOr(left_value, right_value);
        default:
            errors.panic("Unexpected operator '" + get_tokenid_syntax(operation) + "' in expression");
            return NULL;
        }
    }
    else if(type_name == "int" or type_name == "short" or type_name == "long" or type_name == "byte"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateAdd(left_value, right_value);
        case TOKENID_SUBTRACT:
            return context.builder.CreateSub(left_value, right_value);
        case TOKENID_MULTIPLY:
            return context.builder.CreateMul(left_value, right_value);
        case TOKENID_DIVIDE:
            return context.builder.CreateSDiv(left_value, right_value);
        case TOKENID_MODULUS:
            return context.builder.CreateSRem(left_value, right_value);
        case TOKENID_EQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpEQ(left_value, right_value);
        case TOKENID_INEQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpNE(left_value, right_value);
        case TOKENID_LESS:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSLT(left_value, right_value);
        case TOKENID_GREATER:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSGT(left_value, right_value);
        case TOKENID_LESSEQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSLE(left_value, right_value);
        case TOKENID_GREATEREQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSGE(left_value, right_value);
        case TOKENID_AND:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        case TOKENID_OR:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        default:
            errors.panic("Unexpected operator '" + get_tokenid_syntax(operation) + "' in expression");
            return NULL;
        }
    }
    else if(type_name == "float" or type_name == "double" or type_name == "half"){
        switch (operation) {
        case TOKENID_ADD:
            return context.builder.CreateFAdd(left_value, right_value);
        case TOKENID_SUBTRACT:
            return context.builder.CreateFSub(left_value, right_value);
        case TOKENID_MULTIPLY:
            return context.builder.CreateFMul(left_value, right_value);
        case TOKENID_DIVIDE:
            return context.builder.CreateFDiv(left_value, right_value);
        case TOKENID_MODULUS:
            return context.builder.CreateFRem(left_value, right_value);
        case TOKENID_EQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateFCmpOEQ(left_value, right_value);
        case TOKENID_INEQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateFCmpONE(left_value, right_value);
        case TOKENID_LESS:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateFCmpOLT(left_value, right_value);
        case TOKENID_GREATER:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateFCmpOGT(left_value, right_value);
        case TOKENID_LESSEQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateFCmpOLE(left_value, right_value);
        case TOKENID_GREATEREQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateFCmpOGE(left_value, right_value);
        case TOKENID_AND:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        case TOKENID_OR:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        default:
            errors.panic("Unexpected operator '" + get_tokenid_syntax(operation) + "' in expression");
            return NULL;
        }
    }
    else if(type_name == "ptr" or type_name[0] == '*'){
        if(expr_type != NULL) *expr_type = "ptr";

        switch (operation) {
        case TOKENID_ADD:
            {
                llvm::Value* left_int = context.builder.CreatePtrToInt(left_value, context.builder.getInt64Ty());
                llvm::Value* right_int = context.builder.CreatePtrToInt(right_value, context.builder.getInt64Ty());
                llvm::Value* value = context.builder.CreateAdd(left_int, right_int);
                return context.builder.CreateIntToPtr(value, context.builder.getInt8PtrTy());
            }
        case TOKENID_SUBTRACT:
            {
                llvm::Value* left_int = context.builder.CreatePtrToInt(left_value, context.builder.getInt64Ty());
                llvm::Value* right_int = context.builder.CreatePtrToInt(right_value, context.builder.getInt64Ty());
                llvm::Value* value = context.builder.CreateSub(left_int, right_int);
                return context.builder.CreateIntToPtr(value, context.builder.getInt8PtrTy());
            }
        case TOKENID_MULTIPLY:
            {
                llvm::Value* left_int = context.builder.CreatePtrToInt(left_value, context.builder.getInt64Ty());
                llvm::Value* right_int = context.builder.CreatePtrToInt(right_value, context.builder.getInt64Ty());
                llvm::Value* value = context.builder.CreateMul(left_int, right_int);
                return context.builder.CreateIntToPtr(value, context.builder.getInt8PtrTy());
            }
        case TOKENID_DIVIDE:
            {
                llvm::Value* left_int = context.builder.CreatePtrToInt(left_value, context.builder.getInt64Ty());
                llvm::Value* right_int = context.builder.CreatePtrToInt(right_value, context.builder.getInt64Ty());
                llvm::Value* value = context.builder.CreateSDiv(left_int, right_int);
                return context.builder.CreateIntToPtr(value, context.builder.getInt8PtrTy());
            }
        case TOKENID_MODULUS:
            {
                llvm::Value* left_int = context.builder.CreatePtrToInt(left_value, context.builder.getInt64Ty());
                llvm::Value* right_int = context.builder.CreatePtrToInt(right_value, context.builder.getInt64Ty());
                llvm::Value* value = context.builder.CreateSRem(left_int, right_int);
                return context.builder.CreateIntToPtr(value, context.builder.getInt8PtrTy());
            }
        case TOKENID_EQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpEQ(left_value, right_value);
        case TOKENID_INEQUALITY:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpNE(left_value, right_value);
        case TOKENID_LESS:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSLT(left_value, right_value);
        case TOKENID_GREATER:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSGT(left_value, right_value);
        case TOKENID_LESSEQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSLE(left_value, right_value);
        case TOKENID_GREATEREQ:
            if(expr_type != NULL) *expr_type = "bool";
            return context.builder.CreateICmpSGE(left_value, right_value);
        case TOKENID_AND:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        case TOKENID_OR:
            errors.panic("Operands to operator 'and' must have a type of 'bool'");
            return NULL;
        default:
            errors.panic("Unexpected operator '" + get_tokenid_syntax(operation) + "' in expression");
            return NULL;
        }
    }
    else {
        for(Enum& inum : program.enums){
            if(inum.name == type_name){
                switch (operation) {
                case TOKENID_EQUALITY:
                    if(expr_type != NULL) *expr_type = "bool";
                    return context.builder.CreateICmpEQ(left_value, right_value);
                case TOKENID_INEQUALITY:
                    if(expr_type != NULL) *expr_type = "bool";
                    return context.builder.CreateICmpNE(left_value, right_value);
                case TOKENID_LESS:
                    if(expr_type != NULL) *expr_type = "bool";
                    return context.builder.CreateICmpSLT(left_value, right_value);
                case TOKENID_GREATER:
                    if(expr_type != NULL) *expr_type = "bool";
                    return context.builder.CreateICmpSGT(left_value, right_value);
                case TOKENID_LESSEQ:
                    if(expr_type != NULL) *expr_type = "bool";
                    return context.builder.CreateICmpSLE(left_value, right_value);
                case TOKENID_GREATEREQ:
                    if(expr_type != NULL) *expr_type = "bool";
                    return context.builder.CreateICmpSGE(left_value, right_value);
                default:
                    errors.panic("Unexpected operator '" + get_tokenid_syntax(operation) + "' being used on an enum in expression");
                    return NULL;
                }
            }
        }

        errors.panic("Invalid use of operator '" + get_tokenid_syntax(operation) + "' on type '" + type_name + "'");
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
    case TOKENID_MODULUS:
        operator_str = "%";
        break;
    case TOKENID_EQUALITY:
        operator_str = "==";
        break;
    case TOKENID_INEQUALITY:
        operator_str = "!=";
        break;
    case TOKENID_LESS:
        operator_str = "<";
        break;
    case TOKENID_GREATER:
        operator_str = ">";
        break;
    case TOKENID_LESSEQ:
        operator_str = "<=";
        break;
    case TOKENID_GREATEREQ:
        operator_str = ">=";
        break;
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
    flags = EXP_CONSTANT;
    errors = err;
}
BoolExp::BoolExp(bool val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
BoolExp::BoolExp(const BoolExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
BoolExp::~BoolExp(){}
llvm::Value* BoolExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
ByteExp::ByteExp(int8_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
ByteExp::ByteExp(const ByteExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
ByteExp::~ByteExp(){}
llvm::Value* ByteExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
ShortExp::ShortExp(int16_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
ShortExp::ShortExp(const ShortExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
ShortExp::~ShortExp(){}
llvm::Value* ShortExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
IntegerExp::IntegerExp(int32_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
IntegerExp::IntegerExp(const IntegerExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
IntegerExp::~IntegerExp(){}
llvm::Value* IntegerExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
LongExp::LongExp(int64_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
LongExp::LongExp(const LongExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
LongExp::~LongExp(){}
llvm::Value* LongExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedByteExp::UnsignedByteExp(uint8_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedByteExp::UnsignedByteExp(const UnsignedByteExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
UnsignedByteExp::~UnsignedByteExp(){}
llvm::Value* UnsignedByteExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedShortExp::UnsignedShortExp(uint16_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedShortExp::UnsignedShortExp(const UnsignedShortExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
UnsignedShortExp::~UnsignedShortExp(){}
llvm::Value* UnsignedShortExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedIntegerExp::UnsignedIntegerExp(uint32_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedIntegerExp::UnsignedIntegerExp(const UnsignedIntegerExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
UnsignedIntegerExp::~UnsignedIntegerExp(){}
llvm::Value* UnsignedIntegerExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedLongExp::UnsignedLongExp(uint64_t val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
UnsignedLongExp::UnsignedLongExp(const UnsignedLongExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
UnsignedLongExp::~UnsignedLongExp(){}
llvm::Value* UnsignedLongExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "ulong";
    return llvm::ConstantInt::get(context.context, llvm::APInt(64, value, false));
}
std::string UnsignedLongExp::toString(){
    return to_str(value) + "ul";
}
PlainExp* UnsignedLongExp::clone(){
    return new UnsignedLongExp(*this);
}

HalfExp::HalfExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
HalfExp::HalfExp(float val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
HalfExp::HalfExp(const HalfExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
HalfExp::~HalfExp(){}
llvm::Value* HalfExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "half";
    return llvm::ConstantFP::get(llvm::Type::getHalfTy(context.context), value);
}
std::string HalfExp::toString(){
    return to_str(value) + "h";
}
PlainExp* HalfExp::clone(){
    return new HalfExp(*this);
}

FloatExp::FloatExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
FloatExp::FloatExp(float val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
FloatExp::FloatExp(const FloatExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
FloatExp::~FloatExp(){}
llvm::Value* FloatExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
DoubleExp::DoubleExp(double val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
DoubleExp::DoubleExp(const DoubleExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
DoubleExp::~DoubleExp(){}
llvm::Value* DoubleExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
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
    flags = EXP_CONSTANT;
    errors = err;
}
StringExp::StringExp(const std::string& val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
StringExp::StringExp(const StringExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
StringExp::~StringExp(){}
llvm::Value* StringExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    // Constant Definitions
    llvm::Constant* string_data = llvm::ConstantDataArray::getString(context.context, this->value.c_str(), true);

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
    return "\"" + string_replace_all(value, "\n", "\\n") + "\"";
}
PlainExp* StringExp::clone(){
    return new StringExp(*this);
}

LengthStringExp::LengthStringExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
LengthStringExp::LengthStringExp(const std::string& val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
LengthStringExp::LengthStringExp(const LengthStringExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
LengthStringExp::~LengthStringExp(){}
llvm::Value* LengthStringExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    // Constant Definitions
    llvm::Constant* string_data = llvm::ConstantDataArray::getString(context.context, this->value.c_str(), true);

    llvm::GlobalVariable* global_array = new llvm::GlobalVariable(*context.module, string_data->getType(), true, llvm::GlobalValue::PrivateLinkage, 0, "$.str");
    global_array->setAlignment(1);

    std::vector<llvm::Constant*> gep_indices(2);
    llvm::ConstantInt* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, 10));
    gep_indices[0] = zero;
    gep_indices[1] = zero;
    llvm::Constant* const_string = llvm::ConstantExpr::getGetElementPtr(string_data->getType(), global_array, gep_indices);

    std::vector<llvm::Constant*> length_string_values(2);
    length_string_values[0] = const_string;
    length_string_values[1] = llvm::ConstantInt::get(context.context, llvm::APInt(32, this->value.length()));

    // Global Variable Definitions
    global_array->setInitializer(string_data);
    if(expr_type != NULL) *expr_type = "[]ubyte";
    return llvm::ConstantStruct::get(program.llvm_array_type, length_string_values);
}
std::string LengthStringExp::toString(){
    return "\"" + string_replace_all(value, "\n", "\\n") + "\"";
}
PlainExp* LengthStringExp::clone(){
    return new LengthStringExp(*this);
}

WordExp::WordExp(ErrorHandler& err){
    flags = EXP_MUTABLE;
    errors = err;
}
WordExp::WordExp(const std::string& val, ErrorHandler& err){
    value = val;
    flags = EXP_MUTABLE;
    errors = err;
}
WordExp::WordExp(const WordExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_MUTABLE;
}
WordExp::~WordExp(){}
llvm::Value* WordExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    AssembleVariable* var = context.current_function->findVariable(value);
    Global global;

    if(var != NULL){
        if(expr_type != NULL) *expr_type = var->type;
        return var->variable;
    }

    if(program.find_global(value, &global) == 0){
        if(expr_type != NULL) *expr_type = global.type;
        return context.findGlobal(value)->variable;
    }

    errors.panic( UNDECLARED_VARIABLE(value) );
    return NULL;
}
std::string WordExp::toString(){
    return value;
}
PlainExp* WordExp::clone(){
    return new WordExp(*this);
}

AddrExp::AddrExp(ErrorHandler& err){
    flags = 0x00;
    errors = err;
}
AddrExp::AddrExp(PlainExp* val, ErrorHandler& err){
    value = val;
    flags = 0x00;
    errors = err;
}
AddrExp::AddrExp(const AddrExp& other) : PlainExp(other) {
    value = other.value->clone();
    flags = 0x00;
}
AddrExp::~AddrExp(){}
llvm::Value* AddrExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    std::string type_name;
    llvm::Value* llvm_value = this->value->assemble(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    if(expr_type != NULL) *expr_type = "*" + type_name;
    return llvm_value;
}
std::string AddrExp::toString(){
    return "&" + value->toString();
}
PlainExp* AddrExp::clone(){
    return new AddrExp(*this);
}

LoadExp::LoadExp(ErrorHandler& err){
    flags = EXP_MUTABLE;
    errors = err;
}
LoadExp::LoadExp(PlainExp* val, ErrorHandler& err){
    value = val;
    flags = EXP_MUTABLE;
    errors = err;
}
LoadExp::LoadExp(const LoadExp& other) : PlainExp(other) {
    value = other.value->clone();
    flags = EXP_MUTABLE;
}
LoadExp::~LoadExp(){
    delete value;
}
llvm::Value* LoadExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    std::string pointer_typename;
    llvm::Value* pointer_value = value->assemble(program, func, context, &pointer_typename);
    if(pointer_value == NULL) return NULL;

    if(pointer_typename[0] != '*' or !pointer_value->getType()->isPointerTy()){
        errors.panic("Can't dereference non-pointer type '" + pointer_typename + "'");
        return NULL;
    }

    if(expr_type != NULL) *expr_type = pointer_typename.substr(1, pointer_typename.length()-1);
    return context.builder.CreateLoad(pointer_value);
}
std::string LoadExp::toString(){
    return "*" + value->toString();
}
PlainExp* LoadExp::clone(){
    return new LoadExp(*this);
}

IndexLoadExp::IndexLoadExp(ErrorHandler& err){
    flags = EXP_MUTABLE;
    errors = err;
}
IndexLoadExp::IndexLoadExp(PlainExp* val, PlainExp* idx, ErrorHandler& err){
    value = val;
    index = idx;
    flags = EXP_MUTABLE;
    errors = err;
}
IndexLoadExp::IndexLoadExp(const IndexLoadExp& other) : PlainExp(other) {
    value = other.value->clone();
    index = other.index->clone();
    flags = EXP_MUTABLE;
}
IndexLoadExp::~IndexLoadExp(){
    delete value;
}
llvm::Value* IndexLoadExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    std::string pointer_typename;
    llvm::Value* pointer_value = value->assemble(program, func, context, &pointer_typename);
    if(pointer_value == NULL) return NULL;

    if(!(value->flags & EXP_MUTABLE)){
        errors.panic("Can't use [] operator on immutable expression");
        return NULL;
    }

    program.resolve_if_alias(pointer_typename);

    if(Program::is_array_typename(pointer_typename)){
        // Handle as higher level array
        return this->assemble_highlevel_array(program, func, context, expr_type, pointer_value, pointer_typename);
    }
    else {
        // Handle as lower lever array
        return this->assemble_lowlevel_array(program, func, context, expr_type, pointer_value, pointer_typename);
    }

    return NULL;
}
std::string IndexLoadExp::toString(){
    return value->toString() + "[" + index->toString() + "]";
}
PlainExp* IndexLoadExp::clone(){
    return new IndexLoadExp(*this);
}
llvm::Value* IndexLoadExp::assemble_lowlevel_array(Program& program, Function& func, AssemblyData& context, std::string* expr_type,
                                                    llvm::Value* pointer_value, const std::string& pointer_typename){
    // Assemble lower lever array

    if(!Program::is_pointer_typename(pointer_typename) or !pointer_value->getType()->isPointerTy()){
        errors.panic("Can't dereference non-pointer type '" + pointer_typename + "'");
        return NULL;
    }

    if(pointer_typename == "ptr"){
        errors.panic("Can't dereference generic pointer type 'ptr'\n    (It must be casted to the appropriate type first)");
        return NULL;
    }

    pointer_value = context.builder.CreateLoad(pointer_value);

    std::string index_typename;
    llvm::Value* index_value = index->assemble_immutable(program, func, context, &index_typename);
    if(index_value == NULL) return NULL;

    // Resolve the typename if its an alias
    program.resolve_if_alias(index_typename);

    if(!Program::is_integer_typename(index_typename)){
        errors.panic("Expected integer type when using []");
        return NULL;
    }

    if(expr_type != NULL) *expr_type = pointer_typename.substr(1, pointer_typename.length()-1);

    std::vector<llvm::Value*> indices(1);
    indices[0] = index_value;

    pointer_value = context.builder.CreateGEP(pointer_value, indices);
    return pointer_value;
}
llvm::Value* IndexLoadExp::assemble_highlevel_array(Program& program, Function& func, AssemblyData& context, std::string* expr_type,
                                                    llvm::Value* pointer_value, const std::string& pointer_typename){
    // Assemble higher lever array
    // NOTE: pointer_typename is assumed to be an array typename

    std::string target_typename = pointer_typename.substr(2, pointer_typename.length()-2);

    // Quick Hack to get the first member of the array structure (program.llvm_array_type) without using GEP.
    //     We can do this since the member we want (which is 'data ptr') is the first member
    pointer_value = context.builder.CreateLoad( context.builder.CreateBitCast(pointer_value, llvm::Type::getInt8PtrTy(context.context)->getPointerTo()) );

    // Cast i8* to the correct type
    llvm::Type* target_llvm_type;
    if(program.find_type(target_typename, context, &target_llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(target_typename));
        return NULL;
    }
    pointer_value = context.builder.CreateBitCast(pointer_value, target_llvm_type->getPointerTo());

    std::string index_typename;
    llvm::Value* index_value = index->assemble_immutable(program, func, context, &index_typename);
    if(index_value == NULL) return NULL;

    // Resolve the typename if its an alias
    program.resolve_if_alias(index_typename);

    if(!Program::is_integer_typename(index_typename)){
        errors.panic("Expected integer type when using []");
        return NULL;
    }

    if(expr_type != NULL) *expr_type = target_typename;

    std::vector<llvm::Value*> indices(1);
    indices[0] = index_value;

    pointer_value = context.builder.CreateGEP(pointer_value, indices);
    return pointer_value;
}

CallExp::CallExp(ErrorHandler& err){
    flags = 0x00;
    errors = err;
}
CallExp::CallExp(const CallExp& other) : PlainExp(other) {
    name = other.name;
    for(PlainExp* e : other.args) args.push_back( e->clone() );
    flags = 0x00;
}
CallExp::CallExp(std::string n, const std::vector<PlainExp*>& a, ErrorHandler& err){
    name = n;
    args = a;
    flags = 0x00;
    errors = err;
}
CallExp::~CallExp(){
    for(PlainExp* e : args){
        delete e;
    }
}
llvm::Value* CallExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    External func_data;
    AssembleVariable* func_variable;
    Global global;

    llvm::Value* expr_value;
    std::string expr_typename;
    llvm::Type* expected_arg_type;
    std::vector<llvm::Value*> argument_values;
    std::vector<std::string> argument_types;
    std::vector<llvm::Type*> argument_llvm_types;

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble_immutable(program, func, context, &expr_typename);
        if(expr_value == NULL) return NULL;

        if(program.find_type(expr_typename, context, &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(expr_typename) );
            return NULL;
        }

        argument_values.push_back(expr_value);
        argument_types.push_back(expr_typename);
        argument_llvm_types.push_back(expected_arg_type);
    }

    if(program.find_func(name, argument_types, &func_data) == 0){
        // Standard function exists

        if(func_data.flags & EXTERN_MULRET){
            errors.panic("No result variables specified for function '" + name + "' that returns multiple arguments");
            return NULL;
        }

        std::string final_name = (func_data.flags & EXTERN_MANGLED) ? mangle(name, func_data.arguments) : name;
        llvm::Function* target = context.module->getFunction(final_name);
        if (!target){
            errors.panic_undeclared_func(name, argument_types);
            return NULL;
        }
        ensure(func_data.arguments.size() == target->arg_size());
        if(expr_type != NULL) *expr_type = func_data.return_type;

        if(func_data.flags & EXTERN_VARARGS){
            std::string va_arg_typename = func_data.arguments[func_data.arguments.size()-1];
            va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);
            uint64_t va_arg_count = args.size() - func_data.arguments.size() + 1;

            return this->handle_var_args(program, func, context, va_arg_typename, va_arg_count, func_data.arguments.size(),
                                        argument_values, target, func_data.flags & EXTERN_STDCALL);
        }
        else {
            for(size_t i = 0; i != argument_values.size(); i++){
                if(program.find_type(func_data.arguments[i], context, &expected_arg_type) != 0){
                    errors.panic( UNDECLARED_TYPE(func_data.arguments[i]) );
                    return 1;
                }

                if(assemble_merge_types_oneway(context, program, argument_types[i], func_data.arguments[i], &argument_values[i], expected_arg_type, NULL) != 0){
                    // NOTE: This error should theoretically never occur
                    errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + func_data.toString() +
                         "\n    Expected type '" + func_data.arguments[i] + "' but received type '" + argument_types[i] + "'");
                    return 1;
                }
            }

            llvm::CallInst* call = context.builder.CreateCall(target, argument_values);
            call->setCallingConv(func_data.flags & EXTERN_STDCALL ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);
            return call;
        }
    }

    func_variable = context.current_function->findVariable(name);

    if(func_variable != NULL){
        // Variable that could be function pointer exists
        if(Program::is_function_typename(func_variable->type)){
            // The variable is a function pointer

            std::string varfunc_return_typename;
            llvm::Type* varfunc_return_llvm_type;
            std::vector<std::string> varfunc_args;
            std::vector<llvm::Type*> varfunc_llvm_args;
            char flags = 0x00;

            if(program.extract_function_pointer_info(func_variable->type, varfunc_llvm_args, context, &varfunc_return_llvm_type, varfunc_args,
                varfunc_return_typename, NULL, flags) != 0) return NULL;

            if(varfunc_return_llvm_type == NULL){
                errors.panic("No result variables specified for function pointer '" + name + "' that returns multiple arguments");
                return 1;
            }

            if(expr_type != NULL) *expr_type = varfunc_return_typename;

            if(flags & FUNC_VARARGS){
                AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );
                std::string va_arg_typename = varfunc_args[varfunc_args.size()-1];
                va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);
                uint64_t va_arg_count = args.size() - varfunc_args.size() + 1;

                return this->handle_var_args(program, func, context, va_arg_typename, va_arg_count, varfunc_args.size(),
                                        argument_values, context.builder.CreateLoad(func_variable->variable), Program::function_typename_is_stdcall(func_variable->type));
            }
            else {
                if (varfunc_args.size() != args.size()){
                    errors.panic("Incorrect function argument count when calling '" + name + "' (" + to_str(args.size()) + " received, " + to_str(varfunc_args.size()) + " required)");
                    return NULL;
                }

                for(size_t i = 0; i != argument_values.size(); i++){
                    if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                        errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + func_variable->type +
                             "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                        return NULL;
                    }
                }

                llvm::Value* function_address = context.builder.CreateLoad(func_variable->variable);
                llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values);

                if(Program::function_typename_is_stdcall(func_variable->type)){
                    call->setCallingConv(llvm::CallingConv::X86_StdCall);
                } else {
                    call->setCallingConv(llvm::CallingConv::C);
                }
                return call;
            }
        }
    }

    if(program.find_global(name, &global) == 0){
        if(Program::is_function_typename(global.type)){
            // The variable is a function pointer

            std::string varfunc_return_typename;
            llvm::Type* varfunc_return_llvm_type;
            std::vector<std::string> varfunc_args;
            std::vector<llvm::Type*> varfunc_llvm_args;
            char flags = 0x00;

            if(program.extract_function_pointer_info(global.type, varfunc_llvm_args, context, &varfunc_return_llvm_type, varfunc_args,
                varfunc_return_typename, NULL, flags) != 0) return NULL;

            if(varfunc_return_llvm_type == NULL){
                errors.panic("Incorrect function argument count when calling '" + name + "' (" + to_str(args.size()) + " received, " + to_str(varfunc_args.size()) + " required)");
                return 1;
            }

            if(expr_type != NULL) *expr_type = varfunc_return_typename;

            if(flags & FUNC_VARARGS){
                AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );
                std::string va_arg_typename = varfunc_args[varfunc_args.size()-1];
                va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);
                uint64_t va_arg_count = args.size() - varfunc_args.size() + 1;
                AssembleGlobal* global_asm_data = context.findGlobal(global.name);

                if(global_asm_data == NULL){
                    errors.panic("Failed to find assembly data for global");
                    return NULL;
                }

                return this->handle_var_args(program, func, context, va_arg_typename, va_arg_count, varfunc_args.size(),
                                        argument_values, context.builder.CreateLoad(global_asm_data->variable), Program::function_typename_is_stdcall(global.type));
            }
            else {
                if (varfunc_args.size() != args.size()){
                    errors.panic("Incorrect function argument count when calling '" + name + "'");
                    return NULL;
                }

                for(size_t i = 0; i != argument_values.size(); i++){
                    if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                        errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + global.type +
                             "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                        return NULL;
                    }
                }

                llvm::Value* function_address = context.builder.CreateLoad(context.findGlobal(name)->variable);
                llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values);

                if(Program::function_typename_is_stdcall(global.type)){
                    call->setCallingConv(llvm::CallingConv::X86_StdCall);
                } else {
                    call->setCallingConv(llvm::CallingConv::C);
                }
                return call;
            }
        }
    }

    errors.panic_undeclared_func(name, argument_types);
    return NULL;
}
llvm::Value* CallExp::handle_var_args(Program& program, Function& func, AssemblyData& context, const std::string& va_arg_typename, uint64_t va_arg_count, uint64_t impl_arg_count,
                                  std::vector<llvm::Value*>& argument_values, llvm::Value* target, bool target_is_stdcall){
    AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );

    llvm::Type* va_arg_type;
    if(program.find_type(va_arg_typename, context, &va_arg_type) != 0){
        errors.panic( UNDECLARED_TYPE(va_arg_typename) );
        return NULL;
    }

    if(asm_func->va_args == NULL){
        llvm::BasicBlock* prev_block = context.builder.GetInsertBlock();
        context.builder.SetInsertPoint(asm_func->entry);
        asm_func->va_args = context.builder.CreateAlloca(program.llvm_array_type);
        context.builder.SetInsertPoint(prev_block);
    }

    llvm::Function* malloc_function = context.module->getFunction("malloc");
    llvm::Function* free_function = context.module->getFunction("free");

    if(!malloc_function){
        // Declare the malloc function if it doesn't already exist
        llvm::Type* return_llvm_type;

        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt32Ty(context.context);
        return_llvm_type = llvm::Type::getInt8PtrTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
        malloc_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "malloc", context.module.get());
    }

    if(!free_function){
        // Declare the malloc function if it doesn't already exist
        llvm::Type* return_llvm_type;

        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt8PtrTy(context.context);
        return_llvm_type = llvm::Type::getVoidTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
        free_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "free", context.module.get());
    }

    uint64_t va_arg_type_size = context.module->getDataLayout().getTypeAllocSize(va_arg_type);

    std::vector<llvm::Value*> call_values(1);
    call_values[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, va_arg_type_size * va_arg_count, false));
    llvm::Value* arguments_memory = context.builder.CreateCall(malloc_function, call_values);
    llvm::Value* va_data = context.builder.CreateBitCast(asm_func->va_args, llvm::Type::getInt8PtrTy(context.context)->getPointerTo());

    // Store allocated memory inside va_data
    context.builder.CreateStore(arguments_memory, va_data);

    // Store length of allocated memory inside va_length
    std::vector<llvm::Value*> indices(2);
    indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
    indices[1] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 1, false));

    llvm::Value* va_length = context.builder.CreateGEP(program.llvm_array_type, asm_func->va_args, indices);
    context.builder.CreateStore(llvm::ConstantInt::get(context.context, llvm::APInt(32, va_arg_count, false)), va_length);

    // Call the function with the new argument values
    std::vector<llvm::Value*> new_argument_values(impl_arg_count);

    // Add variable arguments into array
    size_t va_index = 0;
    std::vector<llvm::Value*> array_gep_indices(1);
    llvm::Value* raw_values_array = context.builder.CreateBitCast(arguments_memory, va_arg_type->getPointerTo());
    for(size_t i = impl_arg_count-1; i != argument_values.size(); i++){
        array_gep_indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, va_index, false));
        context.builder.CreateStore(argument_values[i], context.builder.CreateGEP(raw_values_array, array_gep_indices));
        va_index++;
    }

    // Add argument values and va argument values together
    for(size_t j = 0; j != new_argument_values.size()-1; j++){
        new_argument_values[j] = argument_values[j];
    }
    new_argument_values[new_argument_values.size()-1] = context.builder.CreateLoad(asm_func->va_args);

    // Pass arguments
    llvm::CallInst* call = context.builder.CreateCall(target, new_argument_values);
    call->setCallingConv(target_is_stdcall ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);

    // Free variable argument array
    call_values[0] = arguments_memory;
    context.builder.CreateCall(free_function, call_values);
    return call;
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
    flags = EXP_MUTABLE;
}
MemberExp::MemberExp(PlainExp* v, const std::string& m, ErrorHandler& err){
    value = v;
    member = m;
    flags = EXP_MUTABLE;
    errors = err;
}
MemberExp::MemberExp(const MemberExp& other) : PlainExp(other) {
    value = other.value->clone();
    member = other.member;
    flags = EXP_MUTABLE;
}
MemberExp::~MemberExp(){
    delete value;
}
llvm::Value* MemberExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    std::string type_name;
    llvm::Value* data = value->assemble(program, func, context, &type_name);
    if(data == NULL) return NULL;
    program.resolve_if_alias(type_name);

    if(!(value->flags & EXP_MUTABLE)){
        // Value isn't mutable so we can't get the value of a member from it
        errors.panic("Can't use '.' operator on immutable expression");
        return NULL;
    }

    if(Program::is_pointer_typename(type_name)){
        // The type is actually a pointer to a structure or class, so we'll dereference it automatically
        // ( Unlike the nightmare that is '->' in C++ )
        data = context.builder.CreateLoad(data);
        type_name = type_name.substr(1, type_name.length()-1);
    }

    Structure target_struct;
    if(program.find_struct(type_name, &target_struct) == 0){
        // A structure named that was found
        return this->assemble_struct(program, func, context, expr_type, target_struct, data);
    }

    Class target_class;
    if(program.find_class(type_name, &target_class) == 0){
        // A class named that was found
        return this->assemble_class(program, func, context, expr_type, target_class, data, type_name);
    }

    if(Program::is_array_typename(type_name)){
        // The type is actually an array
        return this->assemble_array(program, func, context, expr_type, data, type_name);
    }

    // No structure or class named that was found
    errors.panic( UNDECLARED_STRUCT(type_name) );
    return NULL;
}
std::string MemberExp::toString(){
    return value->toString() + "." + member;
}
PlainExp* MemberExp::clone(){
    return new MemberExp(*this);
}
llvm::Value* MemberExp::assemble_struct(Program& program, Function& func, AssemblyData& context, std::string* expr_type, Structure& target, llvm::Value* data){
    int index;
    llvm::Value* member_index;

    // Find the index of the member inside of the structure
    if(target.find_index(member, &index) != 0){
        // That member doesn't exist
        errors.panic( UNDECLARED_MEMBER(member, target.name) );
        return NULL;
    }

    // Get the llvm type for the struct
    llvm::Type* struct_llvm_type;
    if(program.find_type(target.name, context, &struct_llvm_type) != 0){
        // That type doesn't exist so something bad probally went wrong
        errors.panic( UNDECLARED_TYPE(target.name) );
        return NULL;
    }

    // Prepare GEP Indices
    std::vector<llvm::Value*> indices(2);
    member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, true));
    indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
    indices[1] = member_index;

    // Create GEP and set expr_type if it isn't null
    llvm::Value* member_ptr = context.builder.CreateGEP(struct_llvm_type, data, indices);
    if(expr_type != NULL) *expr_type = target.members[index].type;

    return member_ptr;
}
llvm::Value* MemberExp::assemble_class(Program& program, Function& func, AssemblyData& context, std::string* expr_type, Class& target, llvm::Value* data, std::string& data_typename){
    int index;
    llvm::Value* member_index;

    // Find the index of the member inside of the class
    if(target.find_index(member, &index) != 0){
        // That member doesn't exist
        errors.panic( UNDECLARED_MEMBER(member, target.name) );
        return NULL;
    }

    // Make sure that the member is not static and public
    ClassField* field = &target.members[index];
    std::string parent_class_name = (func.parent_class_offset != 0) ? program.classes[func.parent_class_offset-1].name : "";

    if(!(field->flags & CLASSFIELD_PUBLIC) and parent_class_name != data_typename){
        errors.panic("The member '" + member + "' of class '" + target.name + "' is private");
        return NULL;
    }
    if(field->flags & CLASSFIELD_STATIC){
        errors.panic("The member '" + member + "' of class '" + target.name + "' is static");
        return NULL;
    }

    // Get the llvm type for the class
    llvm::Type* class_llvm_type;
    if(program.find_type(target.name, context, &class_llvm_type) != 0){
        // That type doesn't exist so something bad probally went wrong
        errors.panic( UNDECLARED_TYPE(target.name) );
        return NULL;
    }

    // Prepare GEP Indices
    std::vector<llvm::Value*> indices(2);
    member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, true));
    indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
    indices[1] = member_index;

    // Create GEP and set expr_type if it isn't null
    llvm::Value* member_ptr = context.builder.CreateGEP(class_llvm_type, data, indices);
    if(expr_type != NULL) *expr_type = field->type;

    return member_ptr;
}
llvm::Value* MemberExp::assemble_array(Program& program, Function& func, AssemblyData& context, std::string* expr_type, llvm::Value* data, std::string& data_typename){
    int index;
    llvm::Value* member_index;

    if(member == "data"){
        if(expr_type != NULL) *expr_type = "ptr";
        index = 0;
    } else if(member == "length"){
        if(expr_type != NULL) *expr_type = "usize";
        index = 1;
    } else {
        errors.panic("Array type has no member named '" + member + "'");
        return NULL;
    }

    // Prepare GEP Indices
    std::vector<llvm::Value*> indices(2);
    member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, false));
    indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
    indices[1] = member_index;

    // Create GEP
    return context.builder.CreateGEP(program.llvm_array_type, data, indices);
}

MemberCallExp::MemberCallExp(ErrorHandler& err){
    flags = 0x00;
    errors = err;
}
MemberCallExp::MemberCallExp(PlainExp* obj, const std::string& n, const std::vector<PlainExp*>& a, ErrorHandler& err){
    object = obj;
    name = n;
    args = a;
    flags = 0x00;
    errors = err;
}
MemberCallExp::MemberCallExp(const MemberCallExp& other) : PlainExp(other) {
    object = other.object->clone();
    name = other.name;
    args = other.args;
    flags = 0x00;
}
MemberCallExp::~MemberCallExp(){
    delete object;
}
llvm::Value* MemberCallExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    External func_data;
    llvm::Value* expr_value;
    std::string expr_typename;
    llvm::Type* expected_arg_type;
    std::vector<llvm::Value*> argument_values;
    std::vector<std::string> argument_types;
    std::vector<llvm::Type*> argument_llvm_types;

    std::string object_typename;
    llvm::Value* object_value = object->assemble(program, func, context, &object_typename);
    llvm::Type* object_llvm_type;

    if(Program::is_pointer_typename(object_typename)){
        // The type is actually a pointer to a structure or class, so we'll dereference it automatically
        // ( Unlike the nightmare that is '->' in C++ )
        object_value = context.builder.CreateLoad(object_value);
        object_typename = object_typename.substr(1, object_typename.length()-1);
    }

    // Ensure the object is mutable
    if(!(object->flags & EXP_MUTABLE)){
        errors.panic("Can't call method of object because it is immutable");
        return NULL;
    }

    // Get the llvm type for the object
    if(program.find_type(object_typename, context, &object_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(object_typename) );
        return NULL;
    }

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble_immutable(program, func, context, &expr_typename);
        if(expr_value == NULL) return NULL;

        if(program.find_type(expr_typename, context, &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(expr_typename) );
            return NULL;
        }

        argument_values.push_back(expr_value);
        argument_types.push_back(expr_typename);
        argument_llvm_types.push_back(expected_arg_type);
    }

    if(program.find_method(object_typename, name, argument_types, &func_data) != 0){
        errors.panic_undeclared_method(object_typename, name, argument_types);
        return NULL;
    }

    std::string parent_class_name = (func.parent_class_offset != 0) ? program.classes[func.parent_class_offset-1].name : "";

    // Ensure the function is public
    if(!(func_data.flags & EXTERN_PUBLIC) and parent_class_name != object_typename){
        errors.panic("The method '" + object_typename + "." + name + "' is private");
        return NULL;
    }

    argument_values.insert(argument_values.begin(), object_value);
    argument_types.insert(argument_types.begin(), "*" + object_typename);
    argument_llvm_types.insert(argument_llvm_types.begin(), object_llvm_type->getPointerTo());

    std::string final_name = mangle(object_typename, name, func_data.arguments);
    llvm::Function* target = context.module->getFunction(final_name);
    if (target == NULL){
        errors.panic_undeclared_method(object_typename, name, argument_types);
        return NULL;
    }

    ensure(func_data.arguments.size() + 1 == target->arg_size());

    if(func_data.flags & EXTERN_VARARGS){
        AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );
        std::string va_arg_typename = func_data.arguments[func_data.arguments.size()-1];
        va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);

        llvm::Type* va_arg_type;
        if(program.find_type(va_arg_typename, context, &va_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(va_arg_typename) );
            return 1;
        }

        if(asm_func->va_args == NULL){
            llvm::BasicBlock* prev_block = context.builder.GetInsertBlock();
            context.builder.SetInsertPoint(asm_func->entry);
            asm_func->va_args = context.builder.CreateAlloca(program.llvm_array_type);
            context.builder.SetInsertPoint(prev_block);
        }

        llvm::Function* malloc_function = context.module->getFunction("malloc");
        llvm::Function* free_function = context.module->getFunction("free");

        if(!malloc_function){
            // Declare the malloc function if it doesn't already exist
            llvm::Type* return_llvm_type;

            std::vector<llvm::Type*> args(1);
            args[0] = llvm::Type::getInt32Ty(context.context);
            return_llvm_type = llvm::Type::getInt8PtrTy(context.context);

            llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
            malloc_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "malloc", context.module.get());
        }

        if(!free_function){
            // Declare the malloc function if it doesn't already exist
            llvm::Type* return_llvm_type;

            std::vector<llvm::Type*> args(1);
            args[0] = llvm::Type::getInt8PtrTy(context.context);
            return_llvm_type = llvm::Type::getVoidTy(context.context);

            llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
            free_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "free", context.module.get());
        }

        uint64_t va_arguments_count = args.size() - func_data.arguments.size() + 1;
        uint64_t va_arg_type_size = context.module->getDataLayout().getTypeAllocSize(va_arg_type);

        std::vector<llvm::Value*> call_values(1);
        call_values[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, va_arg_type_size * va_arguments_count, false));
        llvm::Value* arguments_memory = context.builder.CreateCall(malloc_function, call_values);
        llvm::Value* va_data = context.builder.CreateBitCast(asm_func->va_args, llvm::Type::getInt8PtrTy(context.context)->getPointerTo());

        // Store allocated memory inside va_data
        context.builder.CreateStore(arguments_memory, va_data);

        // Store length of allocated memory inside va_length
        std::vector<llvm::Value*> indices(2);
        indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
        indices[1] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 1, false));

        llvm::Value* va_length = context.builder.CreateGEP(program.llvm_array_type, asm_func->va_args, indices);
        context.builder.CreateStore(llvm::ConstantInt::get(context.context, llvm::APInt(32, va_arguments_count, false)), va_length);

        // Call the function with the new argument values
        std::vector<llvm::Value*> new_argument_values(func_data.arguments.size() + 1);

        // Add variable arguments into array
        size_t va_index = 0;
        std::vector<llvm::Value*> array_gep_indices(1);
        llvm::Value* raw_values_array = context.builder.CreateBitCast(arguments_memory, va_arg_type->getPointerTo());
        for(size_t i = func_data.arguments.size(); i != argument_values.size(); i++){
            array_gep_indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, va_index, false));
            context.builder.CreateStore(argument_values[i], context.builder.CreateGEP(raw_values_array, array_gep_indices));
            va_index++;
        }

        // Add argument values and va argument values together
        size_t j;
        for(j = 0; j != new_argument_values.size()-1; j++){
            new_argument_values[j] = argument_values[j];
        }
        new_argument_values[new_argument_values.size()-1] = context.builder.CreateLoad(asm_func->va_args);

        // Pass arguments
        if(expr_type != NULL) *expr_type = func_data.return_type;
        llvm::CallInst* call = context.builder.CreateCall(target, new_argument_values);
        call->setCallingConv(func_data.flags & EXTERN_STDCALL ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);

        // Free variable argument array
        call_values[0] = arguments_memory;
        context.builder.CreateCall(free_function, call_values);
        return call;
    }
    else {
        if (target->arg_size() != args.size()+1){
            // NOTE: This error should never appear
            errors.panic("Incorrect method argument count for method '" + object_typename + "." + name + "'");
            return NULL;
        }

        for(size_t i = 1; i != argument_values.size(); i++){
            if(program.find_type(func_data.arguments[i-1], context, &expected_arg_type) != 0){
                errors.panic( UNDECLARED_TYPE(func_data.arguments[i-1]) );
                return NULL;
            }

            if(assemble_merge_types_oneway(context, program, argument_types[i], func_data.arguments[i-1], &argument_values[i], expected_arg_type, NULL) != 0){
                // NOTE: This error should never occur
                errors.panic("Incorrect type for argument " + to_str(i+1) + " of method '" + object_typename + "." + name + "'\n    Definition: " + func_data.toString() +
                     "\n    Expected type '" + func_data.arguments[i-1] + "' but received type '" + argument_types[i] + "'");
                return NULL;
            }
        }

        if(expr_type != NULL) *expr_type = func_data.return_type;
        llvm::Value* call = context.builder.CreateCall(target, argument_values);
        return call;
    }
}
std::string MemberCallExp::toString(){
    std::string args_str;

    for(size_t i = 0; i != args.size(); i++){
        args_str += args[i]->toString();
        if(i + 1 != args.size()) args_str += ", ";
    }

    return object->toString() + "." + name + "(" + args_str + ")";
}
PlainExp* MemberCallExp::clone(){
    return new MemberCallExp(*this);
}

NullExp::NullExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
NullExp::NullExp(const NullExp& other) : PlainExp(other) {
    flags = EXP_CONSTANT;
}
NullExp::~NullExp(){}
llvm::Value* NullExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "ptr";
    return llvm::ConstantPointerNull::get( llvm::Type::getInt8PtrTy(context.context) );
}
std::string NullExp::toString(){
    return "null";
}
PlainExp* NullExp::clone(){
    return new NullExp(errors);
}

NotExp::NotExp(ErrorHandler& err){
    flags = 0x00;
    errors = err;
}
NotExp::NotExp(PlainExp* val, ErrorHandler& err){
    value = val;
    flags = 0x00;
    errors = err;
}
NotExp::NotExp(const NotExp& other) : PlainExp(other) {
    value = other.value->clone();
    flags = 0x00;
}
NotExp::~NotExp(){
    delete value;
}
llvm::Value* NotExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;
    if(expr_type != NULL) *expr_type = "bool";

    if(type_name == "bool"){
        return context.builder.CreateNot(llvm_value);
    }
    else if(type_name == "byte"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
        return llvm_value;
    }
    else if(type_name == "ubyte"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
        return llvm_value;
    }
    else if(type_name == "short"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
        return llvm_value;
    }
    else if(type_name == "ushort"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
        return llvm_value;
    }
    else if(type_name == "int"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
        return llvm_value;
    }
    else if(type_name == "uint"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
        return llvm_value;
    }
    else if(type_name == "long"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, true));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
        return llvm_value;
    }
    else if(type_name == "ulong"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, false));
        llvm_value = context.builder.CreateICmpEQ(llvm_value, zero);
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
    flags = 0x00;
    errors = err;
}
CastExp::CastExp(PlainExp* val, std::string tgt_type, ErrorHandler& err){
    value = val;
    target_typename = tgt_type;
    flags = 0x00;
    errors = err;
}
CastExp::~CastExp(){
    delete value;
}
llvm::Value* CastExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    program.resolve_if_alias(target_typename);
    if(expr_type != NULL) *expr_type = target_typename;

    // If target typename is empty, then something very bad has gone wrong
    if(target_typename == "") return NULL;

    if(target_typename[0] == '*'){
        return this->cast_to_valueptr(program, func, context);
    }
    else if(target_typename == "ptr"){
        return this->cast_to_ptr(program, func, context);
    }
    else if(target_typename == "bool"){
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
    else if(target_typename == "float"){
        return this->cast_to_float(program, func, context);
    }
    else if(target_typename == "double"){
        return this->cast_to_double(program, func, context);
    }
    else if(target_typename == "half"){
        errors.panic("Casting values to the type 'half' is currently not supported");
        return NULL;
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
llvm::Value* CastExp::cast_to_bool(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name == "bool"){
        return llvm_value;
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty());
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty());
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty());
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty());
    }
    else if(type_name == "float" or type_name == "double"){
        llvm_value = context.builder.CreateFPToUI(llvm_value, context.builder.getInt32Ty());
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt1Ty());
    }
    else if(type_name == "half"){
        errors.panic("Casting values of the type 'half' is currently not supported");
        return NULL;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_byte(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt8Ty());
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return llvm_value;
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt8Ty());
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt8Ty());
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt8Ty());
    }
    else if(type_name == "float" or type_name == "double"){
        llvm_value = context.builder.CreateFPToUI(llvm_value, context.builder.getInt32Ty());
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt8Ty());
    }
    else if(type_name == "half"){
        errors.panic("Casting values of the type 'half' is currently not supported");
        return NULL;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_short(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt16Ty());
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt16Ty());
    }
    else if(type_name == "ushort" or type_name == "short"){
        return llvm_value;
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt16Ty());
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt16Ty());
    }
    else if(type_name == "float" or type_name == "double"){
        llvm_value = context.builder.CreateFPToUI(llvm_value, context.builder.getInt32Ty());
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt16Ty());
    }
    else if(type_name == "half"){
        errors.panic("Casting values of the type 'half' is currently not supported");
        return NULL;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_int(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt32Ty());
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt32Ty());
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt32Ty());
    }
    else if(type_name == "uint" or type_name == "int"){
        return llvm_value;
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateTrunc(llvm_value, context.builder.getInt32Ty());
    }
    else if(type_name == "float" or type_name == "double"){
        return context.builder.CreateFPToUI(llvm_value, context.builder.getInt32Ty());
    }
    else if(type_name == "ptr"){
        return context.builder.CreatePtrToInt(llvm_value, context.builder.getInt32Ty());
    }
    else if(Program::is_pointer_typename(type_name) or Program::is_function_typename(type_name)){
        llvm_value = context.builder.CreateBitCast(llvm_value, context.builder.getInt8PtrTy());
        return context.builder.CreatePtrToInt(llvm_value, context.builder.getInt32Ty());
    }
    else if(type_name == "half"){
        errors.panic("Casting values of the type 'half' is currently not supported");
        return NULL;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_long(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name == "bool"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty());
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty());
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty());
    }
    else if(type_name == "uint" or type_name == "int"){
        return context.builder.CreateZExt(llvm_value, context.builder.getInt64Ty());
    }
    else if(type_name == "ulong" or type_name == "long"){
        return llvm_value;
    }
    else if(type_name == "float" or type_name == "double"){
        return context.builder.CreateFPToUI(llvm_value, context.builder.getInt64Ty());
    }
    else if(type_name == "ptr"){
        return context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty());
    }
    else if(type_name == "half"){
        errors.panic("Casting values of the type 'half' is currently not supported");
        return NULL;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_float(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name[0] == 'u' && (type_name == "uint" or type_name == "ushort" or type_name == "ulong" or type_name == "ubyte")){
        return context.builder.CreateUIToFP(llvm_value, context.builder.getFloatTy());
    }
    else if(type_name == "int" or type_name == "short" or type_name == "long" or type_name == "byte" or type_name == "bool"){
        return context.builder.CreateSIToFP(llvm_value, context.builder.getFloatTy());
    }
    else if(type_name == "double"){
        return context.builder.CreateFPTrunc(llvm_value, context.builder.getFloatTy());
    }
    else if(type_name == "half"){
        errors.panic("Casting values of the type 'half' is currently not supported");
        return NULL;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_double(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name[0] == 'u' && (type_name == "uint" or type_name == "ushort" or type_name == "ulong" or type_name == "ubyte")){
        return context.builder.CreateUIToFP(llvm_value, context.builder.getDoubleTy());
    }
    else if(type_name == "int" or type_name == "short" or type_name == "long" or type_name == "byte" or type_name == "bool"){
        return context.builder.CreateSIToFP(llvm_value, context.builder.getDoubleTy());
    }
    else if(type_name == "float"){
        return context.builder.CreateFPExt(llvm_value, context.builder.getDoubleTy());
    }
    else if(type_name == "half"){
        errors.panic("Casting values of the type 'half' is currently not supported");
        return NULL;
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_ptr(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(type_name == "bool"){
        return context.builder.CreateIntToPtr(llvm_value, context.builder.getInt8PtrTy());
    }
    else if(type_name == "ubyte" or type_name == "byte"){
        return context.builder.CreateIntToPtr(llvm_value, context.builder.getInt8PtrTy());
    }
    else if(type_name == "ushort" or type_name == "short"){
        return context.builder.CreateIntToPtr(llvm_value, context.builder.getInt8PtrTy());
    }
    else if(type_name == "uint" or type_name == "int"){
        return llvm_value = context.builder.CreateIntToPtr(llvm_value, context.builder.getInt8PtrTy());
    }
    else if(type_name == "ulong" or type_name == "long"){
        return context.builder.CreateIntToPtr(llvm_value, context.builder.getInt8PtrTy());
    }
    else if(Program::is_pointer_typename(type_name)){
        return context.builder.CreateBitCast(llvm_value, context.builder.getInt8PtrTy());
    }
    else if(Program::is_function_typename(type_name)){
        return context.builder.CreateBitCast(llvm_value, context.builder.getInt8PtrTy());
    }
    else {
        errors.panic("Can't cast type '" + type_name + "' to a '" + target_typename + "'");
        return NULL;
    }

    errors.panic(SUICIDE);
    return NULL;
}
llvm::Value* CastExp::cast_to_valueptr(Program& program, Function& func, AssemblyData& context){
    std::string type_name;
    llvm::Value* llvm_value = value->assemble_immutable(program, func, context, &type_name);
    if(llvm_value == NULL) return NULL;

    // If the expression doesn't have a type, something very bad has gone wrong
    if(type_name == "") return NULL;

    if(!Program::is_pointer_typename(type_name)){
        errors.panic("Can't cast non-pointer type '" + type_name + "' to pointer type '" + target_typename);
        return NULL;
    }

    llvm::Type* target_llvm_type;
    if(program.find_type(target_typename, context, &target_llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(target_typename));
        return NULL;
    }

    return context.builder.CreateBitCast(llvm_value, target_llvm_type);
}

FuncptrExp::FuncptrExp(ErrorHandler& err){
    flags = 0x00;
    errors = err;
}
FuncptrExp::FuncptrExp(const std::string& function_name, const std::vector<std::string>& function_arguments, ErrorHandler& err){
    this->function_name = function_name;
    this->function_arguments = function_arguments;
    flags = 0x00;
    errors = err;
}
FuncptrExp::FuncptrExp(const FuncptrExp& other) : PlainExp(other) {
    this->function_name = other.function_name;
    this->function_arguments = other.function_arguments;
    flags = 0x00;
}
FuncptrExp::~FuncptrExp(){}
llvm::Value* FuncptrExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    External function_data;
    std::string final_name;
    std::string args_str;

    // Stuff used for dirty hack to find variable argument function
    std::string* final_argument;
    bool did_dirty_hack = false;

    for(size_t a = 0; a != function_arguments.size(); a++){
        args_str += function_arguments[a];
        if(a + 1 != function_arguments.size()) args_str += ", ";
    }

    if(function_arguments.size() > 0){
        final_argument = &(function_arguments[function_arguments.size()-1]);
        if(final_argument->size() > 3 && final_argument->substr(0, 3) == "..."){
            // HACK: Very dirty and overall bad, but it does the job
            *final_argument = "[]" + final_argument->substr(3, final_argument->size()-3);
            did_dirty_hack = true;
        }
    }

    if(program.find_func(function_name, function_arguments, &function_data, did_dirty_hack) != 0){
        if(did_dirty_hack){
            // HACK: Very dirty, but hey it does the job
            *final_argument = "..." + final_argument->substr(2, final_argument->size()-2);
        }

        errors.panic_undeclared_func(function_name, function_arguments);
        return NULL;
    }

    final_name = (function_data.flags & EXTERN_MANGLED ? mangle(function_name, function_arguments) : function_name);
    llvm::Function* target_function = context.module->getFunction(final_name);

    if(function_data.flags & EXTERN_MULRET){
        if(expr_type != NULL){
            std::string return_types_str = "(";
            std::vector<std::string>& additional_returns = function_data.extra_return_types;
            size_t extra_ret_size = additional_returns.size();

            for(size_t i = 0; i != extra_ret_size; i++){
                return_types_str += additional_returns[i];
                if(i + 1 != extra_ret_size) return_types_str += ", ";
            }

            return_types_str += ")";
            *expr_type = "def(" + args_str + ")" + return_types_str;
        }
        return target_function;
    } else {
        if(expr_type != NULL) *expr_type = "def(" + args_str + ") " + function_data.return_type;
        return target_function;
    }
}
std::string FuncptrExp::toString(){
    std::string args_str;

    for(size_t i = 0; i != this->function_arguments.size(); i++){
        args_str += this->function_arguments[i];
        if(i + 1 != this->function_arguments.size()) args_str += ", ";
    }

    return "funcptr " + this->function_name + "(" + args_str + ")";
}
PlainExp* FuncptrExp::clone(){
    return new FuncptrExp(*this);
}

SizeofExp::SizeofExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
SizeofExp::SizeofExp(const std::string& type_name, ErrorHandler& err){
    this->type_name = type_name;
    flags = EXP_CONSTANT;
    errors = err;
}
SizeofExp::SizeofExp(const SizeofExp& other) : PlainExp(other) {
    this->type_name = other.type_name;
    flags = EXP_CONSTANT;
}
SizeofExp::~SizeofExp(){}
llvm::Value* SizeofExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "usize";
    llvm::Type* llvm_type;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(program.find_type(type_name, context, &llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(type_name));
        return NULL;
    }

    uint64_t type_size = context.module->getDataLayout().getTypeAllocSize(llvm_type);
    return llvm::ConstantInt::get(context.context, llvm::APInt(32, type_size, false));
}
std::string SizeofExp::toString(){
    return "sizeof " + this->type_name;
}
PlainExp* SizeofExp::clone(){
    return new SizeofExp(*this);
}

AllocExp::AllocExp(ErrorHandler& err){
    flags = 0x00;
    errors = err;
}
AllocExp::AllocExp(const std::string& type_name, ErrorHandler& err){
    this->type_name = type_name;
    this->amount = 1;
    this->element_amount = 0;
    flags = 0x00;
    errors = err;
}
AllocExp::AllocExp(const std::string& type_name, size_t amount, ErrorHandler& err){
    this->type_name = type_name;
    this->amount = amount;
    this->element_amount = 0;
    flags = 0x00;
    errors = err;
}
AllocExp::AllocExp(const std::string& type_name, size_t amount, size_t element_amount, ErrorHandler& err){
    this->type_name = type_name;
    this->amount = amount;
    this->element_amount = element_amount;
    flags = 0x00;
    errors = err;
}
AllocExp::AllocExp(const AllocExp& other) : PlainExp(other) {
    this->type_name = other.type_name;
    this->amount = other.amount;
    this->element_amount = other.element_amount;
    flags = 0x00;
}
AllocExp::~AllocExp(){}
llvm::Value* AllocExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(element_amount == 0){
        // Assemble without creating as a high level array
        return this->assemble_plain(program, func, context, expr_type);
    }
    else {
        // Assemble by creating high level array
        return this->assemble_elements(program, func, context, expr_type);
    }
}
std::string AllocExp::toString(){
    return "new " + this->type_name + (amount != 1 ? " * " + to_str(amount) : "");
}
PlainExp* AllocExp::clone(){
    return new AllocExp(*this);
}
llvm::Value* AllocExp::assemble_plain(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "*" + type_name;
    llvm::Type* llvm_type;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(program.find_type(type_name, context, &llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(type_name));
        return NULL;
    }

    llvm::Function* malloc_function = context.module->getFunction("malloc");

    if(!malloc_function){
        // Declare the malloc function if it doesn't already exist
        llvm::Type* return_llvm_type;

        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt32Ty(context.context);
        return_llvm_type = llvm::Type::getInt8PtrTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
        malloc_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "malloc", context.module.get());
    }

    uint64_t type_size = context.module->getDataLayout().getTypeAllocSize(llvm_type);
    std::vector<llvm::Value*> malloc_args(1);

    malloc_args[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, type_size * amount, false));
    llvm::Value* heap_memory = context.builder.CreateCall(malloc_function, malloc_args);

    // Special casting if allocated type is a function pointer
    if(Program::is_function_typename(type_name)){
        llvm::Type* llvm_func_type;
        if(program.function_typename_to_type(type_name, context, &llvm_func_type) != 0) return NULL;
        heap_memory = context.builder.CreateBitCast(heap_memory, llvm_func_type->getPointerTo());
        return heap_memory;
    }

    // Otherwise standard pointer cast
    return context.builder.CreateBitCast(heap_memory, llvm_type->getPointerTo());
}
llvm::Value* AllocExp::assemble_elements(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "[]" + type_name;
    llvm::Type* llvm_type;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(program.find_type(type_name, context, &llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(type_name));
        return NULL;
    }

    llvm::Function* malloc_function = context.module->getFunction("malloc");

    if(!malloc_function){
        // Declare the malloc function if it doesn't already exist
        llvm::Type* return_llvm_type;;

        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt32Ty(context.context);
        return_llvm_type = llvm::Type::getInt8PtrTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
        malloc_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "malloc", context.module.get());
    }

    uint64_t type_size = context.module->getDataLayout().getTypeAllocSize(llvm_type);
    std::vector<llvm::Value*> malloc_args(1);

    malloc_args[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, type_size * amount * element_amount, false));
    llvm::Value* heap_memory = context.builder.CreateCall(malloc_function, malloc_args);

    std::vector<llvm::Value*> ctor_args = { heap_memory, llvm::ConstantInt::get(context.context, llvm::APInt(32, element_amount, false)) };
    return context.builder.CreateCall(program.llvm_array_ctor, ctor_args);
}

DynamicAllocExp::DynamicAllocExp(ErrorHandler& err){
    flags = 0x00;
    errors = err;
}
DynamicAllocExp::DynamicAllocExp(const std::string& type_name, ErrorHandler& err){
    this->type_name = type_name;
    this->amount = 1;
    this->element_amount = 0;
    flags = 0x00;
    errors = err;
}
DynamicAllocExp::DynamicAllocExp(const std::string& type_name, PlainExp* amount, ErrorHandler& err){
    this->type_name = type_name;
    this->amount = amount;
    this->element_amount = 0;
    flags = 0x00;
    errors = err;
}
DynamicAllocExp::DynamicAllocExp(const std::string& type_name, PlainExp* amount, size_t element_amount, ErrorHandler& err){
    this->type_name = type_name;
    this->amount = amount;
    this->element_amount = element_amount;
    flags = 0x00;
    errors = err;
}
DynamicAllocExp::DynamicAllocExp(const DynamicAllocExp& other) : PlainExp(other) {
    this->type_name = other.type_name;
    this->amount = other.amount;
    this->element_amount = other.element_amount;
    flags = 0x00;
}
DynamicAllocExp::~DynamicAllocExp(){}
llvm::Value* DynamicAllocExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(element_amount == 0){
        // Assemble without creating as a high level array
        return this->assemble_plain(program, func, context, expr_type);
    }
    else {
        // Assemble by creating high level array
        return this->assemble_elements(program, func, context, expr_type);
    }
}
std::string DynamicAllocExp::toString(){
    return "new " + this->type_name + (amount != 1 ? " * " + amount->toString() : "");
}
PlainExp* DynamicAllocExp::clone(){
    return new DynamicAllocExp(*this);
}
llvm::Value* DynamicAllocExp::assemble_plain(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "*" + type_name;
    llvm::Type* llvm_type;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(program.find_type(type_name, context, &llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(type_name));
        return NULL;
    }

    llvm::Function* malloc_function = context.module->getFunction("malloc");

    if(!malloc_function){
        // Declare the malloc function if it doesn't already exist
        llvm::Type* return_llvm_type;;

        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt32Ty(context.context);
        return_llvm_type = llvm::Type::getInt8PtrTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
        malloc_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "malloc", context.module.get());
    }

    uint64_t type_size = context.module->getDataLayout().getTypeAllocSize(llvm_type);
    std::vector<llvm::Value*> malloc_args(1);

    std::string amount_typename;
    llvm::Value* amount_llvm_value = amount->assemble_immutable(program, func, context, &amount_typename);
    if(amount_llvm_value == NULL) return NULL;

    program.resolve_if_alias(amount_typename);
    if(!Program::is_integer_typename(amount_typename)){
        errors.panic("The expression type specified to 'new " + this->type_name + " * ' isn't an integer.\n    Received the type: '" + amount_typename + "'");
        return NULL;
    }

    malloc_args[0] = context.builder.CreateMul(llvm::ConstantInt::get(context.context, llvm::APInt(32, type_size, false)), amount_llvm_value);
    llvm::Value* heap_memory = context.builder.CreateCall(malloc_function, malloc_args);

    // Special casting if allocated type is a function pointer
    if(Program::is_function_typename(type_name)){
        llvm::Type* llvm_func_type;
        if(program.function_typename_to_type(type_name, context, &llvm_func_type) != 0) return NULL;
        heap_memory = context.builder.CreateBitCast(heap_memory, llvm_func_type->getPointerTo());
        return heap_memory;
    }

    // Otherwise standard pointer cast
    return context.builder.CreateBitCast(heap_memory, llvm_type->getPointerTo());
}
llvm::Value* DynamicAllocExp::assemble_elements(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    if(expr_type != NULL) *expr_type = "[]" + type_name;
    llvm::Type* llvm_type;

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(program.find_type(type_name, context, &llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(type_name));
        return NULL;
    }

    llvm::Function* malloc_function = context.module->getFunction("malloc");

    if(!malloc_function){
        // Declare the malloc function if it doesn't already exist
        llvm::Type* return_llvm_type;;

        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt32Ty(context.context);
        return_llvm_type = llvm::Type::getInt8PtrTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, args, false);
        malloc_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "malloc", context.module.get());
    }

    uint64_t type_size = context.module->getDataLayout().getTypeAllocSize(llvm_type);
    std::vector<llvm::Value*> malloc_args(1);

    std::string amount_typename;
    llvm::Value* amount_llvm_value = amount->assemble_immutable(program, func, context, &amount_typename);
    if(amount_llvm_value == NULL) return NULL;

    program.resolve_if_alias(amount_typename);
    if(!Program::is_integer_typename(amount_typename)){
        errors.panic("The expression type specified to 'new [...] " + this->type_name + " * ' isn't an integer.\n    Received the type: '" + amount_typename + "'");
        return NULL;
    }

    malloc_args[0] = context.builder.CreateMul(llvm::ConstantInt::get(context.context, llvm::APInt(32, type_size * element_amount, false)), amount_llvm_value);
    llvm::Value* heap_memory = context.builder.CreateCall(malloc_function, malloc_args);

    std::vector<llvm::Value*> ctor_args = { heap_memory, llvm::ConstantInt::get(context.context, llvm::APInt(32, element_amount, false)) };
    return context.builder.CreateCall(program.llvm_array_ctor, ctor_args);
}

ArrayDataExp::ArrayDataExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
ArrayDataExp::ArrayDataExp(const std::vector<PlainExp*>& elements, ErrorHandler& err){
    this->elements = elements;
    flags = EXP_CONSTANT;
    errors = err;
}
ArrayDataExp::ArrayDataExp(const ArrayDataExp& other) : PlainExp(other) {
    this->elements.resize(other.elements.size());
    flags = EXP_CONSTANT;

    for(size_t i = 0; i != other.elements.size(); i++){
        this->elements[i] = other.elements[i]->clone();
    }
}
ArrayDataExp::~ArrayDataExp(){
    for(size_t i = 0; i != this->elements.size(); i++){
        delete this->elements[i];
    }
}
llvm::Value* ArrayDataExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    llvm::Type* global_llvm_type;
    llvm::GlobalVariable* global_data;

    llvm::Type* element_type = program.llvm_array_type;
    std::vector<llvm::Constant*> values;
    std::string element_typename;

    if(elements.size() == 0){
        errors.panic("Array data constant must have at least one element");
        return NULL;
    }

    for(size_t i = 0; i != elements.size(); i++){
        llvm::Value* val;
        std::string val_typename;

        val = elements[i]->assemble_immutable(program, func, context, &val_typename);
        if(val == NULL) return NULL;

        if(!(elements[i]->flags & EXP_CONSTANT)){
            errors.panic("Can't create constant data array from non-constant values");
            return NULL;
        }

        if(i == 0){
            // Make a note that this should be the type for each constant expression
            element_typename = val_typename;

            // Get the llvm type for the typename
            if(program.find_type(val_typename, context, &element_type) != 0){
                errors.panic(UNDECLARED_TYPE(val_typename));
                return NULL;
            }
        } else if(assemble_merge_types_oneway(context, program, val_typename, element_typename, &val, element_type, NULL) != 0){
            // Type doesn't match previous types
            errors.panic(INCOMPATIBLE_TYPES(val_typename, element_typename));
            return NULL;
        }

        llvm::Constant* constant_value = static_cast<llvm::Constant*>(val);
        values.push_back(constant_value);
    }

    llvm::ArrayType* array_type = llvm::ArrayType::get(element_type, values.size());
    llvm::Constant* constant = llvm::ConstantArray::get(array_type, values);

    global_data = new llvm::GlobalVariable(*(context.module.get()), array_type, true,
                                           llvm::GlobalVariable::LinkageTypes::InternalLinkage, constant, ".constdata");

    global_data = context.builder.CreateBitCast(global_data, element_type->getPointerTo());
    if(expr_type != NULL) *expr_type = "*" + element_typename;
    return global_data;
}
std::string ArrayDataExp::toString(){
    std::string data_contents = "{";

    for(size_t i = 0; i != this->elements.size(); i++){
        data_contents += this->elements[i]->toString();
        if(i + 1 != this->elements.size()) data_contents += ", ";
    }

    data_contents += "}";
    return data_contents;
}
PlainExp* ArrayDataExp::clone(){
    return new ArrayDataExp(*this);
}

RetrieveConstantExp::RetrieveConstantExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
RetrieveConstantExp::RetrieveConstantExp(const std::string& val, ErrorHandler& err){
    value = val;
    flags = EXP_CONSTANT;
    errors = err;
}
RetrieveConstantExp::RetrieveConstantExp(const RetrieveConstantExp& other) : PlainExp(other) {
    value = other.value;
    flags = EXP_CONSTANT;
}
RetrieveConstantExp::~RetrieveConstantExp(){}
llvm::Value* RetrieveConstantExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){
    Constant constant;
    PlainExp* constant_expression;
    bool top_level_constant = !context.assembling_constant_expression;

    if(top_level_constant){
        context.assembling_constant_expression = true;
        context.constant_expression_depth = 0;
    }
    else {
        if(context.constant_expression_depth++ == 64){
            // NOTE: This isn't the most elegant method of detecting infinite loops in constant expressions, but it's a whole lot faster
            //           than trying to validate every single constant expression and overall constant expressions shouldn't be used for recursive computation anyway (like fib).
            //           It works well enough and should never really be a problem. The biggest advantage of doing it this way is faster compile-times
            //               (Which checking every single constant expression would be very expensive especially when you have 100s to 1000s of constants in some libraries)
            // TLDR: It's not an "all inclusive solution", but it does the job and doesn't have massive performance penalties. - Isaac Shelton July 6 2017
            errors.panic("Recursion depth of constant expression exceeded max depth of 64\n    (The constant expression most likely contains itself)");
            return NULL;
        };
    }

    if(program.find_const(value, &constant) != 0){
        errors.panic(UNDECLARED_CONST(value));
        return NULL;
    }

    if(top_level_constant){
        llvm::Value* val = constant.value->assemble(program, func, context, expr_type);
        context.assembling_constant_expression = false;
        return val;
    }
    else {
        return constant.value->assemble(program, func, context, expr_type);
    }
}
std::string RetrieveConstantExp::toString(){
    return "$" + value;
}
PlainExp* RetrieveConstantExp::clone(){
    return new RetrieveConstantExp(*this);
}

NamespaceExp::NamespaceExp(ErrorHandler& err){
    flags = EXP_CONSTANT;
    errors = err;
}
NamespaceExp::NamespaceExp(const std::string& name_space, const std::string& member, ErrorHandler& err){
    this->name_space = name_space;
    this->member = member;
    flags = EXP_CONSTANT;
    errors = err;
}
NamespaceExp::NamespaceExp(const NamespaceExp& other) : PlainExp(other) {
    this->name_space = other.name_space;
    this->member = other.member;
    flags = EXP_CONSTANT;
}
NamespaceExp::~NamespaceExp(){}
llvm::Value* NamespaceExp::assemble(Program& program, Function& func, AssemblyData& context, std::string* expr_type){

    for(Enum& inum : program.enums){
        if(inum.name == name_space){
            for(size_t i = 0; i != inum.fields.size(); i++){
                if(inum.fields[i].name == member){
                    if(expr_type != NULL) *expr_type = inum.name;
                    return llvm::ConstantInt::get(context.context, llvm::APInt(inum.bits, (uint64_t) i, false));
                }
            }

            errors.panic("No member named '" + member + "' was found in the namespace '" + name_space + "'");
            return NULL;
        }
    }

    errors.panic("No namespace named '" + name_space + "' was found");
    return NULL;
}
std::string NamespaceExp::toString(){
    return name_space + "::" + member;
}
PlainExp* NamespaceExp::clone(){
    return new NamespaceExp(*this);
}
