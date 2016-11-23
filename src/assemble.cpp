
#include <string>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
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

#include "../include/die.h"
#include "../include/assemble.h"
#include "../include/validate.h"

AssembleContext::AssembleContext() : builder(context) {
    module = llvm::make_unique<llvm::Module>("Adept", context);
}

int assemble(AssembleContext& assemble, Configuration& config, Program& program){
    if(program.generate_types(assemble) != 0) return 1;

    for(size_t i = 0; i != program.externs.size(); i++){
        if(assemble_external(assemble, config, program, program.externs[i]) != 0) return 1;
    }

    for(size_t i = 0; i != program.functions.size(); i++){
        if(assemble_function(assemble, config, program, program.functions[i]) != 0) return 1;
    }

    // Print Assembler Time
    config.clock.print_since("Assembler Finished");
    config.clock.remember();

    if(!config.jit){
        std::error_code error_str;
        llvm::raw_ostream* out = new llvm::raw_fd_ostream("C:/Users/isaac/Desktop/adeptout.bc", error_str, llvm::sys::fs::F_None);
        llvm::WriteBitcodeToFile(assemble.module.get(), *out);
        out->flush();
        delete out; // LLVM ERROR: IO failure on output stream.

        system("llc C:/Users/isaac/Desktop/adeptout.bc -filetype=obj");
        system("C:/MinGW64/bin/gcc C:/Users/isaac/Desktop/adeptout.obj C:/MinGW64/lib/gcc/x86_64-w64-mingw32/5.3.0/libstdc++.a -o C:/Users/isaac/Desktop/adeptout.exe");

        if(access("C:/Users/isaac/Desktop/adeptout.exe", F_OK ) == -1){
            std::cout << "Failed to create 'adeptout.exe'" << std::endl;
        }

        // Print Build Time
        std::cout << "Build Finished     (" << config.clock.since() << "s)" << std::endl;
        config.clock.remember();
    }

    return 0;
}
int assemble_structure(AssembleContext& context, Configuration& config, Program& program, Structure& structure){
    return 0;
}
int assemble_function(AssembleContext& context, Configuration& config, Program& program, Function& func){
    llvm::Function* llvm_function = context.module->getFunction(func.name);

    if(!llvm_function){
        llvm::Type* llvm_type;
        std::vector<llvm::Type*> args(func.arguments.size());

        for(size_t i = 0; i != func.arguments.size(); i++){
            if(program.find_type(func.arguments[i].type, &llvm_type) != 0){
                std::cerr << "The type '" << func.arguments[i].type << "' does not exist" << std::endl;
                return 1;
            }
            args[i] = llvm_type;
        }

        if(program.find_type(func.return_type, &llvm_type) != 0){
            std::cerr << "The type '" << func.return_type << "' does not exist" << std::endl;
            return 1;
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);
        llvm_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, func.name, context.module.get());

        // Create a new basic block to start insertion into.
        llvm::BasicBlock *basic_block = llvm::BasicBlock::Create(context.context, "entry", llvm_function);
        context.builder.SetInsertPoint(basic_block);

        size_t i = 0;
        for(auto& arg : llvm_function->args()){
            llvm::AllocaInst* alloca = context.builder.CreateAlloca(args[i], 0, func.arguments[i].name);
            context.builder.CreateStore(&arg, alloca);

            func.variables.push_back( Variable{func.arguments[i].name, func.arguments[i].type, alloca} );
            arg.setName(func.arguments[i++].name);
        }
    }
    else {
        std::cerr << "Duplicate function definition" << std::endl;
        return 1;

        // If we wanting to insert into the existing function:
        /*
        if(!llvm_function) return 1;
        // Create a new basic block to start insertion into.
        llvm::BasicBlock *basic_block = llvm::BasicBlock::Create(context.context, "entry", llvm_function);
        context.builder.SetInsertPoint(basic_block);
        */
    }

    for(size_t i = 0; i != func.statements.size(); i++){
        if(assemble_statement(context, config, program, func, func.statements[i]) != 0){
            llvm_function->eraseFromParent();
            return 1;
        }
    }

    llvm::verifyFunction(*llvm_function);

    // USE IF ERROR
    // Error reading body, remove function.
    // llvm_function->eraseFromParent();

    return 0;
}
int assemble_external(AssembleContext& context, Configuration& config, Program& program, External& external){
    llvm::Function* llvm_function = context.module->getFunction(external.name);

    if(!llvm_function){
        llvm::Type* llvm_type;
        std::vector<llvm::Type*> args(external.arguments.size());

        for(size_t i = 0; i != external.arguments.size(); i++){
            if(program.find_type(external.arguments[i], &llvm_type) != 0){
                std::cerr << "The type '" << external.arguments[i] << "' does not exist" << std::endl;
                return 1;
            }
            args[i] = llvm_type;
        }

        if(program.find_type(external.return_type, &llvm_type) != 0){
            std::cerr << "The type '" << external.return_type << "' does not exist" << std::endl;
            return 1;
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);
        llvm_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, external.name, context.module.get());
    }
    else {
        std::cerr << "Duplicate function definition" << std::endl;
        return 1;
    }

    // USE IF ERROR
    // Error reading body, remove function.
    // llvm_function->eraseFromParent();

    return 0;
}
int assemble_statement(AssembleContext& context, Configuration& config, Program& program, Function& func, Statement& statement){

    switch(statement.id){
    case STATEMENTID_RETURN:
        {
            ReturnStatement data = *( static_cast<ReturnStatement*>(statement.data) );
            if(!validate_return_statement(program, data, func)) return 1;

            llvm::Value *return_value = data.value->assemble(program, func, context);
            context.builder.CreateRet(return_value);
            break;
        }
    case STATEMENTID_DECLARE:
        {
            DeclareStatement data = *( static_cast<DeclareStatement*>(statement.data) );
            llvm::Type* var_type;

            if(program.find_type(data.type, &var_type) != 0){
                std::cerr << "The type '" << data.type << " does not exist" << std::endl;
                return 1;
            }

            llvm::AllocaInst* alloc = context.builder.CreateAlloca(var_type, 0, data.name.c_str());
            func.variables.push_back( Variable{data.name, data.type, alloc} );
            break;
        }
    case STATEMENTID_DECLAREAS:
        {
            DeclareAsStatement data = *( static_cast<DeclareAsStatement*>(statement.data) );
            llvm::Type* var_type;

            if(program.find_type(data.type, &var_type) != 0){
                std::cerr << "The type '" << data.type << " does not exist" << std::endl;
                return 1;
            }

            llvm::Value* value = data.value->assemble(program, func, context);
            if(value == NULL) return 1;

            llvm::AllocaInst* alloc = context.builder.CreateAlloca(var_type, 0, data.name.c_str());
            context.builder.CreateStore(value, alloc);

            func.variables.push_back( Variable{data.name, data.type, alloc} );
            break;
        }
    case STATEMENTID_ASSIGN:
        {
            AssignStatement data = *( static_cast<AssignStatement*>(statement.data) );
            Variable variable;
            llvm::Type* var_type;

            if(func.find_variable(data.name, &variable) != 0) return 1;

            if(program.find_type(variable.type, &var_type) != 0){
                std::cerr << "The type '" << variable.type << " does not exist" << std::endl;
                return 1;
            }

            llvm::Value* value = data.value->assemble(program, func, context);
            if(value == NULL) return 1;

            context.builder.CreateStore(value, variable.variable);
            break;
        }
    case STATEMENTID_CALL:
        {
            CallStatement data = *( static_cast<CallStatement*>(statement.data) );
            llvm::Function* target = context.module->getFunction(data.name);
            if (!target){
                std::cerr << "Unknown Function '" << data.name << "', returning NULL" << std::endl;
                return 1;
            }

            // If argument mismatch error.
            if (target->arg_size() != data.args.size()){
                std::cerr << "Incorrect function arguments size for for '" << data.name << "', returning NULL" << std::endl;
                return 1;
            }

            std::vector<llvm::Value*> value_args;
            for(size_t i = 0, e = data.args.size(); i != e; ++i) {
                value_args.push_back( data.args[i]->assemble(program, func, context) );
                if (!value_args.back()) return 1;
            }

            context.builder.CreateCall(target, value_args, "calltmp");
            break;
        }
    }

    return 0;
}

