
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

#include <unistd.h>
#include "../include/build.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

BuildConfig* adept_build_config;
Program* adept_current_program;
Configuration* adept_current_config;

void build_add_symbols(){
    llvm::sys::DynamicLibrary::AddSymbol("adept.getBuildConfig", (void*) adept_getBuildConfig);
    llvm::sys::DynamicLibrary::AddSymbol("adept.compile", (void*) adept_compile);
    llvm::sys::DynamicLibrary::AddSymbol("adept.dependency.add", (void*) adept_dependency_add);
    llvm::sys::DynamicLibrary::AddSymbol("adept.dependency.addForced", (void*) adept_dependency_addForced);
    llvm::sys::DynamicLibrary::AddSymbol("adept.dependency.exists", (void*) adept_dependency_exists);
}

void build_config_to_config(Configuration* config){

}

extern "C" void* adept_getBuildConfig(){
    return adept_build_config;
}

extern "C" int adept_compile(const char* file) {
    Program program;
    Configuration config;
    AssembleContext context;
    TokenList* tokens;

    tokens = new TokenList;
    if( configure(config, file)            != 0) return 1;
    if( tokenize(config, file, tokens)     != 0 ) return 1;
    if( parse(config, tokens, program)     != 0 ) return 1;
    if( assemble(context, config, program) != 0 ) return 1;
    if( finalize(config, context)          != 0 ) return 1;
    return 0;
}

extern "C" void adept_dependency_add(const char* name){
    std::string local_file = filename_path(adept_current_config->filename) + name;
    std::string public_file = "C:/Users/" + adept_current_config->username + "/.adept/lib/" + name;

    if( access(local_file.c_str(), F_OK) != -1 ){
        name = local_file.c_str();
    }
    else if( access(public_file.c_str(), F_OK) != -1 ){
        name = public_file.c_str();
    }
    else {
        fail( UNKNOWN_MODULE(name) );
    }

    if( !adept_dependency_exists(name) ){
        adept_current_program->extra_libs.push_back(name);
    }
}

extern "C" void adept_dependency_addForced(const char* name){
    std::string local_file = filename_path(adept_current_config->filename) + name;
    std::string public_file = "C:/Users/" + adept_current_config->username + "/.adept/lib/" + name;

    if( access(local_file.c_str(), F_OK) != -1 ){
        name = local_file.c_str();
    }
    else if( access(public_file.c_str(), F_OK) != -1 ){
        name = public_file.c_str();
    }
    else {
        fail( UNKNOWN_MODULE(name) );
    }

    adept_current_program->extra_libs.push_back(name);
}

extern "C" bool adept_dependency_exists(const char* name){
    std::string dependency = name;
    bool exists = false;

    std::string local_file = filename_path(adept_current_config->filename) + name;
    std::string public_file = "C:/Users/" + adept_current_config->username + "/.adept/lib/" + name;

    if( access(local_file.c_str(), F_OK) != -1 ){
        dependency = local_file;
    }
    else if( access(public_file.c_str(), F_OK) != -1 ){
        dependency = public_file;
    }
    else {
        fail( UNKNOWN_MODULE(name) );
    }

    for(size_t i = 0; i != adept_current_program->extra_libs.size(); i++){
        if(adept_current_program->extra_libs[i] == dependency){
            exists = true;
            break;
        }
    }

    return exists;
}
