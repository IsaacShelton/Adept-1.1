
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

#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/mangling.h"
#include "../include/statement.h"

Statement::Statement(){}
Statement::Statement(ErrorHandler& errors){
    this->errors = errors;
}
Statement::Statement(const Statement& other){
    this->errors = other.errors;
}
Statement::~Statement(){}
bool Statement::isConditional(){
    return false;
}

DeclareStatement::DeclareStatement(ErrorHandler& errors){
    this->errors = errors;
}
DeclareStatement::DeclareStatement(const std::string& variable_name, const std::string& variable_type, ErrorHandler& errors){
    this->variable_name = variable_name;
    this->variable_type = variable_type;
    this->errors = errors;
}
DeclareStatement::DeclareStatement(const DeclareStatement& other) : Statement(other) {
    this->variable_name = other.variable_name;
    this->variable_type = other.variable_type;
}
DeclareStatement::~DeclareStatement(){}
int DeclareStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Type* variable_llvmtype;

    if(func.find_variable(this->variable_name, NULL) == 0){
        errors.panic(DUPLICATE_VARIBLE(this->variable_name));
        return 1;
    }

    if(program.find_type(this->variable_type, &variable_llvmtype) != 0){
        errors.panic( UNDECLARED_TYPE(this->variable_type) );
        return 1;
    }

    llvm::AllocaInst* allocation_instance;
    llvm::BasicBlock* previous_block = context.builder.GetInsertBlock();

    context.builder.SetInsertPoint(func.asm_func.entry);
    allocation_instance = context.builder.CreateAlloca(variable_llvmtype, 0, this->variable_name.c_str());
    context.builder.SetInsertPoint(previous_block);

    func.variables.push_back( Variable{this->variable_name, this->variable_type, allocation_instance} );
    return 0;
}
std::string DeclareStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += variable_name + " " + variable_type;
    return result;
}
Statement* DeclareStatement::clone(){
    return new DeclareStatement(*this);
}
bool DeclareStatement::isTerminator(){
    return false;
}

DeclareAssignStatement::DeclareAssignStatement(ErrorHandler& errors){
    this->variable_value = NULL;
    this->errors = errors;
}
DeclareAssignStatement::DeclareAssignStatement(const std::string& variable_name, const std::string& variable_type, PlainExp* variable_value, ErrorHandler& errors){
    this->variable_name = variable_name;
    this->variable_type = variable_type;
    this->variable_value = variable_value;
    this->errors = errors;
}
DeclareAssignStatement::DeclareAssignStatement(const DeclareAssignStatement& other) : Statement(other) {
    this->variable_name = other.variable_name;
    this->variable_type = other.variable_type;
    this->variable_value = other.variable_value->clone();
}
DeclareAssignStatement::~DeclareAssignStatement(){
    delete this->variable_value;
}
int DeclareAssignStatement::assemble(Program& program, Function& func, AssembleContext& context){
    std::string expression_type;
    llvm::Type* llvm_type;

    if(func.find_variable(this->variable_name, NULL) == 0){
        errors.panic(DUPLICATE_VARIBLE(this->variable_name));
        return 1;
    }

    if(program.find_type(this->variable_type, &llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(this->variable_type) );
        return 1;
    }

    llvm::Value* llvm_value = variable_value->assemble_immutable(program, func, context, &expression_type);
    if(llvm_value == NULL) return 1;

    if(assemble_merge_types_oneway(context, program, expression_type, this->variable_type, &llvm_value, llvm_type, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_type, this->variable_type) );
        return 1;
    }

    llvm::AllocaInst* allocation_instance;
    llvm::BasicBlock* previous_block = context.builder.GetInsertBlock();

    context.builder.SetInsertPoint(func.asm_func.entry);
    allocation_instance = context.builder.CreateAlloca(llvm_type, 0, this->variable_name.c_str());

    context.builder.SetInsertPoint(previous_block);
    context.builder.CreateStore(llvm_value, allocation_instance);

    func.variables.push_back( Variable{this->variable_name, this->variable_type, allocation_instance} );
    return 0;
}
std::string DeclareAssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += variable_name + " " + variable_type + " = " + variable_value->toString();
    return result;
}
Statement* DeclareAssignStatement::clone(){
    return new DeclareAssignStatement(*this);
}
bool DeclareAssignStatement::isTerminator(){
    return false;
}

ReturnStatement::ReturnStatement(ErrorHandler& errors){
    this->return_value = NULL;
    this->errors = errors;
}
ReturnStatement::ReturnStatement(PlainExp* return_value, ErrorHandler& errors) {
    this->return_value = return_value;
    this->errors = errors;
}
ReturnStatement::ReturnStatement(const ReturnStatement& other) : Statement(other) {
    if(other.return_value != NULL){
        this->return_value = other.return_value->clone();
    } else {
        this->return_value = NULL;
    }
}
ReturnStatement::~ReturnStatement(){
    delete this->return_value;
}
int ReturnStatement::assemble(Program& program, Function& func, AssembleContext& context){
    if(this->return_value != NULL){
        std::string expression_type;
        llvm::Value* llvm_value = this->return_value->assemble_immutable(program, func, context, &expression_type);
        if(llvm_value == NULL) return 1;

        if(assemble_merge_types_oneway(context, program, expression_type, func.return_type, &llvm_value, func.asm_func.return_type, NULL) != 0){
            errors.panic( INCOMPATIBLE_TYPES(expression_type, func.return_type) );
            return 1;
        }

        context.builder.CreateStore(llvm_value, func.asm_func.exitval);
        context.builder.CreateBr(func.asm_func.quit);
    }
    else {
        context.builder.CreateBr(func.asm_func.quit);
    }
    return 0;
}
std::string ReturnStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    if(this->return_value == NULL){
        result += "return";
    } else {
        result += "return " + this->return_value->toString();
    }

    return result;
}
Statement* ReturnStatement::clone(){
    return new ReturnStatement(*this);
}
bool ReturnStatement::isTerminator(){
    return true;
}

AssignStatement::AssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
}
AssignStatement::AssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
}
AssignStatement::AssignStatement(const AssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
}
AssignStatement::~AssignStatement(){
    delete this->location;
    delete this->value;
}
int AssignStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!location->is_mutable){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, &location_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(location_typename) );
        return 1;
    }

    llvm_value = this->value->assemble_immutable(program, func, context, &expression_typename);

    if(llvm_value == NULL) return 1;

    // Merge expression type into required type if possible
    if(assemble_merge_types_oneway(context, program, expression_typename, location_typename, &llvm_value, location_llvm_type, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_typename, location_typename) );
        return 1;
    }

    // Store the final value
    context.builder.CreateStore(llvm_value, store_location);
    return 0;
}
std::string AssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->location->toString() + " = " + this->value->toString();
    return result;
}
Statement* AssignStatement::clone(){
    return new AssignStatement(*this);
}
bool AssignStatement::isTerminator(){
    return false;
}

AdditionAssignStatement::AdditionAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
}
AdditionAssignStatement::AdditionAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
}
AdditionAssignStatement::AdditionAssignStatement(const AdditionAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
}
AdditionAssignStatement::~AdditionAssignStatement(){
    delete this->location;
    delete this->value;
}
int AdditionAssignStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!location->is_mutable){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, &location_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(location_typename) );
        return 1;
    }

    llvm_value = this->value->assemble_immutable(program, func, context, &expression_typename);
    if(llvm_value == NULL) return 1;

    // Merge expression type into required type if possible
    if(assemble_merge_types_oneway(context, program, expression_typename, location_typename, &llvm_value, location_llvm_type, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_typename, location_typename) );
        return 1;
    }

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    // Perform the addition
    if(location_typename == "int" or location_typename == "uint" or location_typename == "short" or location_typename == "ushort"
       or location_typename == "long" or location_typename == "ulong" or location_typename == "byte"
       or location_typename == "ubyte" or location_typename == "bool"){
        llvm_value = context.builder.CreateAdd(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "float" or location_typename == "double"){
        llvm_value = context.builder.CreateFAdd(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* added_value = context.builder.CreateAdd(left_int, right_int, "addtmp");
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy(), "casttmp");
    }
    else {
        errors.panic("The '+=' operator can't be used on type '" + location_typename + "'");
        return 1;
    }

    // Store the final value
    context.builder.CreateStore(llvm_value, store_location);
    return 0;
}
std::string AdditionAssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->location->toString() + " += " + this->value->toString();
    return result;
}
Statement* AdditionAssignStatement::clone(){
    return new AdditionAssignStatement(*this);
}
bool AdditionAssignStatement::isTerminator(){
    return false;
}

SubtractionAssignStatement::SubtractionAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
}
SubtractionAssignStatement::SubtractionAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
}
SubtractionAssignStatement::SubtractionAssignStatement(const SubtractionAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
}
SubtractionAssignStatement::~SubtractionAssignStatement(){
    delete this->location;
    delete this->value;
}
int SubtractionAssignStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!location->is_mutable){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, &location_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(location_typename) );
        return 1;
    }

    llvm_value = this->value->assemble_immutable(program, func, context, &expression_typename);
    if(llvm_value == NULL) return 1;

    // Merge expression type into required type if possible
    if(assemble_merge_types_oneway(context, program, expression_typename, location_typename, &llvm_value, location_llvm_type, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_typename, location_typename) );
        return 1;
    }

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    // Perform the subtraction
    if(location_typename == "int" or location_typename == "uint" or location_typename == "short" or location_typename == "ushort"
       or location_typename == "long" or location_typename == "ulong" or location_typename == "byte"
       or location_typename == "ubyte" or location_typename == "bool"){
        llvm_value = context.builder.CreateSub(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "float" or location_typename == "double"){
        llvm_value = context.builder.CreateFSub(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* added_value = context.builder.CreateSub(left_int, right_int, "addtmp");
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy(), "casttmp");
    }
    else {
        errors.panic("The '-=' operator can't be used on type '" + location_typename + "'");
        return 1;
    }

    // Store the final value
    context.builder.CreateStore(llvm_value, store_location);
    return 0;
}
std::string SubtractionAssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->location->toString() + " -= " + this->value->toString();
    return result;
}
Statement* SubtractionAssignStatement::clone(){
    return new SubtractionAssignStatement(*this);
}
bool SubtractionAssignStatement::isTerminator(){
    return false;
}

MultiplicationAssignStatement::MultiplicationAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
}
MultiplicationAssignStatement::MultiplicationAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
}
MultiplicationAssignStatement::MultiplicationAssignStatement(const MultiplicationAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
}
MultiplicationAssignStatement::~MultiplicationAssignStatement(){
    delete this->location;
    delete this->value;
}
int MultiplicationAssignStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!location->is_mutable){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, &location_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(location_typename) );
        return 1;
    }

    llvm_value = this->value->assemble_immutable(program, func, context, &expression_typename);
    if(llvm_value == NULL) return 1;

    // Merge expression type into required type if possible
    if(assemble_merge_types_oneway(context, program, expression_typename, location_typename, &llvm_value, location_llvm_type, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_typename, location_typename) );
        return 1;
    }

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    // Perform the multiplication
    if(location_typename == "int" or location_typename == "uint" or location_typename == "short" or location_typename == "ushort"
       or location_typename == "long" or location_typename == "ulong" or location_typename == "byte"
       or location_typename == "ubyte" or location_typename == "bool"){
        llvm_value = context.builder.CreateMul(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "float" or location_typename == "double"){
        llvm_value = context.builder.CreateFMul(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* added_value = context.builder.CreateMul(left_int, right_int, "addtmp");
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy(), "casttmp");
    }
    else {
        errors.panic("The '*=' operator can't be used on type '" + location_typename + "'");
        return 1;
    }

    // Store the final value
    context.builder.CreateStore(llvm_value, store_location);
    return 0;
}
std::string MultiplicationAssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->location->toString() + " *= " + this->value->toString();
    return result;
}
Statement* MultiplicationAssignStatement::clone(){
    return new MultiplicationAssignStatement(*this);
}
bool MultiplicationAssignStatement::isTerminator(){
    return false;
}

DivisionAssignStatement::DivisionAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
}
DivisionAssignStatement::DivisionAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
}
DivisionAssignStatement::DivisionAssignStatement(const DivisionAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
}
DivisionAssignStatement::~DivisionAssignStatement(){
    delete this->location;
    delete this->value;
}
int DivisionAssignStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!location->is_mutable){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, &location_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(location_typename) );
        return 1;
    }

    llvm_value = this->value->assemble_immutable(program, func, context, &expression_typename);
    if(llvm_value == NULL) return 1;

    // Merge expression type into required type if possible
    if(assemble_merge_types_oneway(context, program, expression_typename, location_typename, &llvm_value, location_llvm_type, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_typename, location_typename) );
        return 1;
    }

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    // Perform the division
    if(location_typename == "int" or location_typename == "uint" or location_typename == "short" or location_typename == "ushort"
       or location_typename == "long" or location_typename == "ulong" or location_typename == "byte"
       or location_typename == "ubyte" or location_typename == "bool"){
        llvm_value = context.builder.CreateSDiv(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "float" or location_typename == "double"){
        llvm_value = context.builder.CreateFDiv(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* added_value = context.builder.CreateSDiv(left_int, right_int, "addtmp");
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy(), "casttmp");
    }
    else {
        errors.panic("The '/=' operator can't be used on type '" + location_typename + "'");
        return 1;
    }

    // Store the final value
    context.builder.CreateStore(llvm_value, store_location);
    return 0;
}
std::string DivisionAssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->location->toString() + " /= " + this->value->toString();
    return result;
}
Statement* DivisionAssignStatement::clone(){
    return new DivisionAssignStatement(*this);
}
bool DivisionAssignStatement::isTerminator(){
    return false;
}

ModulusAssignStatement::ModulusAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
}
ModulusAssignStatement::ModulusAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
}
ModulusAssignStatement::ModulusAssignStatement(const ModulusAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
}
ModulusAssignStatement::~ModulusAssignStatement(){
    delete this->location;
    delete this->value;
}
int ModulusAssignStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!location->is_mutable){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, &location_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(location_typename) );
        return 1;
    }

    llvm_value = this->value->assemble_immutable(program, func, context, &expression_typename);
    if(llvm_value == NULL) return 1;

    // Merge expression type into required type if possible
    if(assemble_merge_types_oneway(context, program, expression_typename, location_typename, &llvm_value, location_llvm_type, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_typename, location_typename) );
        return 1;
    }

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    // Perform the modulus operation
    if(location_typename == "int" or location_typename == "uint" or location_typename == "short" or location_typename == "ushort"
       or location_typename == "long" or location_typename == "ulong" or location_typename == "byte"
       or location_typename == "ubyte" or location_typename == "bool"){
        llvm_value = context.builder.CreateSRem(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "float" or location_typename == "double"){
        llvm_value = context.builder.CreateFRem(store_value, llvm_value, "addtmp");
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty(), "casttmp");
        llvm::Value* added_value = context.builder.CreateSRem(left_int, right_int, "addtmp");
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy(), "casttmp");
    }
    else {
        errors.panic("The '%=' operator can't be used on type '" + location_typename + "'");
        return 1;
    }

    // Store the final value
    context.builder.CreateStore(llvm_value, store_location);
    return 0;
}
std::string ModulusAssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->location->toString() + " %= " + this->value->toString();
    return result;
}
Statement* ModulusAssignStatement::clone(){
    return new ModulusAssignStatement(*this);
}
bool ModulusAssignStatement::isTerminator(){
    return false;
}

CallStatement::CallStatement(ErrorHandler& errors){
    this->errors = errors;
}
CallStatement::CallStatement(const std::string& name, const std::vector<PlainExp*>& args, ErrorHandler& errors){
    this->name = name;
    this->args = args;
    this->errors = errors;
}
CallStatement::CallStatement(const CallStatement& other) : Statement(other) {
    this->name = other.name;
    this->args.resize(other.args.size());

    for(size_t i = 0; i != args.size(); i++){
        args[i] = other.args[i]->clone();
    }
}
CallStatement::~CallStatement(){
    for(PlainExp* arg : args) delete arg;
}
int CallStatement::assemble(Program& program, Function& func, AssembleContext& context){
    External func_data;
    Variable func_variable;
    Global func_global;

    llvm::Value* expr_value;
    std::string expr_typename;
    llvm::Type* expected_arg_type;
    std::vector<llvm::Value*> argument_values;
    std::vector<std::string> argument_types;
    std::vector<llvm::Type*> argument_llvm_types;

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble_immutable(program, func, context, &expr_typename);
        if(expr_value == NULL) return 1;

        if(program.find_type(expr_typename, &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(expr_typename) );
            return 1;
        }

        argument_values.push_back(expr_value);
        argument_types.push_back(expr_typename);
        argument_llvm_types.push_back(expected_arg_type);
    }

    if(program.find_func(name, argument_types, &func_data) == 0){
        // Standard function exists

        std::string final_name = (func_data.is_mangled) ? mangle(name, func_data.arguments) : name;
        llvm::Function* target = context.module->getFunction(final_name);
        if (!target){
            errors.panic_undeclared_func(name, argument_types);
            return 1;
        }
        assert(func_data.arguments.size() == target->arg_size());

        if (target->arg_size() != args.size()){
            // NOTE: This error should never appear
            errors.panic("Incorrect function argument count for function '" + name + "'");
            return 1;
        }

        for(size_t i = 0; i != argument_values.size(); i++){
            if(program.find_type(func_data.arguments[i], &expected_arg_type) != 0){
                errors.panic( UNDECLARED_TYPE(func_data.arguments[i]) );
                return 1;
            }

            if(assemble_merge_types_oneway(context, program, argument_types[i], func_data.arguments[i], &argument_values[i], expected_arg_type, NULL) != 0){
                // NOTE: This error should never occur
                errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + func_data.toString() +
                     "\n    Expected type '" + func_data.arguments[i] + "' but received type '" + argument_types[i] + "'");
                return 1;
            }
        }

        llvm::CallInst* call = context.builder.CreateCall(target, argument_values, "calltmp");
        call->setCallingConv(func_data.is_stdcall ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);
        return 0;
    }

    if(func.find_variable(name, &func_variable) == 0){
        // Variable that could be function pointer exists
        if(Program::is_function_typename(func_variable.type)){
            // The variable is a function pointer

            std::string varfunc_return_typename;
            llvm::Type* varfunc_return_llvm_type;
            std::vector<std::string> varfunc_args;
            std::vector<llvm::Type*> varfunc_llvm_args;

            if(program.extract_function_pointer_info(func_variable.type, varfunc_llvm_args, &varfunc_return_llvm_type, varfunc_args,
                varfunc_return_typename) != 0) return 1;

            if (varfunc_args.size() != args.size()){
                errors.panic("Incorrect function argument count when calling '" + name + "'");
                return 1;
            }

            for(size_t i = 0; i != argument_values.size(); i++){
                if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                    errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + func_variable.type +
                         "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                    return 1;
                }
            }

            llvm::Value* function_address = context.builder.CreateLoad(func_variable.variable);
            llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values, "calltmp");

            if(Program::function_typename_is_stdcall(func_variable.type)){
                call->setCallingConv(llvm::CallingConv::X86_StdCall);
            } else {
                call->setCallingConv(llvm::CallingConv::C);
            }
            return 0;
        }
    }

    if(program.find_global(name, &func_global) == 0){
        if(Program::is_function_typename(func_global.type)){
            // The variable is a function pointer

            std::string varfunc_return_typename;
            llvm::Type* varfunc_return_llvm_type;
            std::vector<std::string> varfunc_args;
            std::vector<llvm::Type*> varfunc_llvm_args;

            if(program.extract_function_pointer_info(func_global.type, varfunc_llvm_args, &varfunc_return_llvm_type, varfunc_args,
                varfunc_return_typename) != 0) return 1;

            if (varfunc_args.size() != args.size()){
                errors.panic("Incorrect function argument count when calling '" + name + "'");
                return 1;
            }

            for(size_t i = 0; i != argument_values.size(); i++){
                if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                    errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + func_global.type +
                         "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                    return 1;
                }
            }

            llvm::Value* function_address = context.builder.CreateLoad(func_global.variable);
            llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values, "calltmp");

            if(Program::function_typename_is_stdcall(func_global.type)){
                call->setCallingConv(llvm::CallingConv::X86_StdCall);
            } else {
                call->setCallingConv(llvm::CallingConv::C);
            }
            return 0;
        }
    }

    errors.panic_undeclared_func(name, argument_types);
    return 1;
}
std::string CallStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->name + "(";
    for(size_t i = 0; i != this->args.size(); i++){
        result += this->args[i]->toString();
        if(i+1 != this->args.size()) result += ", ";
    }
    result += ")";

    return result;
}
Statement* CallStatement::clone(){
    return new CallStatement(*this);
}
bool CallStatement::isTerminator(){
    return false;
}

MemberCallStatement::MemberCallStatement(ErrorHandler& errors){
    this->errors = errors;
}
MemberCallStatement::MemberCallStatement(PlainExp* object, const std::string& name, const std::vector<PlainExp*>& args, ErrorHandler& errors){
    this->object = object;
    this->name = name;
    this->args = args;
    this->errors = errors;
}
MemberCallStatement::MemberCallStatement(const MemberCallStatement& other) : Statement(other) {
    this->object = other.object->clone();
    this->name = other.name;
    this->args.resize(other.args.size());

    for(size_t i = 0; i != args.size(); i++){
        args[i] = other.args[i]->clone();
    }
}
MemberCallStatement::~MemberCallStatement(){
    delete object;
}
int MemberCallStatement::assemble(Program& program, Function& func, AssembleContext& context){
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
        object_value = context.builder.CreateLoad(object_value, "loadtmp");
        object_typename = object_typename.substr(1, object_typename.length()-1);
    }

    // Ensure the object is mutable
    if(!object->is_mutable){
        errors.panic("Can't call method of object because it is immutable");
        return 1;
    }

    // Get the llvm type for the object
    if(program.find_type(object_typename, &object_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(object_typename) );
        return 1;
    }

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble_immutable(program, func, context, &expr_typename);
        if(expr_value == NULL) return 1;

        if(program.find_type(expr_typename, &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(expr_typename) );
            return 1;
        }

        argument_values.push_back(expr_value);
        argument_types.push_back(expr_typename);
        argument_llvm_types.push_back(expected_arg_type);
    }

    if(program.find_method(object_typename, name, argument_types, &func_data) != 0){
        errors.panic_undeclared_method(object_typename, name, argument_types);
        return 1;
    }

    std::string parent_class_name = (func.parent_class_offset != 0) ? program.classes[func.parent_class_offset-1].name : "";

    // Ensure the function is public
    if(!func_data.is_public and parent_class_name != object_typename){
        errors.panic("The method '" + object_typename + "." + name + "' is private");
        return 1;
    }

    std::string final_name = mangle(object_typename, name, func_data.arguments);

    llvm::Function* target = context.module->getFunction(final_name);
    if (!target){
        errors.panic_undeclared_method(object_typename, name, argument_types);
        return 1;
    }

    argument_values.insert(argument_values.begin(), object_value);
    argument_types.insert(argument_types.begin(), "*" + object_typename);
    argument_llvm_types.insert(argument_llvm_types.begin(), object_llvm_type->getPointerTo());

    assert(func_data.arguments.size() == target->arg_size());

    if (target->arg_size() != args.size()+1){
        // NOTE: This error should never appear
        errors.panic("Incorrect method argument count for method '" + object_typename + "." + name + "'");
        return 1;
    }

    for(size_t i = 1; i != argument_values.size(); i++){
        if(program.find_type(func_data.arguments[i-1], &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(func_data.arguments[i-1]) );
            return 1;
        }

        if(assemble_merge_types_oneway(context, program, argument_types[i], func_data.arguments[i-1], &argument_values[i], expected_arg_type, NULL) != 0){
            // NOTE: This error should never occur
            errors.panic("Incorrect type for argument " + to_str(i+1) + " of method '" + object_typename + "." + name + "'\n    Definition: " + func_data.toString() +
                 "\n    Expected type '" + func_data.arguments[i-1] + "' but received type '" + argument_types[i] + "'");
            return 1;
        }
    }

    context.builder.CreateCall(target, argument_values, "calltmp");
    return 0;
}
std::string MemberCallStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += this->object->toString() + "." + this->name + "(";
    for(size_t i = 0; i != this->args.size(); i++){
        result += this->args[i]->toString();
        if(i+1 != this->args.size()) result += ", ";
    }
    result += ")";

    return result;
}
Statement* MemberCallStatement::clone(){
    return new MemberCallStatement(*this);
}
bool MemberCallStatement::isTerminator(){
    return false;
}

IfStatement::IfStatement(ErrorHandler& errors){
    this->errors = errors;
}
IfStatement::IfStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
}
IfStatement::IfStatement(const IfStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }
}
IfStatement::~IfStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }
}
int IfStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expr_typename, &expr_value);

    if(expr_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expr_value, true_block, false_block);
    context.builder.SetInsertPoint(true_block);

    bool terminated = false;
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        if(this->positive_statements[s]->isTerminator()) terminated = true;

        if(this->positive_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(false_block);
    context.builder.SetInsertPoint(false_block);
    return 0;
}
std::string IfStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "if " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* IfStatement::clone(){
    return new IfStatement(*this);
}
bool IfStatement::isTerminator(){
    return false;
}
bool IfStatement::isConditional(){
    return true;
}

UnlessStatement::UnlessStatement(ErrorHandler& errors){
    this->errors = errors;
}
UnlessStatement::UnlessStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
}
UnlessStatement::UnlessStatement(const UnlessStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }
}
UnlessStatement::~UnlessStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }
}
int UnlessStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expr_typename, &expr_value);

    if(expr_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expr_value, false_block, true_block);
    context.builder.SetInsertPoint(true_block);

    bool terminated = false;
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        if(this->positive_statements[s]->isTerminator()) terminated = true;

        if(this->positive_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(false_block);
    context.builder.SetInsertPoint(false_block);
    return 0;
}
std::string UnlessStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "unless " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* UnlessStatement::clone(){
    return new UnlessStatement(*this);
}
bool UnlessStatement::isTerminator(){
    return false;
}
bool UnlessStatement::isConditional(){
    return true;
}

WhileStatement::WhileStatement(ErrorHandler& errors){
    this->errors = errors;
}
WhileStatement::WhileStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
}
WhileStatement::WhileStatement(const WhileStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }
}
WhileStatement::~WhileStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }
}
int WhileStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* test_block = llvm::BasicBlock::Create(context.context, "test", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(test_block);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expr_typename, &expr_value);

    if(expr_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expr_value, true_block, false_block);
    context.builder.SetInsertPoint(true_block);

    bool terminated = false;
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        if(this->positive_statements[s]->isTerminator()) terminated = true;

        if(this->positive_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(false_block);
    return 0;
}
std::string WhileStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "while " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* WhileStatement::clone(){
    return new WhileStatement(*this);
}
bool WhileStatement::isTerminator(){
    return false;
}
bool WhileStatement::isConditional(){
    return true;
}

UntilStatement::UntilStatement(ErrorHandler& errors){
    this->errors = errors;
}
UntilStatement::UntilStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
}
UntilStatement::UntilStatement(const UntilStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }
}
UntilStatement::~UntilStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }
}
int UntilStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* test_block = llvm::BasicBlock::Create(context.context, "test", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(test_block);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expr_typename, &expr_value);

    if(expr_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expr_value, false_block, true_block);
    context.builder.SetInsertPoint(true_block);

    bool terminated = false;
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        if(this->positive_statements[s]->isTerminator()) terminated = true;

        if(this->positive_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(false_block);
    return 0;
}
std::string UntilStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "until " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* UntilStatement::clone(){
    return new UntilStatement(*this);
}
bool UntilStatement::isTerminator(){
    return false;
}
bool UntilStatement::isConditional(){
    return true;
}

IfElseStatement::IfElseStatement(ErrorHandler& errors){
    this->errors = errors;
}
IfElseStatement::IfElseStatement(PlainExp* condition, const StatementList& positive_statements, const StatementList& negative_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->negative_statements = negative_statements;
    this->errors = errors;
}
IfElseStatement::IfElseStatement(const IfElseStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->negative_statements.resize(other.negative_statements.size());
    this->errors = errors;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }
    for(size_t i = 0; i != this->negative_statements.size(); i++){
        this->negative_statements[i] = other.negative_statements[i]->clone();
    }
}
IfElseStatement::~IfElseStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }
    for(Statement* statement : negative_statements){
        delete statement;
    }
}
int IfElseStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);
    llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context.context, "continue", llvm_function);

    std::string expression_typename;
    llvm::Value* expression_value = this->condition->assemble_immutable(program, func, context, &expression_typename);
    if(expression_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expression_typename, &expression_value);

    if(expression_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expression_value, true_block, false_block);
    context.builder.SetInsertPoint(true_block);

    bool terminated = false;
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        if(this->positive_statements[s]->isTerminator()) terminated = true;

        if(this->positive_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(continue_block);
    context.builder.SetInsertPoint(false_block);

    terminated = false;
    for(size_t s = 0; s != this->negative_statements.size(); s++){
        if(this->negative_statements[s]->isTerminator()) terminated = true;

        if(this->negative_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(continue_block);
    context.builder.SetInsertPoint(continue_block);
    return 0;
}
std::string IfElseStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "if " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";

    if(this->negative_statements.size() == 1){
        if(this->negative_statements[0]->isConditional()){
            result += "} else " + this->negative_statements[0]->toString(indent, true);
            // Break early to avoid rest of "} else { ... }"
            return result;
        }
    }

    result += "} else {\n";
    for(size_t s = 0; s != this->negative_statements.size(); s++){
        result += this->negative_statements[s]->toString(indent+1, false) + "\n";
    }
    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}\n";

    return result;
}
Statement* IfElseStatement::clone(){
    return new IfElseStatement(*this);
}
bool IfElseStatement::isTerminator(){
    return false;
}
bool IfElseStatement::isConditional(){
    return true;
}

UnlessElseStatement::UnlessElseStatement(ErrorHandler& errors){
    this->errors = errors;
}
UnlessElseStatement::UnlessElseStatement(PlainExp* condition, const StatementList& positive_statements, const StatementList& negative_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->negative_statements = negative_statements;
    this->errors = errors;
}
UnlessElseStatement::UnlessElseStatement(const UnlessElseStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->negative_statements.resize(other.negative_statements.size());
    this->errors = errors;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }
    for(size_t i = 0; i != this->negative_statements.size(); i++){
        this->negative_statements[i] = other.negative_statements[i]->clone();
    }
}
UnlessElseStatement::~UnlessElseStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }
    for(Statement* statement : negative_statements){
        delete statement;
    }
}
int UnlessElseStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);
    llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context.context, "continue", llvm_function);

    std::string expression_typename;
    llvm::Value* expression_value = this->condition->assemble_immutable(program, func, context, &expression_typename);
    if(expression_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expression_typename, &expression_value);

    if(expression_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expression_value, false_block, true_block);
    context.builder.SetInsertPoint(true_block);

    bool terminated = false;
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        if(this->positive_statements[s]->isTerminator()) terminated = true;

        if(this->positive_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(continue_block);
    context.builder.SetInsertPoint(false_block);

    terminated = false;
    for(size_t s = 0; s != this->negative_statements.size(); s++){
        if(this->negative_statements[s]->isTerminator()) terminated = true;

        if(this->negative_statements[s]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(continue_block);
    context.builder.SetInsertPoint(continue_block);
    return 0;
}
std::string UnlessElseStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "unless " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";

    if(this->negative_statements.size() == 1){
        if(this->negative_statements[0]->isConditional()){
            result += "} else " + this->negative_statements[0]->toString(indent, true);
            // Break early to avoid rest of "} else { ... }"
            return result;
        }
    }

    result += "} else {\n";
    for(size_t s = 0; s != this->negative_statements.size(); s++){
        result += this->negative_statements[s]->toString(indent+1, false) + "\n";
    }
    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}\n";

    return result;
}
Statement* UnlessElseStatement::clone(){
    return new UnlessElseStatement(*this);
}
bool UnlessElseStatement::isTerminator(){
    return false;
}
bool UnlessElseStatement::isConditional(){
    return true;
}

DeallocStatement::DeallocStatement(ErrorHandler& errors){
    this->errors = errors;
}
DeallocStatement::DeallocStatement(PlainExp* value, ErrorHandler& errors){
    this->value = value;
    this->errors = errors;
}
DeallocStatement::DeallocStatement(const DeallocStatement& other) : Statement(other) {
    this->value = other.value->clone();
    this->errors = other.errors;
}
DeallocStatement::~DeallocStatement(){
    delete this->value;
}
int DeallocStatement::assemble(Program& program, Function& func, AssembleContext& context){
    llvm::Type* llvm_type;
    std::string type_name;
    llvm::Value* pointer = value->assemble_immutable(program, func, context, &type_name);

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(program.find_type(type_name, &llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(type_name));
        return 1;
    }

    llvm::Function* free_function = context.module->getFunction("free");

    if(!free_function){
        // Declare the malloc function if it doesn't already exist
        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt8PtrTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm::Type::getInt8Ty(context.context), args, false);
        free_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "free", context.module.get());
    }

    std::vector<llvm::Value*> free_args(1);
    free_args[0] = context.builder.CreateBitCast(pointer, llvm::Type::getInt8PtrTy(context.context), "casttmp");
    context.builder.CreateCall(free_function, free_args, "deltmp");
    return 0;
}
std::string DeallocStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "delete " + value->toString();
    return result;
}
Statement* DeallocStatement::clone(){
    return new DeallocStatement(*this);
}
bool DeallocStatement::isTerminator(){
    return false;
}
bool DeallocStatement::isConditional(){
    return false;
}
