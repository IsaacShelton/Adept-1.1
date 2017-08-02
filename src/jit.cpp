
#include <iostream>
#include <unistd.h>
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
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"

#include "../include/die.h"
#include "../include/jit.h"
#include "../include/config.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/mangling.h"

void jit_init(){
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}
int jit_run(Config* config, AssemblyData& context, const std::string& func_name, std::vector<ModuleDependency>& dependencies, std::string& result, std::vector<llvm::GenericValue> args){
    std::string error_str;
    llvm::Function* entry_point = context.module->getFunction(func_name.c_str());
    jit_init();

    if(!entry_point) {
        std::cout << "Can't invoke function '" + func_name + "' because it does not exist" << std::endl;
        return 1;
    }

    llvm::Triple triple(context.module->getTargetTriple());
    if (triple.getTriple().empty()) triple.setTriple(llvm::sys::getDefaultTargetTriple());
    context.module->setTargetTriple(triple.str());

    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple.str(), error_str);

    if(!target){
        std::cerr << error_str << std::endl;
        return 1;
    }

    llvm::SubtargetFeatures subtarget_features;
    llvm::StringMap<bool> host_features;
    if(llvm::sys::getHostCPUFeatures(host_features)){
        for (auto &host_feature : host_features){
            subtarget_features.AddFeature(host_feature.first(), host_feature.second);
        }
    }

    std::string cpu = llvm::sys::getHostCPUName();
    std::string features = subtarget_features.getString();
    llvm::TargetOptions target_options;
    std::unique_ptr<llvm::TargetMachine> target_machine(
            target->createTargetMachine(triple.getTriple(), cpu, features, target_options, llvm::Reloc::Model::Static, llvm::CodeModel::Default, llvm::CodeGenOpt::Default));
    ensure(target_machine && "Could not allocate target machine!");
    context.module->setDataLayout(target_machine->createDataLayout());

    // Create execution engine
    llvm::ExecutionEngine* execution_engine = llvm::EngineBuilder(std::move(context.module)).setErrorStr(&error_str).create();

    // Ensure execution engine was created successfully
    if(!execution_engine){
        std::cerr << "Failed to construct ExecutionEngine: " << error_str << std::endl;
        return 1;
    }

    // Add all of the dependencies to the execution engine
    for(size_t i = 0; i != dependencies.size(); i++){
        ModuleDependency* dependency = &dependencies[i];

        if(!dependency->is_nothing){
            llvm::SMDiagnostic sm_diagnostic;
            std::unique_ptr<llvm::Module> required_module = llvm::parseIRFile(dependency->target_bc, sm_diagnostic, context.context);

            if (!required_module) {
                sm_diagnostic.print("Failed to parse IR File: ", llvm::errs());
                return false;
            }

            required_module->setModuleIdentifier(dependency->target_bc.c_str());
            execution_engine->addModule(std::move(required_module));
        }
    }

    // Load adept core
    llvm::SMDiagnostic sm_diagnostic;
    std::unique_ptr<llvm::Module> adept_core_module = llvm::parseIRFile("C:/Users/" + config->username + "/.adept/obj/core/dyncore.bc", sm_diagnostic, context.context);
    if (!adept_core_module) {
        sm_diagnostic.print("Failed to parse IR File (dyncore.bc) : ", llvm::errs());
        return false;
    }
    adept_core_module->setModuleIdentifier("__adept_core__");
    execution_engine->addModule(std::move(adept_core_module));

    execution_engine->finalizeObject();
    config->time_verbose_clock.remember();
    llvm::GenericValue return_value = execution_engine->runFunction(entry_point, args);

    result = return_value.IntVal.toString(10, true);
    return 0;
}
int jit_run_main(Config* config, AssemblyData& context, std::vector<ModuleDependency>& dependencies, int& result){
    std::string error_str;
    llvm::Function* entry_point = context.module->getFunction("main");
    jit_init();

    if(!entry_point) {
        std::cout << "Can't invoke function 'main' because it does not exist" << std::endl;
        return 1;
    }

    llvm::Triple triple(context.module->getTargetTriple());
    if (triple.getTriple().empty()) triple.setTriple(llvm::sys::getDefaultTargetTriple());
    context.module->setTargetTriple(triple.str());

    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple.str(), error_str);

    if(!target){
        std::cerr << error_str << std::endl;
        return 1;
    }

    llvm::SubtargetFeatures subtarget_features;
    llvm::StringMap<bool> host_features;
    if(llvm::sys::getHostCPUFeatures(host_features)){
        for (auto &host_feature : host_features){
            subtarget_features.AddFeature(host_feature.first(), host_feature.second);
        }
    }

    std::string cpu = llvm::sys::getHostCPUName();
    std::string features = subtarget_features.getString();
    llvm::TargetOptions target_options;
    std::unique_ptr<llvm::TargetMachine> target_machine(
            target->createTargetMachine(triple.getTriple(), cpu, features, target_options, llvm::Reloc::Model::Static, llvm::CodeModel::Default, llvm::CodeGenOpt::Default));
    ensure(target_machine && "Could not allocate target machine!");
    context.module->setDataLayout(target_machine->createDataLayout());

    // Create execution engine
    llvm::ExecutionEngine* execution_engine = llvm::EngineBuilder(std::move(context.module)).setErrorStr(&error_str).create();

    // Ensure execution engine was created successfully
    if(!execution_engine){
        std::cerr << "Failed to construct ExecutionEngine: " << error_str << std::endl;
        return 1;
    }

    // Add all of the dependencies to the execution engine
    for(size_t i = 0; i != dependencies.size(); i++){
        ModuleDependency* dependency = &dependencies[i];

        if(!dependency->is_nothing){
            llvm::SMDiagnostic sm_diagnostic;
            std::unique_ptr<llvm::Module> required_module = llvm::parseIRFile(dependency->target_bc, sm_diagnostic, context.context);

            if (!required_module) {
                sm_diagnostic.print("Failed to parse IR File: ", llvm::errs());
                return false;
            }

            required_module->setModuleIdentifier(dependency->target_bc.c_str());
            execution_engine->addModule(std::move(required_module));
        }
    }

    // Load adept core
    llvm::SMDiagnostic sm_diagnostic;
    std::unique_ptr<llvm::Module> adept_core_module = llvm::parseIRFile("C:/Users/" + config->username + "/.adept/obj/core/dyncore.bc", sm_diagnostic, context.context);
    if (!adept_core_module) {
        sm_diagnostic.print("Failed to parse IR File (dyncore.bc) : ", llvm::errs());
        return false;
    }
    adept_core_module->setModuleIdentifier("__adept_core__");
    execution_engine->addModule(std::move(adept_core_module));

    execution_engine->finalizeObject();
    config->time_verbose_clock.remember();
    std::vector<std::string> args(1);
    args[0] = "<your-program-name>";
    result = execution_engine->runFunctionAsMain(entry_point, args, NULL);
    return 0;
}
