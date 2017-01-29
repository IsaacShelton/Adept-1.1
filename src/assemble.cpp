
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
#include "llvm/Transforms/Utils/Cloning.h"

#include "../include/jit.h"
#include "../include/build.h"
#include "../include/errors.h"
#include "../include/native.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/mangling.h"

int build(AssembleContext& context, Configuration& config, Program& program, ErrorHandler& errors){
    std::string target_name = filename_change_ext(config.filename, "exe");
    std::string target_obj  = (config.obj)      ? filename_change_ext(config.filename, "obj") : "C:/Users/" + config.username + "/.adept/obj/object.o";
    std::string target_bc   = (config.bytecode) ? filename_change_ext(config.filename, "bc")  : "C:/Users/" + config.username + "/.adept/obj/bytecode.bc";
    std::vector<ModuleDependency> dependencies;
    std::string linked_objects;
    ModuleBuildOptions module_build_options;

    // Configure module_build_options using config
    module_build_options.optimization = config.optimization;

    std::string build_result;
    AssembleContext build_context;
    llvm::Function* build_function = context.module->getFunction("build");

    if(build_function != NULL){
        // Prepare to run 'build()'
        adept_current_program = &program;
        adept_current_config = &config;

        adept_default_build_config.sourceFilename = config.filename.c_str();
        adept_default_build_config.programFilename = target_name.c_str();
        adept_default_build_config.objectFilename = target_obj.c_str();
        adept_default_build_config.bytecodeFilename = target_bc.c_str();
        adept_default_build_config.optimization = config.optimization;
        adept_default_build_config.time = config.time;
        adept_default_build_config.silent = config.silent;
        adept_default_build_config.jit = false;

        // Clone module that contains 'build()'
        build_context.module = llvm::CloneModule(context.module.get());

        // Run 'build()'
        if(jit_run(build_context, "build", build_result) != 0) return 1;
        if(build_result != "0") return 1;

        return 0;
    }

    std::error_code error_str;
    llvm::raw_ostream* out_stream;

    out_stream = new llvm::raw_fd_ostream(target_bc.c_str(), error_str, llvm::sys::fs::F_None);
    llvm::WriteBitcodeToFile(context.module.get(), *out_stream);
    out_stream->flush();
    delete out_stream;

    for(size_t i = 0; i != program.imports.size(); i++){
        AssembleContext import_context;
        ModuleDependency* dependency = &program.imports[i];

        if(assemble(import_context, *dependency->config, *dependency->program, errors) != 0) return 1;

        if(dependency->program->functions.size() != 0){
            out_stream = new llvm::raw_fd_ostream(dependency->target_bc.c_str(), error_str, llvm::sys::fs::F_None);
            llvm::WriteBitcodeToFile(import_context.module.get(), *out_stream);
            out_stream->flush();
            delete out_stream;

            native_build_module(context, dependency->target_bc, dependency->target_obj, module_build_options);
            linked_objects += "\"" + dependency->target_obj + "\" ";
        }

        delete program.imports[i].config;
        delete program.imports[i].program;
    }

    for(const std::string& lib : program.extra_libs){
        linked_objects += "\"" + lib + "\" ";
    }

    native_build_module(context, target_bc, target_obj, module_build_options);

    // Print Native Export Time
    if(config.time and !config.silent){
        config.clock.print_since("NATIVE DONE", filename_name(config.filename));
        config.clock.remember();
    }

    if(config.link){
        // TODO: Replace 'system' call
        // NOTE: '-Wl,--start-group' can be used disable smart linking and allow any order of linking
        system( ("C:/MinGW64/bin/gcc -Wl,--start-group \"" + target_obj + "\" " + linked_objects + " -o " + target_name).c_str() );

        if(access(target_name.c_str(), F_OK ) == -1){
            fail( FAILED_TO_CREATE(filename_name(target_name)) );
            return 1;
        }
    }

    // Print Linker Time
    if(config.time and !config.silent){
        config.clock.print_since("LINKER DONE", filename_name(config.filename));
        config.clock.remember();
    }

    return 0;
}
int assemble(AssembleContext& assemble, Configuration& config, Program& program, ErrorHandler& errors){
    assemble.module = llvm::make_unique<llvm::Module>( filename_name(config.filename).c_str(), assemble.context);
    if(program.generate_types(assemble) != 0) return 1;
    build_add_symbols();
    jit_init();

    for(size_t i = 0; i != program.externs.size(); i++){
        if(assemble_external(assemble, config, program, program.externs[i]) != 0) return 1;
    }

    for(size_t i = 0; i != program.functions.size(); i++){
        if(assemble_function(assemble, config, program, program.functions[i]) != 0) return 1;
    }

    for(size_t i = 0; i != program.functions.size(); i++){
        if(assemble_function_body(assemble, config, program, program.functions[i]) != 0) return 1;
    }

    // Print Assembler Time
    if(config.time and !config.silent){
        config.clock.print_since("ASSEMBLER DONE", filename_name(config.filename));
        config.clock.remember();
    }

    if(!config.jit and !config.obj and !config.bytecode and config.link){
        if(build(assemble, config, program, errors) != 0) return 1;
    }

    return 0;
}

int assemble_structure(AssembleContext& context, Configuration& config, Program& program, Structure& structure){
    // Currently structures don't require any special assembly
    return 0;
}
int assemble_function(AssembleContext& context, Configuration& config, Program& program, Function& func){
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );

    if(!llvm_function){
        llvm::Type* llvm_type;
        std::vector<llvm::Type*> args(func.arguments.size());

        // Throw an error if main is private
        if(func.name == "main" and !func.is_public){
            die(MAIN_IS_PRIVATE);
        }

        // Convert argument typenames to llvm types
        for(size_t i = 0; i != func.arguments.size(); i++){
            if(program.find_type(func.arguments[i].type, &llvm_type) != 0){
                die( UNDECLARED_TYPE(func.arguments[i].type) );
            }
            args[i] = llvm_type;
        }

        // Convert return type typename to an llvm type
        if(program.find_type(func.return_type, &llvm_type) != 0){
            die( UNDECLARED_TYPE(func.return_type) );
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);;
        llvm_function = llvm::Function::Create(function_type, (func.is_public) ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage, mangle(func), context.module.get());

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

        context.builder.SetInsertPoint(quit);

        if(!llvm_type->isVoidTy()){
            llvm::Value* retval = context.builder.CreateLoad(exitval, "loadtmp");
            func.asm_func.exitval = exitval;
            context.builder.CreateRet(retval);
        }
        else {
            context.builder.CreateRet(NULL);
        }

        func.asm_func.return_type = llvm_type;
        func.asm_func.entry = entry;
        func.asm_func.body = body;
        func.asm_func.quit = quit;
    }
    else {
        die( DUPLICATE_FUNC(func.name) );
    }

    // USE IF ERROR
    // Error reading body, remove function.
    // llvm_function->eraseFromParent();

    return 0;
}
int assemble_function_body(AssembleContext& context, Configuration& config, Program& program, Function& func){
    llvm::Function* llvm_function = context.module->getFunction( mangle(func) );
    AssembleFunction& asm_func = func.asm_func;

    assert(llvm_function != NULL);
    context.builder.SetInsertPoint(asm_func.body);
    bool terminated = false;

    for(size_t i = 0; i != func.statements.size(); i++){
        if(func.statements[i]->isTerminator()) terminated = true;

        if(func.statements[i]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(asm_func.quit);

    context.builder.SetInsertPoint(asm_func.entry);
    context.builder.CreateBr(asm_func.body);

    llvm::verifyFunction(*llvm_function);
    return 0;
}
int assemble_external(AssembleContext& context, Configuration& config, Program& program, External& external){
    std::string final_name = (external.is_mangled) ? mangle(external.name, external.arguments) : external.name;
    llvm::Function* llvm_function = context.module->getFunction(final_name);

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
        llvm_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, final_name, context.module.get());
    }
    else {
        for(ModuleDependency& m : program.imports){
            std::cout << m.name << std::endl;
        }

        std::cout << "hi" << std::endl;
        die( DUPLICATE_FUNC(external.name) );
    }

    // USE IF ERROR
    // Error reading body, remove function.
    // llvm_function->eraseFromParent();

    return 0;
}

void assemble_merge_conditional_types(AssembleContext& context, std::string& type, llvm::Value** expr){
    // If type isn't a boolean, try to convert it to one
    // TODO: Clean up code in 'if' blocks

    if(type == "bool") return;
    if(type == "void") return;
    if(type == "") return;

    if(type == "ubyte"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }
    else if(type == "byte"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }
    else if(type == "ushort"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }
    else if(type == "short"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }
    else if(type == "uint"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }
    else if(type == "int"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }
    else if(type == "ulong"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }
    else if(type == "long"){
        llvm::Value* zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    }

    return;
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
bool assemble_types_mergeable(const std::string& a, const std::string& b){
    // Checks if types can merge, (oneway conversion, merging 'a' into a 'b')

    if(a == "" or b == "") return false;
    if(a == "void" or b == "void") return false;

    if(a == b) return true;
    if(b[0] == '*' and a == "ptr") return true;
    if(a[0] == '*' and b == "ptr") return true;

    return false;
}
