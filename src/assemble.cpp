
#include <string>
#include <future>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
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
#include "../include/die.h"
#include "../include/build.h"
#include "../include/errors.h"
#include "../include/search.h"
#include "../include/native.h"
#include "../include/threads.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/mangling.h"

int build(AssemblyData& context, Configuration& config, Program& program, ErrorHandler& errors){
    if(filename_name(config.filename) == "build.adept"){
        // -=- Build script -=-
        return build_buildscript(context, config, program, errors);
    }
    else {
        // -=- Build  -=-
        return build_program(context, config, program, errors);
    }
}
int build_program(AssemblyData& context, Configuration& config, Program& program, ErrorHandler& errors){
    std::string target_name = filename_change_ext(config.filename, "exe");
    std::string target_obj  = (config.obj)      ? filename_change_ext(config.filename, "obj") : "C:/Users/" + config.username + "/.adept/obj/object.o";
    std::string target_bc   = (config.bytecode) ? filename_change_ext(config.filename, "bc")  : "C:/Users/" + config.username + "/.adept/obj/bytecode.bc";
    std::vector<ModuleDependency> dependencies;
    std::vector<std::string> linked_objects;
    ModuleBuildOptions module_build_options;
    std::error_code error_str;
    llvm::raw_ostream* out_stream;

    // Configure module_build_options using config
    module_build_options.optimization = config.optimization;

    out_stream = new llvm::raw_fd_ostream(target_bc.c_str(), error_str, llvm::sys::fs::F_None);
    llvm::WriteBitcodeToFile(context.module.get(), *out_stream);
    out_stream->flush();
    delete out_stream;

    for(size_t i = 0; i != program.dependencies.size(); i++){
        AssemblyData import_context;
        ModuleDependency* dependency = &program.dependencies[i];

        if(std::find(linked_objects.begin(), linked_objects.end(), dependency->target_obj) != linked_objects.end()){
            // We've already linked to that object file
            continue;
        }

        Program* dependency_program = program.parent_manager->getProgram(dependency->filename);
        std::cout << 1 << std::endl;
        if(assemble(import_context, *dependency->config, *dependency_program, errors) != 0) return 1;

        std::cout << 2 << std::endl;
        if(dependency_program->functions.size() != 0 or dependency_program->classes.size() != 0){
            out_stream = new llvm::raw_fd_ostream(dependency->target_bc.c_str(), error_str, llvm::sys::fs::F_None);
            std::cout << "a" << std::endl;
            llvm::WriteBitcodeToFile(import_context.module.get(), *out_stream);
            std::cout << "b" << std::endl;
            out_stream->flush();
            std::cout << 3 << std::endl;
            delete out_stream;
            std::cout << 4 << std::endl;
            native_build_module(context, dependency->target_bc, dependency->target_obj, module_build_options);
            linked_objects.push_back(dependency->target_obj);
            std::cout << 5 << std::endl;
        }
    }

    // Link to the minimal Adept core
    linked_objects.push_back("C:/Users/" + config.username + "/.adept/obj/core/core.o");

    for(const std::string& lib : program.extra_libs){
        linked_objects.push_back(lib);
    }

    native_build_module(context, target_bc, target_obj, module_build_options);

    // Print Native Export Time
    if(config.time and !config.silent){
        config.clock.print_since("NATIVE DONE", filename_name(config.filename));
        config.clock.remember();
    }

    if(config.link){
        std::string linked_objects_string;

        for(const std::string& obj : linked_objects){
            linked_objects_string += "\"" + obj + "\" ";
        }

        // TODO: Replace 'system' call
        // NOTE: '-Wl,--start-group' can be used disable smart linking and allow any order of linking
        std::string command = "\"\"C:\\Users\\" + config.username + "\\.adept\\mingw64\\bin\\gcc.exe\" -Wl,--start-group \"" + target_obj + "\" " + config.extra_options + linked_objects_string + " -o \"" + target_name + "\"\"";
        system(command.c_str());

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
int build_buildscript(AssemblyData& context, Configuration& config, Program& program, ErrorHandler& errors){
    std::string build_result;
    AssemblyData build_context;
    llvm::Function* build_function = context.module->getFunction("build");
    std::error_code error_str;
    llvm::raw_ostream* out_stream;
    ModuleBuildOptions module_build_options;
    std::string target_bc   = (config.bytecode) ? filename_change_ext(config.filename, "bc")  : "C:/Users/" + config.username + "/.adept/obj/bytecode.bc";

    if(build_function == NULL){
        // Build function does not exist
        fail_filename(config, "Can't invoke build script because of missing 'build' function");
        return 1;
    }

    // Configure module_build_options using config
    module_build_options.optimization = config.optimization;

    out_stream = new llvm::raw_fd_ostream(target_bc.c_str(), error_str, llvm::sys::fs::F_None);
    llvm::WriteBitcodeToFile(context.module.get(), *out_stream);
    out_stream->flush();
    delete out_stream;

    for(size_t i = 0; i != program.dependencies.size(); i++){
        AssemblyData import_context;
        ModuleDependency* dependency = &program.dependencies[i];
        Program* dependency_program = program.parent_manager->getProgram(dependency->filename);

        dependency->config->clock.remember();
        if(assemble(import_context, *dependency->config, *dependency_program, errors) != 0) return 1;

        if(!dependency->is_nothing){
            out_stream = new llvm::raw_fd_ostream(dependency->target_bc.c_str(), error_str, llvm::sys::fs::F_None);
            llvm::WriteBitcodeToFile(import_context.module.get(), *out_stream);
            out_stream->flush();
            delete out_stream;
        }
    }

    // Prepare to run 'build()'
    current_global_cache_manager = program.parent_manager;

    // Clone module that contains 'build()'
    build_context.module = llvm::CloneModule(context.module.get());

    // Run 'build()'
    if(jit_run(&config, build_context, "build", program.dependencies, build_result) != 0) return 1;
    if(build_result != "0") return 1;

    return 0;
}
int assemble(AssemblyData& context, Configuration& config, Program& program, ErrorHandler& errors){
    context.module = llvm::make_unique<llvm::Module>( filename_name(config.filename).c_str(), context.context);
    if(program.generate_types(context) != 0) return 1;
    build_add_symbols();
    jit_init();

    // Create assembly data for globals
    for(size_t i = 0; i != program.globals.size(); i++){
        context.addGlobal(program.globals[i].name);
    }

    size_t thread_count = 8;
    size_t target_batch_size;
    size_t overflow_batch_size;
    std::vector<std::future<int>> futures;

    if(program.globals.size() > 8){
        // Divide work into threads (not the most elegant solution but it's good enough for now)
        // For instance if we wanted to assemble 253 globals in 8 theads, the threads whould each responsible
        // for the following amount of globals:

        // [00]  [01]  [02]  [03]  [04]  [05]  [06]  [07]
        //  31,   31,   31,   31,   31,   31,   31,   36

        // Start on globals
        target_batch_size = program.globals.size() / (thread_count);
        overflow_batch_size = target_batch_size + (program.globals.size() % thread_count);
        for(size_t i = 0; i < program.globals.size(); ){
            if(program.globals.size()-i != overflow_batch_size){
                futures.push_back( std::async(assemble_globals_batch, &context, &config, &program, &program.globals[i], target_batch_size) );
                i += target_batch_size;
            }
            else {
                futures.push_back( std::async(assemble_globals_batch, &context, &config, &program, &program.globals[i], overflow_batch_size) );
                break;
            }
        }
        // Wait for globals to finish and check if any errors were found
        if(threads_int_result(futures) != 0) return 1;
    }
    else {
        if(assemble_globals_batch(&context, &config, &program, &program.globals[0], program.globals.size()) != 0) return 1;
    }

    // Start on externals
    if(program.externs.size() > 8){
        futures.clear();
        target_batch_size = program.externs.size() / (thread_count);
        overflow_batch_size = target_batch_size + (program.externs.size() % thread_count);
        for(size_t i = 0; i < program.externs.size(); ){
            if(program.externs.size()-i != overflow_batch_size){
                futures.push_back( std::async(assemble_externals_batch, &context, &config, &program, &program.externs[i], target_batch_size) );
                i += target_batch_size;
            }
            else {
                futures.push_back( std::async(assemble_externals_batch, &context, &config, &program, &program.externs[i], overflow_batch_size) );
                break;
            }
        }
        // Wait for externals to finish and check if any errors were found
        if(threads_int_result(futures) != 0) return 1;
    }
    else {
        if(assemble_externals_batch(&context, &config, &program, &program.externs[0], program.externs.size()) != 0) return 1;
    }

    // Assemble the skeleton of each class
    // ASYNC: Maybe parallelize this?
    for(size_t i = 0; i != program.classes.size(); i++){
        if(assemble_class(context, config, program, program.classes[i]) != 0) return 1;
    }

    // Assemble the skeleton of each function
    // ASYNC: Maybe parallelize this?
    for(size_t i = 0; i != program.functions.size(); i++){
        if(assemble_function(context, config, program, program.functions[i]) != 0) return 1;
    }

    // Assemble the bodies of each class
    // ASYNC: Maybe parallelize this?
    for(size_t i = 0; i != program.classes.size(); i++){
        if(assemble_class_body(context, config, program, program.classes[i]) != 0) return 1;
    }

    // Assemble the bodies of each function
    // ASYNC: Maybe parallelize this?
    for(size_t i = 0; i != program.functions.size(); i++){
        if(assemble_function_body(context, config, program, program.functions[i]) != 0) return 1;
    }

    // Print Assembler Time
    if(config.time and !config.silent){
        config.clock.print_since("ASSEMBLER DONE", filename_name(config.filename));
        config.clock.remember();
    }

    if(!config.jit and !config.obj and !config.bytecode and config.link){
        if(build(context, config, program, errors) != 0) return 1;
    }

    return 0;
}

int assemble_globals_batch(AssemblyData* context, const Configuration* config, const Program* program, Global* globals, size_t globals_count){
    // ASYNC: Assembles a chunk of globals, intended for threading purposes

    for(size_t g = 0; g != globals_count; g++){
        if(assemble_global(context, config, program, &globals[g]) != 0){
            return 1;
        }
    }

    return 0;
}
int assemble_externals_batch(AssemblyData* context, const Configuration* config, const Program* program, External* externs, size_t externs_count){
    // ASYNC: Assembles a chunk of globals, intended for threading purposes

    for(size_t e = 0; e != externs_count; e++){
        if(assemble_external(context, config, program, &externs[e]) != 0){
            return 1;
        }
    }

    return 0;
}

int assemble_structure(AssemblyData& context, Configuration& config, Program& program, Structure& structure){
    // Currently structures don't require any special assembly
    return 0;
}
int assemble_class(AssemblyData& context, Configuration& config, Program& program, Class& klass){
    for(size_t i = 0; i != klass.methods.size(); i++){
        if(assemble_method(context, config, program, klass, klass.methods[i], klass.is_imported) != 0) return 1;
    }
    return 0;
}
int assemble_class_body(AssemblyData& context, Configuration& config, Program& program, Class& klass){
    // No not the class body you're thinking of
    // ==========        x        ==========
    // =====================================
    //
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

    // Get it? Since the name of this function is 'assemble_class_body'
    // Anyways....

    if(klass.is_imported){
        // Don't assemble the class if it was imported
        return 0;
    }

    for(size_t i = 0; i != klass.methods.size(); i++){
        if(assemble_method_body(context, config, program, klass, klass.methods[i]) != 0) return 1;
    }

    return 0;
}
int assemble_function(AssemblyData& context, Configuration& config, Program& program, Function& func){
    std::string mangled_function_name = mangle(program, func);
    llvm::Function* llvm_function = context.module->getFunction(mangled_function_name);

    if(!llvm_function){
        llvm::Type* llvm_type;
        std::vector<llvm::Type*> args(func.arguments.size());

        // Throw an error if main is private
        if(func.name == "main" and !func.is_public){
            fail_filename(config, MAIN_IS_PRIVATE);
            return 1;
        }

        // Convert argument typenames to llvm types
        for(size_t i = 0; i != func.arguments.size(); i++){
            if(program.find_type(func.arguments[i].type, context, &llvm_type) != 0){
                fail_filename(config, UNDECLARED_TYPE(func.arguments[i].type));
                return 1;
            }
            args[i] = llvm_type;
        }

        // Convert return type typename to an llvm type
        if(program.find_type(func.return_type, context, &llvm_type) != 0){
            fail_filename(config, UNDECLARED_TYPE(func.return_type));
            return 1;
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);
        llvm_function = llvm::Function::Create(function_type, (func.is_public) ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage, mangle(program, func), context.module.get());

        // Create a new basic block to start insertion into.
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context.context, "entry", llvm_function);
        llvm::BasicBlock* body = llvm::BasicBlock::Create(context.context, "body", llvm_function);
        llvm::BasicBlock* quit = llvm::BasicBlock::Create(context.context, "quit", llvm_function);

        // Set insert point to entry
        context.builder.SetInsertPoint(entry);

        // Allocate return variable
        llvm::AllocaInst* exitval;
        if(!llvm_type->isVoidTy()){
            exitval = context.builder.CreateAlloca(llvm_type, 0, "exitval");
        }

        // Create function assembly data
        AssembleFunction* func_assembly_data = context.addFunction(mangled_function_name);
        func_assembly_data->entry = entry;
        func_assembly_data->body = body;
        func_assembly_data->quit = quit;
        func_assembly_data->return_type = llvm_type;
        func_assembly_data->exitval = exitval;

        size_t i = 0;
        func_assembly_data->variables.reserve(args.size());

        for(auto& arg : llvm_function->args()){
            llvm::AllocaInst* alloca = context.builder.CreateAlloca(args[i], 0, func.arguments[i].name);
            context.builder.CreateStore(&arg, alloca);
            func_assembly_data->addVariable(func.arguments[i].name, func.arguments[i].type, alloca);
            i++;
        }

        context.builder.SetInsertPoint(quit);

        if(!llvm_type->isVoidTy()){
            llvm::Value* retval = context.builder.CreateLoad(exitval, "loadtmp");
            context.builder.CreateRet(retval);
        }
        else {
            context.builder.CreateRet(NULL);
        }
    }
    else {
        fail_filename(config, DUPLICATE_FUNC(func.name));
        return 1;
    }

    return 0;
}
int assemble_function_body(AssemblyData& context, Configuration& config, Program& program, Function& func){
    std::string mangled_function_name = mangle(program, func);
    llvm::Function* llvm_function = context.module->getFunction(mangled_function_name);
    AssembleFunction* asm_func = context.getFunction(mangled_function_name);
    context.current_function = asm_func;

    if(asm_func == NULL){
        fail("Failed to get assembly data for function in assemble_function_body");
        fail(SUICIDE);
        return 1;
    }

    assert(llvm_function != NULL);
    context.builder.SetInsertPoint(asm_func->body);
    bool terminated = false;

    for(size_t i = 0; i != func.statements.size(); i++){
        if(func.statements[i]->isTerminator()) terminated = true;

        if(func.statements[i]->assemble(program, func, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(asm_func->quit);

    context.builder.SetInsertPoint(asm_func->entry);
    context.builder.CreateBr(asm_func->body);

    llvm::verifyFunction(*llvm_function);
    return 0;
}
int assemble_method(AssemblyData& context, Configuration& config, Program& program, Class& klass, Function& method, bool is_imported){
    std::string mangled_method_name = mangle(klass, method);
    llvm::Function* llvm_function = context.module->getFunction(mangled_method_name);

    if(!llvm_function){
        llvm::Type* llvm_type;
        std::vector<llvm::Type*> args(method.arguments.size()+1);

        // Get llvm type of pointer to structure of 'Class* klass'
        if(program.find_type(klass.name, context, &llvm_type) != 0){
            fail_filename(config, UNDECLARED_TYPE(klass.name));
            return 1;
        }
        args[0] = llvm_type->getPointerTo(); // First argument is always a pointer to the class data

        // Convert argument typenames to llvm types
        for(size_t i = 0; i != method.arguments.size(); i++){
            if(program.find_type(method.arguments[i].type, context, &llvm_type) != 0){
                fail_filename(config, UNDECLARED_TYPE(method.arguments[i].type));
                return 1;
            }
            args[i+1] = llvm_type;
        }

        // Convert return type typename to an llvm type
        if(program.find_type(method.return_type, context, &llvm_type) != 0){
            fail_filename(config, UNDECLARED_TYPE(method.return_type));
            return 1;
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);;
        llvm_function = llvm::Function::Create(function_type, (method.is_public or is_imported) ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage, mangled_method_name, context.module.get());
        assert(llvm_function != NULL);

        if(!is_imported){
            // Create a new basic block to start insertion into.
            llvm::BasicBlock* entry = llvm::BasicBlock::Create(context.context, "entry", llvm_function);
            llvm::BasicBlock* body = llvm::BasicBlock::Create(context.context, "body", llvm_function);
            llvm::BasicBlock* quit = llvm::BasicBlock::Create(context.context, "quit", llvm_function);

            context.builder.SetInsertPoint(entry);

            llvm::AllocaInst* exitval;
            if(!llvm_type->isVoidTy()){
                exitval = context.builder.CreateAlloca(llvm_type, 0, "exitval");
            }

            // Create function assembly data
            AssembleFunction* func_assembly_data = context.addFunction(mangled_method_name);
            func_assembly_data->entry = entry;
            func_assembly_data->body = body;
            func_assembly_data->quit = quit;
            func_assembly_data->return_type = llvm_type;
            func_assembly_data->exitval = exitval;

            size_t i = 0;
            func_assembly_data->variables.reserve(args.size() + 1);

            // Add variable for each argument
            for(auto& arg : llvm_function->args()){
                if(i == 0){
                    // Add variable for 'this'
                    llvm::AllocaInst* alloca = context.builder.CreateAlloca(args[i], 0, "this");
                    context.builder.CreateStore(&arg, alloca);
                    func_assembly_data->addVariable("this", "*" + klass.name, alloca);
                    i++;
                    continue;
                }

                Field& method_argument = method.arguments[i-1];
                llvm::AllocaInst* alloca = context.builder.CreateAlloca(args[i], 0, method_argument.name);
                context.builder.CreateStore(&arg, alloca);
                func_assembly_data->addVariable(method_argument.name, method_argument.type, alloca);
                i++;
            }

            context.builder.SetInsertPoint(quit);

            if(!llvm_type->isVoidTy()){
                llvm::Value* retval = context.builder.CreateLoad(exitval, "loadtmp");
                context.builder.CreateRet(retval);
            }
            else {
                context.builder.CreateRet(NULL);
            }
        }
    }
    else {
        fail_filename(config, DUPLICATE_METHOD(klass.name + "." + method.name));
        return 1;
    }

    return 0;
}
int assemble_method_body(AssemblyData& context, Configuration& config, Program& program, Class& klass, Function& method){
    std::string mangled_method_name = mangle(klass, method);
    llvm::Function* llvm_function = context.module->getFunction(mangled_method_name);
    AssembleFunction* asm_func = context.getFunction(mangled_method_name);
    context.current_function = asm_func;

    assert(llvm_function != NULL);
    context.builder.SetInsertPoint(asm_func->body);
    bool terminated = false;

    for(size_t i = 0; i != method.statements.size(); i++){
        if(method.statements[i]->isTerminator()) terminated = true;

        if(method.statements[i]->assemble(program, method, context) != 0){
            return 1;
        }

        if(terminated) break;
    }

    if(!terminated) context.builder.CreateBr(asm_func->quit);

    context.builder.SetInsertPoint(asm_func->entry);
    context.builder.CreateBr(asm_func->body);

    llvm::verifyFunction(*llvm_function);
    return 0;
}
int assemble_external(AssemblyData* context, const Configuration* config, const Program* program, const External* external){
    // ASYNC: This function is thread safe if used correctly:
    //   - No arguments should be modified until ensuring that this function has finished
    //   - All pointers passed to this function must be valid

    std::string final_name = (external->is_mangled) ? mangle(external->name, external->arguments) : external->name;
    llvm::Function* llvm_function = context->module->getFunction(final_name);

    if(!llvm_function){
        llvm::Type* llvm_type;
        std::vector<llvm::Type*> args(external->arguments.size());

        for(size_t i = 0; i != external->arguments.size(); i++){
            if(program->find_type(external->arguments[i], *context, &llvm_type) != 0){
                fail_filename(*config, UNDECLARED_TYPE(external->arguments[i]));
                return 1;
            }
            args[i] = llvm_type;
        }

        if(program->find_type(external->return_type, *context, &llvm_type) != 0){
            fail_filename(*config, UNDECLARED_TYPE(external->return_type));
            return 1;
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_type, args, false);
        llvm_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, final_name, context->module.get());
    }
    else {
        fail_filename(*config, DUPLICATE_FUNC(external->name));
        return 1;
    }

    return 0;
}
int assemble_global(AssemblyData* context, const Configuration* config, const Program* program, Global* global){
    // ASYNC: This function is thread safe if used correctly:
    //   - This function modifies the global passed to it, so it must not be modified until ensuring that this
    //     function has finished
    //   - No other arguments should be modified until ensuring that this function has finished
    //   - All pointers passed to this function must be valid

    llvm::Type* global_llvm_type;
    const bool is_constant = false;
    llvm::GlobalVariable* created_global;
    llvm::GlobalVariable::LinkageTypes linkage;
    AssembleGlobal* asm_global = context->findGlobal(global->name); // TODO add AssemblyData::findGlobal

    if(asm_global == NULL){
        global->errors.panic("Attempted to assemble global variable '" + global->name + "' but couldn't find assembly data");
        fail(SUICIDE);
        return 1;
    }

    if(program->find_type(global->type, *context, &global_llvm_type) != 0){
        fail_filename(*config, UNDECLARED_TYPE(global->type));
        return 1;
    }

    if(global->is_imported){
        linkage = llvm::GlobalVariable::LinkageTypes::ExternalLinkage;
    }
    else {
        if(global->is_public and !config->add_build_api and !config->jit){
            // If global variable is public and is not a part of a build script
            linkage = llvm::GlobalVariable::LinkageTypes::CommonLinkage;
        } else {
            // If global variable is private or is part of a build script
            linkage = llvm::GlobalVariable::LinkageTypes::InternalLinkage;
        }
    }

    created_global = new llvm::GlobalVariable(*(context->module.get()), global_llvm_type, is_constant,
                        linkage, nullptr, global->name);

    if(!global->is_imported){
        // TODO: Add support for native integer types as globals (currently only structs and pointers can be used)

        // Initialize the global as zero
        if(Program::is_function_typename(global->type) or Program::is_pointer_typename(global->type)){
            // Safety: Probally not very safe, but whatever. 'global_llvm_type' should always be a pointer
            assert(global_llvm_type->isPointerTy());
            created_global->setInitializer( llvm::ConstantPointerNull::get( static_cast<llvm::PointerType*>(global_llvm_type) ) );
        }
        else {
            created_global->setInitializer( llvm::ConstantAggregateZero::get(global_llvm_type) );
        }
    }

    created_global->setExternallyInitialized( (global->is_public or global->is_imported) ); // Assume externally initialized if public
    asm_global->variable = created_global;
    return 0;
}

void assemble_merge_conditional_types(AssemblyData& context, Program& program, std::string& type, llvm::Value** expr){
    // If type isn't a boolean, try to convert it to one

    // Resolve any aliases
    program.resolve_if_alias(type);

    if(type == "bool") return;
    if(type == "void") return;
    if(type == "") return;

    const size_t affected_types_size = 8;

    // NOTE: MUST be pre sorted alphabetically (used for string_search)
    //         Make sure to update switch statement with correct indices after add or removing a type
    const std::string affected_types[affected_types_size] = {
        "byte", "int", "long", "short",
        "ubyte", "uint", "ulong", "ushort"
    };

    llvm::Value* zero;
    int str_index = string_search(affected_types, affected_types_size, type);

    switch(str_index){
    case 0: // byte
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    case 1: // int
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    case 2: // long
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    case 3: // short
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, true));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    case 4: // ubyte
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(8, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    case 5: // uint
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(32, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    case 6: // ulong
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(64, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    case 7: // ushort
        zero = llvm::ConstantInt::get(context.context, llvm::APInt(16, 0, false));
        *expr = context.builder.CreateICmpNE(*expr, zero, "cmptmp");
        type = "bool";
        return;
    default:
        return;
    }
}
int assemble_merge_types(AssemblyData& context, Program& program, std::string type_a, std::string type_b, llvm::Value** expr_a, llvm::Value** expr_b, std::string* out){
    // Merge a and b if possible

    // Resolve any aliases
    program.resolve_if_alias(type_a);
    program.resolve_if_alias(type_b);

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
int assemble_merge_types_oneway(AssemblyData& context, Program& program, std::string type_a, std::string type_b, llvm::Value** expr_a, llvm::Type* exprtype_b, std::string* out){
    // Merge a into b if possible

    // Resolve any aliases
    program.resolve_if_alias(type_a);
    program.resolve_if_alias(type_b);

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

    if(Program::is_function_typename(type_a) and type_b == "ptr"){
        if(out != NULL) *out = type_b;
        llvm::Type* llvm_func_type;
        if(program.function_typename_to_type(type_a, context, &llvm_func_type) != 0) return 1;
        *expr_a = context.builder.CreateBitCast(*expr_a, llvm_func_type, "casttmp");
        return 0;
    }

    if(Program::is_function_typename(type_b) and type_a == "ptr"){
        if(out != NULL) *out = type_a;
        llvm::Type* llvm_func_type;
        if(program.function_typename_to_type(type_b, context, &llvm_func_type) != 0) return 1;
        *expr_a = context.builder.CreateBitCast(*expr_a, llvm_func_type, "casttmp");
        return 0;
    }

    return 1;
}
bool assemble_types_mergeable(Program& program, std::string a, std::string b){
    // Checks if types can merge, (oneway conversion, merging 'a' into a 'b')

    // Resolve any aliases
    program.resolve_if_alias(a);
    program.resolve_if_alias(b);

    if(a == "" or b == "") return false;
    if(a == "void" or b == "void") return false;

    if(a == b) return true;
    if(b[0] == '*' and a == "ptr") return true;
    if(a[0] == '*' and b == "ptr") return true;

    if(Program::is_function_typename(a)) return true;
    if(Program::is_function_typename(b)) return true;

    return false;
}
