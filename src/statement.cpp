
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
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/mangling.h"
#include "../include/statement.h"

Statement::Statement(){}
Statement::Statement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00;
}
Statement::Statement(const Statement& other){
    this->errors = other.errors;
}
Statement::~Statement(){}

DeclareStatement::DeclareStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00;
}
DeclareStatement::DeclareStatement(const std::string& variable_name, const std::string& variable_type, ErrorHandler& errors){
    this->variable_name = variable_name;
    this->variable_type = variable_type;
    this->errors = errors;
    this->flags = 0x00;
}
DeclareStatement::DeclareStatement(const DeclareStatement& other) : Statement(other) {
    this->variable_name = other.variable_name;
    this->variable_type = other.variable_type;
    this->flags = 0x00;
}
DeclareStatement::~DeclareStatement(){}
int DeclareStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Type* variable_llvmtype;

    if(context.current_function->findVariable(this->variable_name) != NULL){
        errors.panic(DUPLICATE_VARIBLE(this->variable_name));
        return 1;
    }

    if(program.find_type(this->variable_type, context, &variable_llvmtype) != 0){
        errors.panic( UNDECLARED_TYPE(this->variable_type) );
        return 1;
    }

    llvm::AllocaInst* allocation_instance;
    llvm::BasicBlock* previous_block = context.builder.GetInsertBlock();

    context.builder.SetInsertPoint(context.current_function->entry);
    allocation_instance = context.builder.CreateAlloca(variable_llvmtype, 0);

    context.current_function->addVariable(this->variable_name, this->variable_type, allocation_instance);
    context.builder.SetInsertPoint(previous_block);
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

DeclareAssignStatement::DeclareAssignStatement(ErrorHandler& errors){
    this->variable_value = NULL;
    this->errors = errors;
    this->flags = 0x00;
}
DeclareAssignStatement::DeclareAssignStatement(const std::string& variable_name, const std::string& variable_type, PlainExp* variable_value, ErrorHandler& errors){
    this->variable_name = variable_name;
    this->variable_type = variable_type;
    this->variable_value = variable_value;
    this->errors = errors;
    this->flags = 0x00;
}
DeclareAssignStatement::DeclareAssignStatement(const DeclareAssignStatement& other) : Statement(other) {
    this->variable_name = other.variable_name;
    this->variable_type = other.variable_type;
    this->variable_value = other.variable_value->clone();
    this->flags = 0x00;
}
DeclareAssignStatement::~DeclareAssignStatement(){
    delete this->variable_value;
}
int DeclareAssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    std::string expression_type;
    llvm::Type* variable_llvmtype;

    if(context.current_function->findVariable(this->variable_name) != NULL){
        errors.panic(DUPLICATE_VARIBLE(this->variable_name));
        return 1;
    };

    if(program.find_type(this->variable_type, context, &variable_llvmtype) != 0){
        errors.panic( UNDECLARED_TYPE(this->variable_type) );
        return 1;
    }

    llvm::AllocaInst* allocation_instance;
    llvm::BasicBlock* previous_block = context.builder.GetInsertBlock();

    context.builder.SetInsertPoint(context.current_function->entry);
    allocation_instance = context.builder.CreateAlloca(variable_llvmtype, 0);

    context.builder.SetInsertPoint(previous_block);
    llvm::Value* llvm_value = variable_value->assemble_immutable(program, func, context, &expression_type);
    if(llvm_value == NULL) return 1;

    if(assemble_merge_types_oneway(context, program, expression_type, this->variable_type, &llvm_value, variable_llvmtype, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_type, this->variable_type) );
        return 1;
    }

    context.builder.CreateStore(llvm_value, allocation_instance);

    context.current_function->addVariable(this->variable_name, this->variable_type, allocation_instance);
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

MultiDeclareStatement::MultiDeclareStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00;
}
MultiDeclareStatement::MultiDeclareStatement(const std::vector<std::string>& variable_names, const std::string& variable_type, ErrorHandler& errors){
    this->variable_names = variable_names;
    this->variable_type = variable_type;
    this->errors = errors;
    this->flags = 0x00;
}
MultiDeclareStatement::MultiDeclareStatement(const MultiDeclareStatement& other) : Statement(other) {
    this->variable_names = other.variable_names;
    this->variable_type = other.variable_type;
    this->flags = 0x00;
}
MultiDeclareStatement::~MultiDeclareStatement(){}
int MultiDeclareStatement::assemble(Program& program, Function& func, AssemblyData& context){
    std::string variable_name;
    llvm::Type* variable_llvmtype;
    llvm::AllocaInst* allocation_instance;
    llvm::BasicBlock* previous_block = context.builder.GetInsertBlock();

    // Set block insertion point
    context.builder.SetInsertPoint(context.current_function->entry);

    // Get the llvm type for the variables
    if(program.find_type(this->variable_type, context, &variable_llvmtype) != 0){
        errors.panic( UNDECLARED_TYPE(this->variable_type) );
        return 1;
    }

    for(const std::string& variable_name : this->variable_names){
        if(context.current_function->findVariable(variable_name) != NULL){
            errors.panic(DUPLICATE_VARIBLE(variable_name));
            return 1;
        }

        allocation_instance = context.builder.CreateAlloca(variable_llvmtype, 0);
        context.current_function->addVariable(variable_name, this->variable_type, allocation_instance);
    }

    // Reset block insertion point
    context.builder.SetInsertPoint(previous_block);
    return 0;
}
std::string MultiDeclareStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    for(size_t i = 0; i != variable_names.size(); i++){
        result += variable_names[i];
        if(i + 1 != variable_names.size()) result += ", ";
    }

    result += " " + variable_type;
    return result;
}
Statement* MultiDeclareStatement::clone(){
    return new MultiDeclareStatement(*this);
}

MultiDeclareAssignStatement::MultiDeclareAssignStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00;
}
MultiDeclareAssignStatement::MultiDeclareAssignStatement(const std::vector<std::string>& variable_names, const std::string& variable_type, PlainExp* variable_value, ErrorHandler& errors){
    this->variable_names = variable_names;
    this->variable_type = variable_type;
    this->variable_value = variable_value;
    this->errors = errors;
    this->flags = 0x00;
}
MultiDeclareAssignStatement::MultiDeclareAssignStatement(const MultiDeclareAssignStatement& other) : Statement(other) {
    this->variable_names = other.variable_names;
    this->variable_type = other.variable_type;
    this->variable_value = other.variable_value->clone();
    this->flags = 0x00;
}
MultiDeclareAssignStatement::~MultiDeclareAssignStatement(){
    delete this->variable_value;
}
int MultiDeclareAssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Value* llvm_value;
    std::string variable_name;
    std::string expression_type;
    llvm::Type* variable_llvmtype;
    llvm::AllocaInst* allocation_instance;
    llvm::BasicBlock* previous_block = context.builder.GetInsertBlock();

    // Assemble the value
    llvm_value = variable_value->assemble_immutable(program, func, context, &expression_type);
    if(llvm_value == NULL) return 1;

    // Get the llvm type for the variables
    if(program.find_type(this->variable_type, context, &variable_llvmtype) != 0){
        errors.panic( UNDECLARED_TYPE(this->variable_type) );
        return 1;
    }

    // Merge the value to fit the variable type
    if(assemble_merge_types_oneway(context, program, expression_type, this->variable_type, &llvm_value, variable_llvmtype, NULL) != 0){
        errors.panic( INCOMPATIBLE_TYPES(expression_type, this->variable_type) );
        return 1;
    }

    for(const std::string& variable_name : this->variable_names){
        if(context.current_function->findVariable(variable_name) != NULL){
            errors.panic(DUPLICATE_VARIBLE(variable_name));
            return 1;
        }

        context.builder.SetInsertPoint(context.current_function->entry);
        allocation_instance = context.builder.CreateAlloca(variable_llvmtype, 0);

        context.builder.SetInsertPoint(previous_block);
        context.builder.CreateStore(llvm_value, allocation_instance);

        context.current_function->addVariable(variable_name, this->variable_type, allocation_instance);
    }

    return 0;
}
std::string MultiDeclareAssignStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    for(size_t i = 0; i != variable_names.size(); i++){
        result += variable_names[i];
        if(i + 1 != variable_names.size()) result += ", ";
    }

    result += " " + variable_type + " = " + this->variable_value->toString();
    return result;
}
Statement* MultiDeclareAssignStatement::clone(){
    return new MultiDeclareAssignStatement(*this);
}

ReturnStatement::ReturnStatement(ErrorHandler& errors){
    this->return_value = NULL;
    this->errors = errors;
    this->flags = STMT_TERMINATOR;
}
ReturnStatement::ReturnStatement(PlainExp* return_value, ErrorHandler& errors) {
    this->return_value = return_value;
    this->errors = errors;
    this->flags = STMT_TERMINATOR;
}
ReturnStatement::ReturnStatement(const ReturnStatement& other) : Statement(other) {
    if(other.return_value != NULL){
        this->return_value = other.return_value->clone();
    } else {
        this->return_value = NULL;
    }
    this->flags = STMT_TERMINATOR;
}
ReturnStatement::~ReturnStatement(){
    delete this->return_value;
}
int ReturnStatement::assemble(Program& program, Function& func, AssemblyData& context){
    if(func.flags & FUNC_MULRET){
        errors.panic("Can't return a single value in a function that returns multiple values");
        return 1;
    }

    if(this->return_value != NULL){
        std::string expression_type;
        llvm::Value* llvm_value = this->return_value->assemble_immutable(program, func, context, &expression_type);
        if(llvm_value == NULL) return 1;

        if(assemble_merge_types_oneway(context, program, expression_type, func.return_type, &llvm_value, context.current_function->return_type, NULL) != 0){
            errors.panic( INCOMPATIBLE_TYPES(expression_type, func.return_type) );
            return 1;
        }

        context.builder.CreateStore(llvm_value, context.current_function->exitval);
        context.builder.CreateBr(context.current_function->quit);
    }
    else {
        context.builder.CreateBr(context.current_function->quit);
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

AssignStatement::AssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
    this->flags = 0x00;
}
AssignStatement::AssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
    this->flags = 0x00;
}
AssignStatement::AssignStatement(const AssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
    this->flags = 0x00;
}
AssignStatement::~AssignStatement(){
    delete this->location;
    delete this->value;
}
int AssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!(location->flags & EXP_MUTABLE)){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, context, &location_llvm_type) != 0){
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

AdditionAssignStatement::AdditionAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
    this->flags = 0x00;
}
AdditionAssignStatement::AdditionAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
    this->flags = 0x00;
}
AdditionAssignStatement::AdditionAssignStatement(const AdditionAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
    this->flags = 0x00;
}
AdditionAssignStatement::~AdditionAssignStatement(){
    delete this->location;
    delete this->value;
}
int AdditionAssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!(location->flags & EXP_MUTABLE)){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, context, &location_llvm_type) != 0){
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
    program.resolve_once_if_alias(location_typename);

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    if( (location_typename[0] == 'u' && (location_typename == "uint" or location_typename == "ushort" or location_typename == "ulong" or location_typename == "ubyte"))
     or (location_typename == "int" or location_typename == "short" or location_typename == "long" or location_typename == "byte") ){
        llvm_value = context.builder.CreateAdd(store_value, llvm_value);
    }
    else if(location_typename == "float" or location_typename == "double" or location_typename == "half"){
        llvm_value = context.builder.CreateFAdd(store_value, llvm_value);
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty());
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty());
        llvm::Value* added_value = context.builder.CreateAdd(left_int, right_int);
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy());
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

SubtractionAssignStatement::SubtractionAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
    this->flags = 0x00;
}
SubtractionAssignStatement::SubtractionAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
    this->flags = 0x00;
}
SubtractionAssignStatement::SubtractionAssignStatement(const SubtractionAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
    this->flags = 0x00;
}
SubtractionAssignStatement::~SubtractionAssignStatement(){
    delete this->location;
    delete this->value;
}
int SubtractionAssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!(location->flags & EXP_MUTABLE)){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, context, &location_llvm_type) != 0){
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
    program.resolve_once_if_alias(location_typename);

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    if( (location_typename[0] == 'u' && (location_typename == "uint" or location_typename == "ushort" or location_typename == "ulong" or location_typename == "ubyte"))
     or (location_typename == "int" or location_typename == "short" or location_typename == "long" or location_typename == "byte") ){
        llvm_value = context.builder.CreateSub(store_value, llvm_value);
    }
    else if(location_typename == "float" or location_typename == "double" or location_typename == "half"){
        llvm_value = context.builder.CreateFSub(store_value, llvm_value);
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty());
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty());
        llvm::Value* added_value = context.builder.CreateSub(left_int, right_int);
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy());
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

MultiplicationAssignStatement::MultiplicationAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
    this->flags = 0x00;
}
MultiplicationAssignStatement::MultiplicationAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
    this->flags = 0x00;
}
MultiplicationAssignStatement::MultiplicationAssignStatement(const MultiplicationAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
    this->flags = 0x00;
}
MultiplicationAssignStatement::~MultiplicationAssignStatement(){
    delete this->location;
    delete this->value;
}
int MultiplicationAssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!(location->flags & EXP_MUTABLE)){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, context, &location_llvm_type) != 0){
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
    program.resolve_once_if_alias(location_typename);

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    if( (location_typename[0] == 'u' && (location_typename == "uint" or location_typename == "ushort" or location_typename == "ulong" or location_typename == "ubyte"))
     or (location_typename == "int" or location_typename == "short" or location_typename == "long" or location_typename == "byte") ){
        llvm_value = context.builder.CreateMul(store_value, llvm_value);
    }
    else if(location_typename == "float" or location_typename == "double" or location_typename == "half"){
        llvm_value = context.builder.CreateFMul(store_value, llvm_value);
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty());
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty());
        llvm::Value* added_value = context.builder.CreateMul(left_int, right_int);
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy());
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

DivisionAssignStatement::DivisionAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
    this->flags = 0x00;
}
DivisionAssignStatement::DivisionAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
    this->flags = 0x00;
}
DivisionAssignStatement::DivisionAssignStatement(const DivisionAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
    this->flags = 0x00;
}
DivisionAssignStatement::~DivisionAssignStatement(){
    delete this->location;
    delete this->value;
}
int DivisionAssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!(location->flags & EXP_MUTABLE)){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, context, &location_llvm_type) != 0){
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
    program.resolve_once_if_alias(location_typename);

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    if(location_typename[0] == 'u' && (location_typename == "uint" or location_typename == "ushort" or location_typename == "ulong" or location_typename == "ubyte")){
        llvm_value = context.builder.CreateUDiv(store_value, llvm_value);
    }
    else if(location_typename == "int" or location_typename == "short" or location_typename == "long" or location_typename == "byte"){
        llvm_value = context.builder.CreateSDiv(store_value, llvm_value);
    }
    else if(location_typename == "float" or location_typename == "double" or location_typename == "half"){
        llvm_value = context.builder.CreateFDiv(store_value, llvm_value);
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty());
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty());
        llvm::Value* added_value = context.builder.CreateUDiv(left_int, right_int);
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy());
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

ModulusAssignStatement::ModulusAssignStatement(ErrorHandler& errors){
    this->location = NULL;
    this->value = NULL;
    this->errors = errors;
    this->flags = 0x00;
}
ModulusAssignStatement::ModulusAssignStatement(PlainExp* location, PlainExp* value, ErrorHandler& errors){
    this->location = location;
    this->value = value;
    this->errors = errors;
    this->flags = 0x00;
}
ModulusAssignStatement::ModulusAssignStatement(const ModulusAssignStatement& other) : Statement(other) {
    this->location = other.location->clone();
    this->value = other.value->clone();
    this->flags = 0x00;
}
ModulusAssignStatement::~ModulusAssignStatement(){
    delete this->location;
    delete this->value;
}
int ModulusAssignStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Value* store_value;
    llvm::Value* store_location;
    llvm::Value* llvm_value;
    llvm::Type* location_llvm_type;
    std::string location_typename;
    std::string expression_typename;

    if(!(location->flags & EXP_MUTABLE)){
        errors.panic("Can't assign immutable expression to a value");
        return 1;
    }

    store_location = this->location->assemble(program, func, context, &location_typename);
    if(store_location == NULL) return 1;

    // Find LLVM type for the storage location
    if(program.find_type(location_typename, context, &location_llvm_type) != 0){
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
    program.resolve_once_if_alias(location_typename);

    store_value = context.builder.CreateLoad(store_location);
    assert(location_typename.length() != 0);

    if(location_typename[0] == 'u' && (location_typename == "uint" or location_typename == "ushort" or location_typename == "ulong" or location_typename == "ubyte")){
        llvm_value = context.builder.CreateURem(store_value, llvm_value);
    }
    else if(location_typename == "int" or location_typename == "short" or location_typename == "long" or location_typename == "byte"){
        llvm_value = context.builder.CreateSRem(store_value, llvm_value);
    }
    else if(location_typename == "float" or location_typename == "double" or location_typename == "half"){
        llvm_value = context.builder.CreateFRem(store_value, llvm_value);
    }
    else if(location_typename == "ptr" or location_typename[0] == '*'){
        llvm::Value* left_int = context.builder.CreatePtrToInt(store_value, context.builder.getInt64Ty());
        llvm::Value* right_int = context.builder.CreatePtrToInt(llvm_value, context.builder.getInt64Ty());
        llvm::Value* added_value = context.builder.CreateURem(left_int, right_int);
        llvm_value = context.builder.CreateIntToPtr(added_value, context.builder.getInt8PtrTy());
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

CallStatement::CallStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00;
}
CallStatement::CallStatement(const std::string& name, const std::vector<PlainExp*>& args, ErrorHandler& errors){
    this->name = name;
    this->args = args;
    this->errors = errors;
    this->flags = 0x00;
}
CallStatement::CallStatement(const CallStatement& other) : Statement(other) {
    this->name = other.name;
    this->args.resize(other.args.size());
    this->flags = 0x00;

    for(size_t i = 0; i != args.size(); i++){
        args[i] = other.args[i]->clone();
    }
}
CallStatement::~CallStatement(){
    for(PlainExp* arg : args) delete arg;
}
int CallStatement::assemble(Program& program, Function& func, AssemblyData& context){
    External func_data;
    AssembleVariable* func_variable;
    Global global;
    AssembleGlobal* func_global;

    llvm::Value* expr_value;
    std::string expr_typename;
    llvm::Type* expected_arg_type;
    std::vector<llvm::Value*> argument_values;
    std::vector<std::string> argument_types;
    std::vector<llvm::Type*> argument_llvm_types;

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble_immutable(program, func, context, &expr_typename);
        if(expr_value == NULL) return 1;

        if(program.find_type(expr_typename, context, &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(expr_typename) );
            return 1;
        }

        argument_values.push_back(expr_value);
        argument_types.push_back(expr_typename);
        argument_llvm_types.push_back(expected_arg_type);
    }

    if(program.find_func(name, argument_types, &func_data) == 0){
        // Standard function exists

        if(func_data.flags & EXTERN_MULRET){
            errors.panic("No result variables specified for function '" + name + "' that returns multiple arguments");
            return 1;
        }

        std::string final_name = (func_data.flags & EXTERN_MANGLED) ? mangle(name, func_data.arguments) : name;
        llvm::Function* target = context.module->getFunction(final_name);
        if (!target){
            errors.panic_undeclared_func(name, argument_types);
            return 1;
        }
        assert(func_data.arguments.size() == target->arg_size());

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
        }

        return 0;
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
                varfunc_return_typename, NULL, flags) != 0) return 1;

            if(varfunc_return_llvm_type == NULL){
                errors.panic("No result variables specified for function pointer '" + name + "' that returns multiple arguments");
                return 1;
            }

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
                    return 1;
                }

                for(size_t i = 0; i != argument_values.size(); i++){
                    if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                        errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + func_variable->type +
                             "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                        return 1;
                    }
                }

                llvm::Value* function_address = context.builder.CreateLoad(func_variable->variable);
                llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values);

                if(Program::function_typename_is_stdcall(func_variable->type)){
                    call->setCallingConv(llvm::CallingConv::X86_StdCall);
                } else {
                    call->setCallingConv(llvm::CallingConv::C);
                }
                return 0;
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
                varfunc_return_typename, NULL, flags) != 0) return 1;

            if(varfunc_return_llvm_type == NULL){
                errors.panic("No result variables specified for function pointer '" + name + "' that returns multiple arguments");
                return 1;
            }

            if(flags & FUNC_VARARGS){
                AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );
                std::string va_arg_typename = varfunc_args[varfunc_args.size()-1];
                va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);
                uint64_t va_arg_count = args.size() - varfunc_args.size() + 1;
                AssembleGlobal* global_asm_data = context.findGlobal(global.name);

                if(global_asm_data == NULL){
                    errors.panic("Failed to find assembly data for global");
                    return 1;
                }

                return this->handle_var_args(program, func, context, va_arg_typename, va_arg_count, varfunc_args.size(),
                                        argument_values, context.builder.CreateLoad(global_asm_data->variable), Program::function_typename_is_stdcall(global.type));
            }
            else {
                if (varfunc_args.size() != args.size()){
                    errors.panic("Incorrect function argument count when calling '" + name + "' (" + to_str(args.size()) + " received, " + to_str(varfunc_args.size()) + " required)");
                    return 1;
                }

                for(size_t i = 0; i != argument_values.size(); i++){
                    if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                        errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + global.type +
                             "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                        return 1;
                    }
                }

                llvm::Value* function_address = context.builder.CreateLoad(context.findGlobal(name)->variable);
                llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values);

                if(Program::function_typename_is_stdcall(global.type)){
                    call->setCallingConv(llvm::CallingConv::X86_StdCall);
                } else {
                    call->setCallingConv(llvm::CallingConv::C);
                }
                return 0;
            }
        }
    }

    errors.panic_undeclared_func(name, argument_types);
    return 1;
}
int CallStatement::handle_var_args(Program& program, Function& func, AssemblyData& context, const std::string& va_arg_typename, uint64_t va_arg_count, uint64_t impl_arg_count,
                                  std::vector<llvm::Value*>& argument_values, llvm::Value* target, bool target_is_stdcall){
    AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );

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
    return 0;
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

MemberCallStatement::MemberCallStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00;
}
MemberCallStatement::MemberCallStatement(PlainExp* object, const std::string& name, const std::vector<PlainExp*>& args, ErrorHandler& errors){
    this->object = object;
    this->name = name;
    this->args = args;
    this->errors = errors;
    this->flags = 0x00;
}
MemberCallStatement::MemberCallStatement(const MemberCallStatement& other) : Statement(other) {
    this->object = other.object->clone();
    this->name = other.name;
    this->args.resize(other.args.size());
    this->flags = 0x00;

    for(size_t i = 0; i != args.size(); i++){
        args[i] = other.args[i]->clone();
    }
}
MemberCallStatement::~MemberCallStatement(){
    delete object;
}
int MemberCallStatement::assemble(Program& program, Function& func, AssemblyData& context){
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

    if(object_value == NULL) return 1;

    if(Program::is_pointer_typename(object_typename)){
        // The type is actually a pointer to a struct, so we'll dereference it automatically
        // ( Unlike the nightmare that is '->' in C++ )
        object_value = context.builder.CreateLoad(object_value);
        object_typename = object_typename.substr(1, object_typename.length()-1);
    }

    // Ensure the object is mutable
    if(!(object->flags & EXP_MUTABLE)){
        errors.panic("Can't call method of object because it is immutable");
        return 1;
    }

    // Get the llvm type for the object
    if(program.find_type(object_typename, context, &object_llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(object_typename) );
        return 1;
    }

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble_immutable(program, func, context, &expr_typename);
        if(expr_value == NULL) return 1;

        if(program.find_type(expr_typename, context, &expected_arg_type) != 0){
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

    std::string parent_struct_name = (func.parent_struct_offset != 0) ? program.structures[func.parent_struct_offset-1].name : "";

    // Ensure the function is public
    if(!(func_data.flags & EXTERN_PUBLIC) and parent_struct_name != object_typename){
        errors.panic("The method '" + object_typename + "." + name + "' is private");
        return 1;
    }

    std::string final_name = mangle(object_typename, name, func_data.arguments);

    llvm::Function* target = context.module->getFunction(final_name);
    if(target == NULL){
        errors.panic_undeclared_method(object_typename, name, argument_types);
        return 1;
    }

    argument_values.insert(argument_values.begin(), object_value);
    argument_types.insert(argument_types.begin(), "*" + object_typename);
    argument_llvm_types.insert(argument_llvm_types.begin(), object_llvm_type->getPointerTo());

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
        llvm::CallInst* call = context.builder.CreateCall(target, new_argument_values);
        call->setCallingConv(func_data.flags & EXTERN_STDCALL ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);

        // Free variable argument array
        call_values[0] = arguments_memory;
        context.builder.CreateCall(free_function, call_values);
    }
    else {
        for(size_t i = 1; i != argument_values.size(); i++){
            if(program.find_type(func_data.arguments[i-1], context, &expected_arg_type) != 0){
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

        llvm::CallInst* call = context.builder.CreateCall(target, argument_values);
        call->setCallingConv(func_data.flags & EXTERN_STDCALL ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);
    }

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

IfStatement::IfStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
IfStatement::IfStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
IfStatement::IfStatement(const IfStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

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
int IfStatement::assemble(Program& program, Function& func, AssemblyData& context){
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
    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
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

UnlessStatement::UnlessStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UnlessStatement::UnlessStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UnlessStatement::UnlessStatement(const UnlessStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

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
int UnlessStatement::assemble(Program& program, Function& func, AssemblyData& context){
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
    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
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

WhileStatement::WhileStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
WhileStatement::WhileStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
WhileStatement::WhileStatement(const WhileStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

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
int WhileStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* test_block = llvm::BasicBlock::Create(context.context, "test", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    llvm::BasicBlock* prev_break_point = context.break_point;
    llvm::BasicBlock* prev_continue_point = context.continue_point;
    context.break_point = false_block;
    context.continue_point = test_block;

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
    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(false_block);
    context.break_point = prev_break_point;
    context.continue_point = prev_continue_point;
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

UntilStatement::UntilStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UntilStatement::UntilStatement(PlainExp* condition, const StatementList& positive_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UntilStatement::UntilStatement(const UntilStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

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
int UntilStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* test_block = llvm::BasicBlock::Create(context.context, "test", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    llvm::BasicBlock* prev_break_point = context.break_point;
    llvm::BasicBlock* prev_continue_point = context.continue_point;
    context.break_point = true_block;
    context.continue_point = test_block;

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
    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(false_block);
    context.break_point = prev_break_point;
    context.continue_point = prev_continue_point;
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

IfElseStatement::IfElseStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
IfElseStatement::IfElseStatement(PlainExp* condition, const StatementList& positive_statements, const StatementList& negative_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->negative_statements = negative_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
IfElseStatement::IfElseStatement(const IfElseStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->negative_statements.resize(other.negative_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

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
int IfElseStatement::assemble(Program& program, Function& func, AssemblyData& context){
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
    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(continue_block);
    context.builder.SetInsertPoint(false_block);

    terminated = false;
    for(Statement* s : this->negative_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
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
        if(this->negative_statements[0]->flags & STMT_CONDITIONAL){
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

UnlessElseStatement::UnlessElseStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UnlessElseStatement::UnlessElseStatement(PlainExp* condition, const StatementList& positive_statements, const StatementList& negative_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->negative_statements = negative_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UnlessElseStatement::UnlessElseStatement(const UnlessElseStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->negative_statements.resize(other.negative_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

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
int UnlessElseStatement::assemble(Program& program, Function& func, AssemblyData& context){
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
    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(continue_block);
    context.builder.SetInsertPoint(false_block);

    terminated = false;
    for(Statement* s : this->negative_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
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
        if(this->negative_statements[0]->flags & STMT_CONDITIONAL){
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

IfWhileElseStatement::IfWhileElseStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
IfWhileElseStatement::IfWhileElseStatement(PlainExp* condition, const StatementList& positive_statements, const StatementList& negative_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->negative_statements = negative_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
IfWhileElseStatement::IfWhileElseStatement(const IfWhileElseStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->negative_statements.resize(other.negative_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }

    for(size_t i = 0; i != this->negative_statements.size(); i++){
        this->negative_statements[i] = other.negative_statements[i]->clone();
    }
}
IfWhileElseStatement::~IfWhileElseStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }

    for(Statement* statement : negative_statements){
        delete statement;
    }
}
int IfWhileElseStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );
    std::string expr_typename;
    llvm::Value* expr_value;
    bool terminated;

    llvm::BasicBlock* iftest_block = llvm::BasicBlock::Create(context.context, "iftest", llvm_function);
    llvm::BasicBlock* whiletest_block = llvm::BasicBlock::Create(context.context, "whiletest", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);
    llvm::BasicBlock* else_block = llvm::BasicBlock::Create(context.context, "else", llvm_function);

    llvm::BasicBlock* prev_break_point = context.break_point;
    llvm::BasicBlock* prev_continue_point = context.continue_point;
    context.break_point = false_block;
    context.continue_point = whiletest_block;

    // IF TEST BLOCK
    context.builder.CreateBr(iftest_block);
    context.builder.SetInsertPoint(iftest_block);

    expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expr_typename, &expr_value);
    if(expr_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expr_value, true_block, else_block);

    // WHILE TEST BLOCK
    context.builder.SetInsertPoint(whiletest_block);
    expr_value = this->condition->assemble_immutable(program, func, context, NULL);
    if(expr_value == NULL) return 1; // This should never occur
    context.builder.CreateCondBr(expr_value, true_block, false_block);

    // TRUE BLOCK
    context.builder.SetInsertPoint(true_block);
    terminated = false;

    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(whiletest_block);

    // ELSE BLOCK
    context.builder.SetInsertPoint(else_block);

    terminated = false;
    for(Statement* s : this->negative_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(false_block);

    // FALSE BLOCK
    context.builder.SetInsertPoint(false_block);
    context.break_point = prev_break_point;
    context.continue_point = prev_continue_point;
    return 0;
}
std::string IfWhileElseStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "if while " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "} else {\n";
    for(size_t s = 0; s != this->negative_statements.size(); s++){
        result += this->negative_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* IfWhileElseStatement::clone(){
    return new IfWhileElseStatement(*this);
}

UnlessUntilElseStatement::UnlessUntilElseStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UnlessUntilElseStatement::UnlessUntilElseStatement(PlainExp* condition, const StatementList& positive_statements, const StatementList& negative_statements, ErrorHandler& errors){
    this->condition = condition;
    this->positive_statements = positive_statements;
    this->negative_statements = negative_statements;
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;
}
UnlessUntilElseStatement::UnlessUntilElseStatement(const UnlessUntilElseStatement& other) : Statement(other) {
    this->condition = other.condition->clone();
    this->positive_statements.resize(other.positive_statements.size());
    this->negative_statements.resize(other.negative_statements.size());
    this->errors = errors;
    this->flags = STMT_CONDITIONAL;

    for(size_t i = 0; i != this->positive_statements.size(); i++){
        this->positive_statements[i] = other.positive_statements[i]->clone();
    }

    for(size_t i = 0; i != this->negative_statements.size(); i++){
        this->negative_statements[i] = other.negative_statements[i]->clone();
    }
}
UnlessUntilElseStatement::~UnlessUntilElseStatement(){
    delete condition;

    for(Statement* statement : positive_statements){
        delete statement;
    }

    for(Statement* statement : negative_statements){
        delete statement;
    }
}
int UnlessUntilElseStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );
    std::string expr_typename;
    llvm::Value* expr_value;
    bool terminated;

    llvm::BasicBlock* unlesstest_block = llvm::BasicBlock::Create(context.context, "unlesstest", llvm_function);
    llvm::BasicBlock* untiltest_block = llvm::BasicBlock::Create(context.context, "untiltest", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);
    llvm::BasicBlock* else_block = llvm::BasicBlock::Create(context.context, "else", llvm_function);

    llvm::BasicBlock* prev_break_point = context.break_point;
    llvm::BasicBlock* prev_continue_point = context.continue_point;
    context.break_point = true_block;
    context.continue_point = untiltest_block;

    // IF TEST BLOCK
    context.builder.CreateBr(unlesstest_block);
    context.builder.SetInsertPoint(unlesstest_block);

    expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, program, expr_typename, &expr_value);
    if(expr_typename != "bool"){
        errors.panic("Expression type for conditional must be 'bool' or another compatible primitive");
        return 1;
    }

    context.builder.CreateCondBr(expr_value, else_block, true_block);

    // UNTIL TEST BLOCK
    context.builder.SetInsertPoint(untiltest_block);
    expr_value = this->condition->assemble_immutable(program, func, context, NULL);
    if(expr_value == NULL) return 1; // This should never occur
    context.builder.CreateCondBr(expr_value, false_block, true_block);

    // TRUE BLOCK
    context.builder.SetInsertPoint(true_block);
    terminated = false;

    for(Statement* s : this->positive_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(untiltest_block);

    // ELSE BLOCK
    context.builder.SetInsertPoint(else_block);

    terminated = false;
    for(Statement* s : this->negative_statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(false_block);

    // FALSE BLOCK
    context.builder.SetInsertPoint(false_block);
    context.break_point = prev_break_point;
    context.continue_point = prev_continue_point;
    return 0;
}
std::string UnlessUntilElseStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "unless until " + this->condition->toString() + "{\n";
    for(size_t s = 0; s != this->positive_statements.size(); s++){
        result += this->positive_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "} else {\n";
    for(size_t s = 0; s != this->negative_statements.size(); s++){
        result += this->negative_statements[s]->toString(indent+1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* UnlessUntilElseStatement::clone(){
    return new UnlessUntilElseStatement(*this);
}

DeleteStatement::DeleteStatement(ErrorHandler& errors){
    this->value = NULL;
    this->dangerous = false;
    this->errors = errors;
    this->flags = 0x00;
}
DeleteStatement::DeleteStatement(PlainExp* value, ErrorHandler& errors){
    this->value = value;
    this->dangerous = false;
    this->errors = errors;
    this->flags = 0x00;
}
DeleteStatement::DeleteStatement(PlainExp* value, bool dangerous, ErrorHandler& errors){
    this->value = value;
    this->dangerous = dangerous;
    this->errors = errors;
    this->flags = 0x00;
}
DeleteStatement::DeleteStatement(const DeleteStatement& other) : Statement(other) {
    this->value = other.value->clone();
    this->dangerous = other.dangerous;
    this->errors = other.errors;
    this->flags = 0x00;
}
DeleteStatement::~DeleteStatement(){
    delete this->value;
}
int DeleteStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Type* llvm_type;
    std::string type_name;
    llvm::Value* pointer = value->assemble(program, func, context, &type_name);
    std::vector<llvm::Value*> free_args(1);

    // Declare the 'free' function if it isn't already declared
    llvm::Function* free_function = context.module->getFunction("free");
    if(!free_function){
        // Declare the malloc function if it doesn't already exist
        std::vector<llvm::Type*> args(1);
        args[0] = llvm::Type::getInt8PtrTy(context.context);

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm::Type::getInt8Ty(context.context), args, false);
        free_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "free", context.module.get());
    }

    program.resolve_if_alias(type_name);

    // If we're trying to delete an '[]' type, skip all of the normal steps and do this instead
    if(Program::is_array_typename(type_name)){
        // Resolve typename if it's an alias
        program.resolve_if_alias(type_name);

        if(program.find_type(type_name, context, &llvm_type) != 0){
            errors.panic(UNDECLARED_TYPE(type_name));
            return 1;
        }

        if(!(value->flags & EXP_MUTABLE)){
            errors.panic("Can't delete immutable array expression");
            return 1;
        }

        // Prepare GEP Indices
        std::vector<llvm::Value*> indices(2);
        llvm::Value* constant_zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
        indices[0] = constant_zero;
        indices[1] = constant_zero;

        free_args[0] = context.builder.CreateLoad(context.builder.CreateGEP(program.llvm_array_type, pointer, indices));
        context.builder.CreateCall(free_function, free_args);
        return 0;
    }

    if(value->flags & EXP_MUTABLE){
        pointer = context.builder.CreateLoad(pointer);
    }

    // Make sure expression specified is a pointer
    if(!Program::is_pointer_typename(type_name)){
        errors.panic(DELETE_REQUIRES_POINTER);
        return 1;
    }

    // Resolve typename if it's an alias
    program.resolve_if_alias(type_name);

    if(program.find_type(type_name, context, &llvm_type) != 0){
        errors.panic(UNDECLARED_TYPE(type_name));
        return 1;
    }

    if(type_name == "ptr" and !dangerous){
        errors.warn("Deleting data pointed to by a 'ptr' won't correctly delete types that require special deletion\n    To suppress this warning, put the 'dangerous' keyword immediately after the 'delete' keyword");
    }

    free_args[0] = context.builder.CreateBitCast(pointer, llvm::Type::getInt8PtrTy(context.context));
    context.builder.CreateCall(free_function, free_args);
    return 0;
}
std::string DeleteStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "delete " + value->toString();
    return result;
}
Statement* DeleteStatement::clone(){
    return new DeleteStatement(*this);
}

SwitchStatement::Case::Case(){}
SwitchStatement::Case::Case(PlainExp* value, const StatementList& statements){
    this->value = value;
    this->statements = statements;
}

SwitchStatement::SwitchStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00; // Not really a conditional (or at least we don't treat is as one)
}
SwitchStatement::SwitchStatement(PlainExp* condition, const std::vector<SwitchStatement::Case>& cases, const StatementList& default_statements, ErrorHandler& errors){
    this->condition = condition;
    this->cases = cases;
    this->default_statements = default_statements;
    this->errors = errors;
    this->flags = 0x00; // Not really a conditional (or at least we don't treat is as one)
}
SwitchStatement::SwitchStatement(const SwitchStatement& other) : Statement(other) {
    SwitchStatement::Case* this_case;
    SwitchStatement::Case* other_case;

    this->condition = other.condition->clone();
    this->errors = errors;
    this->cases.resize(other.cases.size());
    this->default_statements.resize(other.default_statements.size());
    this->flags = 0x00; // Not really a conditional (or at least we don't treat is as one)

    for(size_t i = 0; i != other.cases.size(); i++){
        this_case = &this->cases[i];
        other_case = &other.cases[i];

        this_case->value = other_case->value->clone();
        this_case->statements.resize(other_case->statements.size());

        for(size_t j = 0; j != other_case->statements.size(); j++){
            this_case->statements[j] = other_case->statements[j]->clone();
        }
    }

    for(size_t i = 0; i != other.default_statements.size(); i++){
        this->default_statements[i] = other.default_statements[i]->clone();
    }
}
SwitchStatement::~SwitchStatement(){
    delete condition;

    for(const SwitchStatement::Case& switch_case : this->cases){
        delete switch_case.value;

        for(Statement* statement : switch_case.statements){
            delete statement;
        }
    }

    for(Statement* statement : default_statements){
        delete statement;
    }
}
int SwitchStatement::assemble(Program& program, Function& func, AssemblyData& context){
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    std::string value_typename;
    std::string condition_typename;
    llvm::Value* switch_value = condition->assemble_immutable(program, func, context, &condition_typename);
    llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context.context, "switchcont", llvm_function);
    llvm::BasicBlock* default_block; // Points to default block (continue_block if no default case was specified)
    llvm::BasicBlock* current_block = context.builder.GetInsertBlock();
    llvm::Value* case_condition;
    llvm::BasicBlock* case_block;
    std::vector<llvm::APInt> values;

    program.resolve_if_alias(condition_typename);

    if(switch_value == NULL) return 1;
    if(!Program::is_integer_typename(condition_typename)){
        bool is_an_enum = false;
        for(Enum& inum : program.enums){
            if(inum.name == condition_typename){
                is_an_enum = true; break;
            }
        }

        if(!is_an_enum){
            errors.panic("Switch condition value must be an integer");
            return 1;
        }
    }

    if(this->default_statements.size() != 0){
        bool terminated = false;
        default_block = llvm::BasicBlock::Create(context.context, "switchdefault", llvm_function);
        context.builder.SetInsertPoint(default_block);

        for(Statement* s : this->default_statements){
            if(s->flags & STMT_TERMINATOR) terminated = true;
            if(s->assemble(program, func, context) != 0){
                return 1;
            }
        }

        if(!terminated){
            context.builder.CreateBr(continue_block);
        }
    }
    else {
        default_block = continue_block;
    }

    context.builder.SetInsertPoint(current_block);
    llvm::SwitchInst* switch_statement = context.builder.CreateSwitch(switch_value, default_block);

    llvm::BasicBlock* prev_break_point = context.break_point;
    context.break_point = continue_block;

    for(const SwitchStatement::Case& switch_case : this->cases){
        case_condition = switch_case.value->assemble_immutable(program, func, context, &value_typename);
        if(case_condition == NULL) return 1;

        if(!Program::is_integer_typename(value_typename) and value_typename != condition_typename){
            errors.panic("Case condition value must be an integer");
            return 1;
        }

        if(!(switch_case.value->flags & EXP_CONSTANT)){
            errors.panic("Case condition value must be constant");
            return 1;
        }

        bool terminated = false;
        case_block = llvm::BasicBlock::Create(context.context, "case", llvm_function);
        context.builder.SetInsertPoint(case_block);

        for(Statement* s : switch_case.statements){
            if(s->flags & STMT_TERMINATOR) terminated = true;
            if(s->assemble(program, func, context) != 0){
                return 1;
            }
        }

        if(!terminated){
            context.builder.CreateBr(continue_block);
        }

        // WARNING: case_condition should be a llvm::ConstantInt* (this should've been verified earlier) but if it isn't, bad things will happen
        llvm::ConstantInt* constant_val = static_cast<llvm::ConstantInt*>(case_condition);
        llvm::APInt constant_int = constant_val->getValue();

        for(const llvm::APInt& i : values){
            if(constant_int == i){
                errors.panic("Attempting to define multiple cases with the same value");
                return 1;
            }
        }

        switch_statement->addCase(constant_val, case_block);
        values.push_back(constant_int);
    }

    context.builder.SetInsertPoint(continue_block);
    context.break_point = prev_break_point;
    return 0;
}
std::string SwitchStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "switch " + condition->toString() + " {\n";

    for(SwitchStatement::Case& switch_case : cases){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
        result += "case " + switch_case.value->toString() + "\n";

        for(Statement* statement : switch_case.statements){
            result += statement->toString(indent + 1, false) + "\n";
        }
    }

    if(default_statements.size() != 0){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
        result += "default\n";

        for(Statement* statement : default_statements){
            result += statement->toString(indent + 1, false) + "\n";
        }
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* SwitchStatement::clone(){
    return new SwitchStatement(*this);
}

ForStatement::ForStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00; // Not really a conditional (or at least we don't treat is as one)
}
ForStatement::ForStatement(Statement* initialization_statement, PlainExp* condition, Statement* increament_statement, const StatementList& statements, ErrorHandler& errors){
    this->initialization_statement = initialization_statement;
    this->condition = condition;
    this->increament_statement = increament_statement;
    this->statements = statements;
    this->errors = errors;
    this->flags = 0x00; // Not really a conditional (or at least we don't treat is as one)
}
ForStatement::ForStatement(const ForStatement& other) : Statement(other) {
    this->initialization_statement = other.initialization_statement == NULL ? NULL : other.initialization_statement->clone();
    this->condition = other.condition == NULL ? NULL : other.condition->clone();
    this->increament_statement = other.increament_statement == NULL ? NULL : other.increament_statement->clone();
    this->statements.resize(other.statements.size());
    this->flags = 0x00; // Not really a conditional (or at least we don't treat is as one)

    for(size_t i = 0; i != other.statements.size(); i++){
        this->statements[i] = other.statements[i]->clone();
    }
}
ForStatement::~ForStatement(){
    delete initialization_statement;
    delete condition;
    delete increament_statement;

    for(Statement* statement : statements){
        delete statement;
    }
}
int ForStatement::assemble(Program& program, Function& func, AssemblyData& context){
    if(initialization_statement != NULL) if(initialization_statement->assemble(program, func, context) != 0) return 1;

    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    llvm::BasicBlock* incr_block;
    llvm::BasicBlock* test_block = llvm::BasicBlock::Create(context.context, "test", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    if(increament_statement != NULL){
        incr_block = llvm::BasicBlock::Create(context.context, "incr", llvm_function);
    }

    llvm::BasicBlock* prev_break_point = context.break_point;
    llvm::BasicBlock* prev_continue_point = context.continue_point;
    context.break_point = false_block;

    if(increament_statement != NULL){
        context.continue_point = incr_block;
    } else {
        context.continue_point = test_block;
    }

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
    for(Statement* s : this->statements){
        if(s->flags & STMT_TERMINATOR) terminated = true;

        if(s->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated){
        if(increament_statement != NULL){
            context.builder.CreateBr(incr_block);
        } else {
            context.builder.CreateBr(test_block);
        }
    }

    if(increament_statement != NULL){
        context.builder.SetInsertPoint(incr_block);
        if(increament_statement->assemble(program, func, context) != 0) return 1;
        context.builder.CreateBr(test_block);
    }

    context.builder.SetInsertPoint(false_block);

    context.break_point = prev_break_point;
    context.continue_point = prev_continue_point;
    return 0;
}
std::string ForStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    result += "for ";
    if(initialization_statement != NULL) result += initialization_statement->toString(0, true);
    result += ", ";
    if(condition != NULL) result += condition->toString();
    if(increament_statement != NULL) result += ", " + increament_statement->toString(0, true);
    result += " {\n";

    for(Statement* s : statements){
        result += s->toString(indent + 1, false) + "\n";
    }

    for(unsigned int i = 0; i != indent; i++) result += "    ";
    result += "}";
    return result;
}
Statement* ForStatement::clone(){
    return new ForStatement(*this);
}

BreakStatement::BreakStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_TERMINATOR;
}
BreakStatement::BreakStatement(const BreakStatement& other) : Statement(other) {
    this->flags = STMT_TERMINATOR;
}
BreakStatement::~BreakStatement(){}
int BreakStatement::assemble(Program& program, Function& func, AssemblyData& context){
    if(context.break_point == NULL){
        errors.panic("No place to break to");
        return 1;
    }

    context.builder.CreateBr(context.break_point);
    return 0;
}
std::string BreakStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    return result + "break";
}
BreakStatement* BreakStatement::clone(){
    return new BreakStatement(this->errors);
}

ContinueStatement::ContinueStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_TERMINATOR;
}
ContinueStatement::ContinueStatement(const ContinueStatement& other) : Statement(other) {
    this->flags = STMT_TERMINATOR;
}
ContinueStatement::~ContinueStatement(){}
int ContinueStatement::assemble(Program& program, Function& func, AssemblyData& context){
    if(context.continue_point == NULL){
        errors.panic("No place to continue to");
        return 1;
    }

    context.builder.CreateBr(context.continue_point);
    return 0;
}
std::string ContinueStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result;

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    return result + "continue";
}
ContinueStatement* ContinueStatement::clone(){
    return new ContinueStatement(this->errors);
}

MultiResultCallStatement::MultiResultCallStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = 0x00;
}
MultiResultCallStatement::MultiResultCallStatement(const std::string& name, const std::vector<PlainExp*>& args, const std::vector<std::string>& result_variables, ErrorHandler& errors){
    this->name = name;
    this->args = args;
    this->result_variables = result_variables;
    this->errors = errors;
    this->flags = 0x00;
}
MultiResultCallStatement::MultiResultCallStatement(const MultiResultCallStatement& other) : Statement(other) {
    this->name = other.name;
    this->args.resize(other.args.size());
    this->result_variables = other.result_variables;
    this->flags = 0x00;

    for(size_t i = 0; i != args.size(); i++){
        args[i] = other.args[i]->clone();
    }
}
MultiResultCallStatement::~MultiResultCallStatement(){
    for(PlainExp* arg : args) delete arg;
}
int MultiResultCallStatement::assemble(Program& program, Function& func, AssemblyData& context){
    External func_data;
    AssembleVariable* func_variable;
    Global global;
    AssembleGlobal* func_global;

    llvm::Value* expr_value;
    std::string expr_typename;
    llvm::Type* expected_arg_type;
    std::vector<llvm::Value*> argument_values;
    std::vector<std::string> argument_types;
    std::vector<llvm::Type*> argument_llvm_types;

    for(size_t i = 0, e = args.size(); i != e; i++) {
        expr_value = args[i]->assemble_immutable(program, func, context, &expr_typename);
        if(expr_value == NULL) return 1;

        if(program.find_type(expr_typename, context, &expected_arg_type) != 0){
            errors.panic( UNDECLARED_TYPE(expr_typename) );
            return 1;
        }

        argument_values.push_back(expr_value);
        argument_types.push_back(expr_typename);
        argument_llvm_types.push_back(expected_arg_type);
    }

    if(program.find_func(name, argument_types, &func_data) == 0){
        // Standard function exists

        std::string final_name = (func_data.flags & EXTERN_MANGLED) ? mangle(name, func_data.arguments) : name;
        llvm::Function* target = context.module->getFunction(final_name);
        if (!target){
            errors.panic_undeclared_func(name, argument_types);
            return 1;
        }
        assert(func_data.arguments.size() == target->arg_size());

        if(!(func_data.flags & EXTERN_MULRET)){
            errors.panic("The function '" + name + "' doesn't return multiple values");
            return 1;
        }

        if(func_data.flags & EXTERN_VARARGS){
            std::string va_arg_typename = func_data.arguments[func_data.arguments.size()-1];
            va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);
            uint64_t va_arg_count = args.size() - func_data.arguments.size() + 1;

            return this->handle_var_args(program, func, context, va_arg_typename, va_arg_count, func_data.arguments.size(),
                                        argument_values, func_data.extra_return_types, target, func_data.flags & EXTERN_STDCALL);
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

            if(this->append_return_addresses(program, context, func_data.extra_return_types, argument_values) != 0) return 1;

            llvm::CallInst* call = context.builder.CreateCall(target, argument_values);
            call->setCallingConv(func_data.flags & EXTERN_STDCALL ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);
        }

        return 0;
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
            std::vector<std::string> varfunc_extra_return_types;
            char flags = 0x00;

            if(program.extract_function_pointer_info(func_variable->type, varfunc_llvm_args, context, &varfunc_return_llvm_type, varfunc_args,
                varfunc_return_typename, &varfunc_extra_return_types, flags) != 0) return 1;

            if(varfunc_return_llvm_type != NULL){
                errors.panic("The function pointed to by the function pointer '" + name + "' doesn't return multiple values");
                return 1;
            }

            if(flags & FUNC_VARARGS){
                AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );
                std::string va_arg_typename = varfunc_args[varfunc_args.size()-1];
                va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);
                uint64_t va_arg_count = args.size() - varfunc_args.size() + 1;

                return this->handle_var_args(program, func, context, va_arg_typename, va_arg_count, varfunc_args.size(),
                                        argument_values, varfunc_extra_return_types, context.builder.CreateLoad(func_variable->variable),
                                        Program::function_typename_is_stdcall(func_variable->type));
            }
            else {
                if (varfunc_args.size() != args.size()){
                    errors.panic("Incorrect function argument count when calling '" + name + "' (" + to_str(args.size()) + " received, " + to_str(varfunc_args.size()) + " required)");
                    return 1;
                }

                for(size_t i = 0; i != argument_values.size(); i++){
                    if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                        errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + func_variable->type +
                             "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                        return 1;
                    }
                }

                if(this->append_return_addresses(program, context, varfunc_extra_return_types, argument_values) != 0) return 1;

                llvm::Value* function_address = context.builder.CreateLoad(func_variable->variable);
                llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values);

                if(Program::function_typename_is_stdcall(func_variable->type)){
                    call->setCallingConv(llvm::CallingConv::X86_StdCall);
                } else {
                    call->setCallingConv(llvm::CallingConv::C);
                }
                return 0;
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
            std::vector<std::string> varfunc_extra_return_types;
            char flags = 0x00;

            if(program.extract_function_pointer_info(global.type, varfunc_llvm_args, context, &varfunc_return_llvm_type, varfunc_args,
                varfunc_return_typename, &varfunc_extra_return_types, flags) != 0) return 1;

            if(varfunc_return_llvm_type != NULL){
                errors.panic("The function pointed to by the function pointer '" + name + "' doesn't return multiple values");
                return 1;
            }

            if(flags & FUNC_VARARGS){
                AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );
                std::string va_arg_typename = varfunc_args[varfunc_args.size()-1];
                va_arg_typename = va_arg_typename.substr(2, va_arg_typename.length() - 2);
                uint64_t va_arg_count = args.size() - varfunc_args.size() + 1;
                AssembleGlobal* global_asm_data = context.findGlobal(global.name);

                if(global_asm_data == NULL){
                    errors.panic("Failed to find assembly data for global");
                    return 1;
                }

                return this->handle_var_args(program, func, context, va_arg_typename, va_arg_count, varfunc_args.size(),
                                        argument_values, varfunc_extra_return_types, context.builder.CreateLoad(global_asm_data->variable),
                                        Program::function_typename_is_stdcall(global.type));
            }
            else {
                if (varfunc_args.size() != args.size()){
                    errors.panic("Incorrect function argument count when calling '" + name + "' (" + to_str(args.size()) + " received, " + to_str(varfunc_args.size()) + " required)");
                    return 1;
                }

                for(size_t i = 0; i != argument_values.size(); i++){
                    if(assemble_merge_types_oneway(context, program, argument_types[i], varfunc_args[i], &argument_values[i], varfunc_llvm_args[i], NULL) != 0){
                        errors.panic("Incorrect type for argument " + to_str(i+1) + " of function '" + name + "'\n    Definition: " + global.type +
                             "\n    Expected type '" + varfunc_args[i] + "' but received type '" + argument_types[i] + "'");
                        return 1;
                    }
                }

                if(this->append_return_addresses(program, context, varfunc_extra_return_types, argument_values) != 0) return 1;

                llvm::Value* function_address = context.builder.CreateLoad(context.findGlobal(name)->variable);
                llvm::CallInst* call = context.builder.CreateCall(function_address, argument_values);

                if(Program::function_typename_is_stdcall(global.type)){
                    call->setCallingConv(llvm::CallingConv::X86_StdCall);
                } else {
                    call->setCallingConv(llvm::CallingConv::C);
                }
                return 0;
            }
        }
    }

    errors.panic_undeclared_func(name, argument_types);
    return 1;
}
int MultiResultCallStatement::append_return_addresses(Program& program, AssemblyData& context, const std::vector<std::string>& extra_return_types, std::vector<llvm::Value*>& argument_values){
    std::vector<std::string> result_types;
    std::vector<llvm::Value*> addresses_of;

    size_t ret_index = 0;
    for(const std::string& result_variable : result_variables){
        if(result_variable == ""){ // Used when 'null' is given instead of an identifier
            llvm::Type* result_null_type;
            std::string required_type = extra_return_types[ret_index];

            if(program.find_type(required_type, context, &result_null_type) != 0){
                errors.panic( UNDECLARED_TYPE(required_type) );
                return 1;
            }

            result_types.push_back(required_type);
            addresses_of.push_back(llvm::ConstantPointerNull::get(result_null_type->getPointerTo()));
            ret_index++; continue;
        }

        AssembleVariable* var = context.current_function->findVariable(result_variable);
        Global global;

        if(var != NULL){
            result_types.push_back(var->type);
            addresses_of.push_back(var->variable);
            ret_index++; continue;
        }
        else if(program.find_global(result_variable, &global) == 0){
            result_types.push_back(global.type);
            addresses_of.push_back(context.findGlobal(result_variable)->variable);
            ret_index++; continue;
        }
        else {
            errors.panic( UNDECLARED_VARIABLE(result_variable) );
            return 1;
        }
    }

    if(result_types.size() != extra_return_types.size()){
        if(result_types.size() > extra_return_types.size()){
            errors.panic("Too many result variables specified (" + to_str(extra_return_types.size()) + " required)");
        } else {
            errors.panic("Not enough result variables specified (" + to_str(extra_return_types.size()) + " required)");
        }
        return 1;
    }

    for(size_t i = 0; i != result_types.size(); i++){
        if(result_types[i] != extra_return_types[i]){
            errors.panic("Variable '" + result_variables[i] + "' is the type '" + result_types[i] + "' and doesn't match return type #" + to_str(i+1) + " '" + extra_return_types[i] + "' of the function '" + name + "'");
            return 1;
        }
    }

    argument_values.insert(argument_values.end(), addresses_of.begin(), addresses_of.end());
    return 0;
}
int MultiResultCallStatement::handle_var_args(Program& program, Function& func, AssemblyData& context, const std::string& va_arg_typename, uint64_t va_arg_count, uint64_t impl_arg_count,
                                  std::vector<llvm::Value*>& argument_values, const std::vector<std::string>& extra_return_types, llvm::Value* target, bool target_is_stdcall){
    AssembleFunction* asm_func = context.getFunction( !(func.flags & FUNC_EXTERNAL) ? mangle(program, func) : func.name );

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

    if(this->append_return_addresses(program, context, extra_return_types, new_argument_values) != 0) return 1;

    // Pass arguments
    llvm::CallInst* call = context.builder.CreateCall(target, new_argument_values);
    call->setCallingConv(target_is_stdcall ? llvm::CallingConv::X86_StdCall : llvm::CallingConv::C);

    // Free variable argument array
    call_values[0] = arguments_memory;
    context.builder.CreateCall(free_function, call_values);
    return 0;
}
std::string MultiResultCallStatement::toString(unsigned int indent, bool skip_initial_indent){
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
Statement* MultiResultCallStatement::clone(){
    return new MultiResultCallStatement(*this);
}

MultiReturnStatement::MultiReturnStatement(ErrorHandler& errors){
    this->errors = errors;
    this->flags = STMT_TERMINATOR;
}
MultiReturnStatement::MultiReturnStatement(const std::vector<PlainExp*>& return_values, ErrorHandler& errors) {
    this->return_values = return_values;
    this->errors = errors;
    this->flags = STMT_TERMINATOR;
}
MultiReturnStatement::MultiReturnStatement(const MultiReturnStatement& other) : Statement(other) {
    this->return_values.resize(other.return_values.size());

    for(size_t i = 0; i != other.return_values.size(); i++){
        this->return_values[i] = other.return_values[i]->clone();
    }

    this->flags = STMT_TERMINATOR;
}
MultiReturnStatement::~MultiReturnStatement(){
    for(PlainExp* value : this->return_values) delete value;
}
int MultiReturnStatement::assemble(Program& program, Function& func, AssemblyData& context){
    if((!func.flags & FUNC_MULRET)){
        errors.panic("Can't multiple values in a function that returns a single value");
        return 1;
    }

    std::vector<llvm::Value*> ret_llvm_values;
    std::vector<std::string> ret_types;
    std::vector<llvm::Type*> ret_llvm_types;
    llvm::Function* llvm_function = context.module->getFunction( mangle(program, func) );

    for(PlainExp* exp : this->return_values){
        llvm::Type* llvm_type;
        std::string expression_type;
        llvm::Value* llvm_value = exp->assemble_immutable(program, func, context, &expression_type);
        if(llvm_value == NULL) return 1;

        if(program.find_type(expression_type, context, &llvm_type) != 0){
            errors.panic( UNDECLARED_TYPE(expression_type) );
            return 1;
        }

        ret_llvm_values.push_back(llvm_value);
        ret_types.push_back(expression_type);
        ret_llvm_types.push_back(llvm_type);
    }

    if(func.extra_return_types.size() != ret_llvm_values.size()){
        errors.panic("Incorrect amount of return values (" + to_str(func.extra_return_types.size()) + " required)");
        return 1;
    }

    for(size_t i = 0; i != ret_llvm_values.size(); i++){
        if(assemble_merge_types_oneway(context, program, ret_types[i], func.extra_return_types[i], &(ret_llvm_values[i]), ret_llvm_types[i], NULL) != 0){
            errors.panic("The type of return expression #" + to_str(i+1) + " '" + ret_types[i] + "' doesn't match functions return type #" + to_str(i+1) + " '" + func.extra_return_types[i] + "'");
            return 1;
        }

        llvm::BasicBlock* not_null_block = llvm::BasicBlock::Create(context.context, "notnull", llvm_function);
        llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context.context, "cont", llvm_function);

        llvm::Value* store_location = context.current_function->multi_return_pointers[i];
        llvm::Value* null_value = llvm::ConstantPointerNull::get(store_location->getType());
        llvm::Value* pointer_isnt_null = context.builder.CreateICmpNE(store_location, null_value);
        context.builder.CreateCondBr(pointer_isnt_null, not_null_block, continue_block);
        context.builder.SetInsertPoint(not_null_block);
        context.builder.CreateStore(ret_llvm_values[i], store_location);
        context.builder.CreateBr(continue_block);
        context.builder.SetInsertPoint(continue_block);
    }

    context.builder.CreateBr(context.current_function->quit);
    return 0;
}
std::string MultiReturnStatement::toString(unsigned int indent, bool skip_initial_indent){
    std::string result = "return ";

    if(!skip_initial_indent){
        for(unsigned int i = 0; i != indent; i++) result += "    ";
    }

    for(size_t i = 0; i != this->return_values.size(); i++){
        result += this->return_values[i]->toString();
        if(i + 1 != this->return_values.size()) result += ", ";
    }

    return result;
}
Statement* MultiReturnStatement::clone(){
    return new MultiReturnStatement(*this);
}

