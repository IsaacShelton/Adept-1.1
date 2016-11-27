
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
#include "llvm/IR/Instructions.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"

#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/validate.h"

int assemble(AssembleContext& assemble, Configuration& config, Program& program){
    assemble.module = llvm::make_unique<llvm::Module>( filename_name(config.filename).c_str() , assemble.context);
    if(program.generate_types(assemble) != 0) return 1;

    for(size_t i = 0; i != program.externs.size(); i++){
        if(assemble_external(assemble, config, program, program.externs[i]) != 0) return 1;
    }

    for(size_t i = 0; i != program.functions.size(); i++){
        if(assemble_function(assemble, config, program, program.functions[i]) != 0) return 1;
    }

    for(size_t i = 0; i != program.functions.size(); i++){
        Function& func = program.functions[i];
        llvm::Function* llvm_function = assemble.module->getFunction(func.name);
        AssembleFunction& asm_func = func.asm_func;

        assemble.builder.SetInsertPoint(asm_func.body);
        bool terminated = false;

        for(size_t j = 0; j != func.statements.size(); j++){
            if(func.statements[j].id == STATEMENTID_RETURN) terminated = true;

            if(assemble_statement(assemble, config, program, func, asm_func, func.statements[j]) != 0){
                llvm_function->eraseFromParent();
                return 1;
            }

            if(terminated) break;
        }

        if(!terminated) assemble.builder.CreateBr(asm_func.quit);

        llvm::verifyFunction(*llvm_function);
    }

    // Print Assembler Time
    config.clock.print_since("Assembler Finished");
    config.clock.remember();

    if(!config.jit){
        std::string target_name = filename_change_ext(config.filename, "exe");
        std::string target_obj  = (config.obj)      ? filename_change_ext(config.filename, "obj") : "C:/Users/" + config.username + "/.adept/obj/object.obj";
        std::string target_bc   = (config.bytecode) ? filename_change_ext(config.filename, "bc")  : "C:/Users/" + config.username + "/.adept/obj/bytecode.bc";

        std::error_code error_str;
        llvm::raw_ostream* out = new llvm::raw_fd_ostream(target_bc.c_str(), error_str, llvm::sys::fs::F_None);
        llvm::WriteBitcodeToFile(assemble.module.get(), *out);
        out->flush();
        delete out; // LLVM ERROR: IO failure on output stream.

        system((std::string("llc \"") + target_bc + "\" -filetype=obj -o \"" + target_obj + "\"").c_str());

        if(!config.obj){
            system((std::string("C:/MinGW64/bin/gcc \"") + target_obj + std::string("\" C:/MinGW64/lib/gcc/x86_64-w64-mingw32/5.3.0/libstdc++.a -o ") + target_name).c_str());
        }

        if(access(target_name.c_str(), F_OK ) == -1){
            fail( FAILED_TO_CREATE(filename_name(target_name)) );
            return 1;
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
                die( UNDECLARED_TYPE(func.arguments[i].type) );
            }
            args[i] = llvm_type;
        }

        if(program.find_type(func.return_type, &llvm_type) != 0){
            die( UNDECLARED_TYPE(func.return_type) );
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);
        llvm_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, func.name, context.module.get());

        // Create a new basic block to start insertion into.
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context.context, "entry", llvm_function);
        llvm::BasicBlock* body = llvm::BasicBlock::Create(context.context, "body", llvm_function);
        llvm::BasicBlock* quit = llvm::BasicBlock::Create(context.context, "quit", llvm_function);

        context.builder.SetInsertPoint(entry);

        llvm::AllocaInst* exitval;
        if(!llvm_type->isVoidTy()){
            exitval = context.builder.CreateAlloca(llvm_type, 0, "exitval");
        }

        size_t i = 0;
        func.variables.reserve(args.size());

        for(auto& arg : llvm_function->args()){
            llvm::AllocaInst* alloca = context.builder.CreateAlloca(args[i], 0, func.arguments[i].name);
            context.builder.CreateStore(&arg, alloca);
            func.variables.push_back( Variable{func.arguments[i].name, func.arguments[i].type, alloca} );
            i++;
        }
        context.builder.CreateBr(body);

        context.builder.SetInsertPoint(quit);

        if(!llvm_type->isVoidTy()){
            llvm::Value* retval = context.builder.CreateLoad(exitval, "loadtmp");
            func.asm_func.exitval = exitval;
            context.builder.CreateRet(retval);
        }
        else {
            context.builder.CreateRet(NULL);
        }

        func.asm_func.entry = entry;
        func.asm_func.body = body;
        func.asm_func.quit = quit;
    }
    else {
        die( DUPLICATE_FUNC(func.name) );

        // If we wanting to insert into the existing function:
        /*
        if(!llvm_function) return 1;
        // Create a new basic block to start insertion into.
        llvm::BasicBlock *basic_block = llvm::BasicBlock::Create(context.context, "entry", llvm_function);
        context.builder.SetInsertPoint(basic_block);
        */
    }

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
                die( UNDECLARED_TYPE(external.arguments[i]) );
            }
            args[i] = llvm_type;
        }

        if(program.find_type(external.return_type, &llvm_type) != 0){
            die( UNDECLARED_TYPE(external.return_type) );
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);
        llvm_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, external.name, context.module.get());
    }
    else {
        die( DUPLICATE_FUNC(external.name) );
    }

    // USE IF ERROR
    // Error reading body, remove function.
    // llvm_function->eraseFromParent();

    return 0;
}
int assemble_statement(AssembleContext& context, Configuration& config, Program& program, Function& func, AssembleFunction& asm_func, Statement& statement){

    switch(statement.id){
    case STATEMENTID_RETURN:
        {
            ReturnStatement data = *( static_cast<ReturnStatement*>(statement.data) );

            if(!validate_return_statement(program, data, func)) return 1;

            if(data.value != NULL){
                llvm::Value* return_value = data.value->assemble(program, func, context);
                if(return_value == NULL) return 1;

                context.builder.CreateStore(return_value, asm_func.exitval);
                context.builder.CreateBr(asm_func.quit);
            }
            else {
                context.builder.CreateBr(asm_func.quit);
            }

            break;
        }
    case STATEMENTID_DECLARE:
        {
            // Plain declare statements don't need to be validated

            DeclareStatement data = *( static_cast<DeclareStatement*>(statement.data) );
            llvm::Type* var_type;

            if(program.find_type(data.type, &var_type) != 0){
                die( UNDECLARED_TYPE(data.type) );
            }

            llvm::AllocaInst* alloc = context.builder.CreateAlloca(var_type, 0, data.name.c_str());
            func.variables.push_back( Variable{data.name, data.type, alloc} );
            break;
        }
    case STATEMENTID_DECLAREAS:
        {
            DeclareAsStatement data = *( static_cast<DeclareAsStatement*>(statement.data) );
            llvm::Type* var_type;
            std::string var_type_name;

            if(!validate_declareas_statement(program, data, func)) return 1;
            if(program.find_type(data.type, &var_type) != 0) die( UNDECLARED_TYPE(data.type) );

            llvm::Value* value = data.value->assemble(program, func, context);
            if(!data.value->getType(program, func, var_type_name)) return 1;
            if(value == NULL) return 1;

            if(assemble_merge_types_oneway(context, var_type_name, data.type, &value, var_type, NULL) != 0){
                die( INCOMPATIBLE_TYPES(var_type_name, data.type) );
            }

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
            if(!validate_assign_statement(program, data, variable, func)) return 1;
            if(program.find_type(variable.type, &var_type) != 0) die( UNDECLARED_TYPE(variable.type) );

            llvm::Value* value = data.value->assemble(program, func, context);
            if(value == NULL) return 1;

            context.builder.CreateStore(value, variable.variable);
            break;
        }
    case STATEMENTID_ASSIGNMEMBER:
        {
            AssignMemberStatement data = *( static_cast<AssignMemberStatement*>(statement.data) );

            int index;
            Structure target;
            Variable variable;
            llvm::Type* struct_type;
            llvm::Value* member_ptr;

            if(func.find_variable(data.path[0], &variable) != 0){
                die( UNDECLARED_VARIABLE(data.path[0]) );
            }
            if(program.find_struct(variable.type, &target) != 0){
                die( UNDECLARED_STRUCT(variable.type) );
            }
            if(target.find_index(data.path[1], &index) != 0){
                die( UNDECLARED_MEMBER(data.path[1], target.name) );
            }
            member_ptr = variable.variable;

            llvm::Value* member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, true));
            std::vector<llvm::Value*> indices(2);
            indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
            indices[1] = member_index;

            if(program.find_type(variable.type, &struct_type) != 0){
                die( UNDECLARED_TYPE(variable.type) );
            }
            member_ptr = context.builder.CreateGEP(struct_type, member_ptr, indices, "memberptr");

            // var, member, member
            //        ^
            //    member_ptr

            for(size_t i = 2; i != data.path.size(); i++){
                std::string struct_name = target.members[index].type;
                if(program.find_struct(struct_name, &target) != 0){
                    die( UNDECLARED_STRUCT(target.members[index].type) );
                }
                if(target.find_index(data.path[i], &index) != 0){
                    die( UNDECLARED_MEMBER(data.path[i], target.name) );
                    return 1;
                }

                member_index = llvm::ConstantInt::get(context.context, llvm::APInt(32, index, true));
                indices[0] = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
                indices[1] = member_index;

                if(program.find_type(struct_name, &struct_type) != 0){
                    die( UNDECLARED_STRUCT(struct_name) );
                    return 1;
                }

                member_ptr = context.builder.CreateGEP(struct_type, member_ptr, indices, "memberptr");
            }

            llvm::Value* value = data.value->assemble(program, func, context);
            if(value == NULL) return 1;

            context.builder.CreateStore(value, member_ptr);
            break;
        }
    case STATEMENTID_CALL:
        {
            CallStatement data = *( static_cast<CallStatement*>(statement.data) );
            llvm::Function* target = context.module->getFunction(data.name);
            External func_data;

            if (!target){
                die( UNDECLARED_FUNC(data.name) );
            }

            if(program.find_func(data.name, &func_data) != 0) return 1;
            assert(func_data.arguments.size() == target->arg_size());

            // If argument mismatch error.
            if (target->arg_size() != data.args.size()){
                std::cerr << "Incorrect function arguments size for for '" << data.name << "'" << std::endl;
                return 1;
            }

            llvm::Value* value;
            std::string valuetype;
            llvm::Type* expected_type;
            std::vector<llvm::Value*> value_args;
            for(size_t i = 0, e = data.args.size(); i != e; i++) {
                value = data.args[i]->assemble(program, func, context);
                if(!data.args[i]->getType(program, func, valuetype)){
                    die(INCOMPATIBLE_TYPES_VAGUE);
                }
                if(program.find_type(func_data.arguments[i], &expected_type) != 0){
                    die( UNDECLARED_TYPE(func_data.arguments[i]) );
                }

                if(assemble_merge_types_oneway(context, valuetype, func_data.arguments[i], &value, expected_type, NULL) != 0){
                    die( INCOMPATIBLE_TYPES(valuetype, func_data.arguments[i]) );
                }

                value_args.push_back(value);
                if(value == NULL) return 1;
                if (!value_args.back()) return 1;
            }

            context.builder.CreateCall(target, value_args, "calltmp");
            break;
        }
    }

    return 0;
}
int assemble_merge_types(AssembleContext& context, const std::string& type_a, const std::string& type_b, llvm::Value** expr_a, llvm::Value** expr_b, std::string* out){
    // Merge a and b if possible

    if(type_a == "void" or type_b == "void") return 1;
    if(type_a == "" or type_b == "") return 1;
    if(type_a == type_b){
        if(out != NULL) *out = type_a;
        return 0;
    }
    if(type_a[0] == '*' and type_b == "ptr"){
        if(out != NULL) *out = type_a;
        *expr_b = context.builder.CreateBitCast(*expr_b, (*expr_a)->getType(), "casttmp");
        return 0;
    }
    if(type_b[0] == '*' and type_a == "ptr"){
        if(out != NULL) *out = type_b;
        *expr_a = context.builder.CreateBitCast(*expr_a, (*expr_b)->getType(), "casttmp");
        return 0;
    }

    return 1;
}
int assemble_merge_types_oneway(AssembleContext& context, const std::string& type_a, const std::string& type_b, llvm::Value** expr_a, llvm::Type* exprtype_b, std::string* out){
    // Merge a into b if possible

    if(type_a == "void" or type_b == "void") return 1;
    if(type_a == "" or type_b == "void") return 1;
    if(type_a == type_b){
        if(out != NULL) *out = type_b;
        return 0;
    }
    if(type_b[0] == '*' and type_a == "ptr"){
        if(out != NULL) *out = type_b;
        *expr_a = context.builder.CreateBitCast(*expr_a, exprtype_b, "casttmp");
        return 0;
    }
    if(type_a[0] == '*' and type_b == "ptr"){
        if(out != NULL) *out = type_b;
        *expr_a = context.builder.CreateBitCast(*expr_a, llvm::Type::getInt8PtrTy(context.context), "casttmp");
        return 0;
    }

    return 1;
}
