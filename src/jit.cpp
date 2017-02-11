
#include <iostream>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
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
#include "../include/jit.h"
#include "../include/assemble.h"
#include "../include/mangling.h"

void jit_init(){
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}
int jit_run(AssembleContext& context, std::string func_name, std::string& result, std::vector<llvm::GenericValue> args){
    jit_init();
    llvm::Function *entry_point = context.module->getFunction( func_name.c_str() );
    if(!entry_point) {
        std::cout << "Can't invoke function '" + func_name + "' because it does not exist" << std::endl;
        return 1;
    }

    // Create execution environment
    std::string error_str;
    llvm::ExecutionEngine* execution_engine = llvm::EngineBuilder(std::move(context.module)).setErrorStr(&error_str).create();

    if(execution_engine != 0) {
        // Call main function
        llvm::GenericValue return_value = execution_engine->runFunction(entry_point, args);

        // Output results
        result = return_value.IntVal.toString(10, true);
    } else {
        std::cerr << "Failed to construct ExecutionEngine: " << error_str << std::endl;
        return 1;
    }

    return 0;
}
int jit_run(AssembleContext& context, std::string func_name, std::vector<ModuleDependency>& dependencies, ErrorHandler& errors, std::string& result, std::vector<llvm::GenericValue> args){
    jit_init();
    llvm::Function *entry_point = context.module->getFunction( func_name.c_str() );
    if(!entry_point) {
        std::cout << "Can't invoke function '" + func_name + "' because it does not exist" << std::endl;
        return 1;
    }

    // Create execution environment
    std::string error_str;
    llvm::ExecutionEngine* execution_engine = llvm::EngineBuilder(std::move(context.module)).setErrorStr(&error_str).create();

    if(execution_engine != 0) {
        if(dependencies.size() != 0){
            fail("Only one module JIT is currently supported :\\");
            return 1;
        }

        for(size_t i = 0; i != dependencies.size(); i++){
            AssembleContext import_context(false);
            ModuleDependency* dependency = &dependencies[i];

            if(assemble(import_context, *dependency->config, *dependency->program, errors) != 0) return 1;
            execution_engine->addModule(std::move(import_context.module));

            delete dependency->config;
            delete dependency->program;
        }

        // Call main function
        llvm::GenericValue return_value = execution_engine->runFunction(entry_point, args);

        // Output results
        result = return_value.IntVal.toString(10, true);
    } else {
        std::cerr << "Failed to construct ExecutionEngine: " << error_str << std::endl;
        return 1;
    }

    return 0;
}
