
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

    if(program.find_type(this->variable_type, &llvm_type) != 0){
        errors.panic( UNDECLARED_TYPE(this->variable_type) );
        return 1;
    }

    llvm::Value* llvm_value = variable_value->assemble_immutable(program, func, context, &expression_type);
    if(llvm_value == NULL) return 1;

    if(assemble_merge_types_oneway(context, expression_type, this->variable_type, &llvm_value, llvm_type, NULL) != 0){
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
    this->return_value = other.return_value->clone();
}
ReturnStatement::~ReturnStatement(){
    delete this->return_value;
}
int ReturnStatement::assemble(Program& program, Function& func, AssembleContext& context){
    if(this->return_value != NULL){
        std::string expression_type;
        llvm::Value* llvm_value = this->return_value->assemble_immutable(program, func, context, &expression_type);
        if(llvm_value == NULL) return 1;

        if(assemble_merge_types_oneway(context, expression_type, func.return_type, &llvm_value, func.asm_func.return_type, NULL) != 0){
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
    if(assemble_merge_types_oneway(context, expression_typename, location_typename, &llvm_value, location_llvm_type, NULL) != 0){
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
    llvm::Value* expression_value;
    std::string expression_typename;
    llvm::Type* expression_llvm_type;
    External function_data;

    std::vector<llvm::Value*> argument_values;
    std::vector<std::string> argument_types;
    std::vector<llvm::Type*> argument_llvm_types;

    for(size_t i = 0; i != this->args.size(); i++) {
        expression_value = this->args[i]->assemble_immutable(program, func, context, &expression_typename);
        if(expression_value == NULL) return 1;

        if(program.find_type(expression_typename, &expression_llvm_type) != 0){
            errors.panic( UNDECLARED_TYPE(expression_typename) );
            return 1;
        }

        argument_values.push_back(expression_value);
        argument_types.push_back(expression_typename);
        argument_llvm_types.push_back(expression_llvm_type);
    }

    if(program.find_func(this->name, argument_types, &function_data) != 0){
        errors.panic( UNDECLARED_FUNC(this->name) );
        return 1;
    }

    std::string final_name = (function_data.is_mangled) ? mangle(this->name, function_data.arguments) : this->name;
    llvm::Function* target = context.module->getFunction(final_name);

    if (!target){
        errors.panic( UNDECLARED_FUNC(this->name) );
        return 1;
    }
    assert(function_data.arguments.size() == target->arg_size());

    if (target->arg_size() != this->args.size()){
        // NOTE: This error should never appear
        errors.panic("Incorrect function argument count for function '" + this->name + "'");
        errors.panic(SUICIDE);
        return 1;
    }

    for(size_t z = 0; z != argument_values.size(); z++){
        if(assemble_merge_types_oneway(context, argument_types[z], function_data.arguments[z], &argument_values[z], argument_llvm_types[z], NULL) != 0){
            // NOTE: This error should never occur
            errors.panic("Incorrect type for argument " + to_str(z+1) + " of function '" + this->name + "'\n    Definition: " + function_data.toString() +
                 "\n    Expected type '" + function_data.arguments[z] + "' but received type '" + argument_types[z] + "'");
            errors.panic(SUICIDE);
            return 1;
        }
    }

    context.builder.CreateCall(target, argument_values, "calltmp");
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
bool CallStatement::isTerminator(){
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
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, expr_typename, &expr_value);

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
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, expr_typename, &expr_value);

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
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );

    llvm::BasicBlock* test_block = llvm::BasicBlock::Create(context.context, "test", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(test_block);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, expr_typename, &expr_value);

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
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );

    llvm::BasicBlock* test_block = llvm::BasicBlock::Create(context.context, "test", llvm_function);
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);

    context.builder.CreateBr(test_block);
    context.builder.SetInsertPoint(test_block);

    std::string expr_typename;
    llvm::Value* expr_value = this->condition->assemble_immutable(program, func, context, &expr_typename);
    if(expr_value == NULL) return 1;

    assemble_merge_conditional_types(context, expr_typename, &expr_value);

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
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);
    llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context.context, "continue", llvm_function);

    std::string expression_typename;
    llvm::Value* expression_value = this->condition->assemble_immutable(program, func, context, &expression_typename);
    if(expression_value == NULL) return 1;

    assemble_merge_conditional_types(context, expression_typename, &expression_value);

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
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );

    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context, "true", llvm_function);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context, "false", llvm_function);
    llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(context.context, "continue", llvm_function);

    std::string expression_typename;
    llvm::Value* expression_value = this->condition->assemble_immutable(program, func, context, &expression_typename);
    if(expression_value == NULL) return 1;

    assemble_merge_conditional_types(context, expression_typename, &expression_value);

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
