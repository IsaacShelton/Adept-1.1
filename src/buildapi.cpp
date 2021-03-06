
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
#include "llvm/Support/DynamicLibrary.h"

#include <unistd.h>
#include <boost/filesystem.hpp>
#include "../include/die.h"
#include "../include/cache.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/buildapi.h"
#include "../include/finalize.h"

CacheManager* current_global_cache_manager; // So build scripts can access program cache

// Functions to get standard streams
extern "C" FILE* fstdout(){ return stdout; }
extern "C" FILE* fstderr(){ return stderr; }
extern "C" FILE* fstdin(){ return stdin; }
extern "C" int* ferrno(){ return _errno(); }

// Function to add the symbols for the build script API so scripts can access them
void build_add_symbols(){
    llvm::sys::DynamicLibrary::AddSymbol("fstdout", (void*) fstdout);
    llvm::sys::DynamicLibrary::AddSymbol("fstderr", (void*) fstderr);
    llvm::sys::DynamicLibrary::AddSymbol("fstdin", (void*) fstdin);
    llvm::sys::DynamicLibrary::AddSymbol("ferrno", (void*) ferrno);

    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.create", (void*) buildscript_AdeptConfig_create);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.free", (void*) buildscript_AdeptConfig_free);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.defaults", (void*) buildscript_AdeptConfig_defaults);

    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.setTiming@bool", (void*) buildscript_AdeptConfig_setTiming);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.setSilent@bool", (void*) buildscript_AdeptConfig_setSilent);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.setJIT@bool", (void*) buildscript_AdeptConfig_setJIT);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.setOptimization@ubyte", (void*) buildscript_AdeptConfig_setOptimization);

    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.isTiming", (void*) buildscript_AdeptConfig_isTiming);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.isSilent", (void*) buildscript_AdeptConfig_isSilent);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.isJIT", (void*) buildscript_AdeptConfig_isJIT);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.getOptimization", (void*) buildscript_AdeptConfig_getOptimization);

    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.compile@*ubyte", (void*) buildscript_AdeptConfig_compile);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.loadLibrary@*ubyte", (void*) buildscript_AdeptConfig_loadLibrary);
    llvm::sys::DynamicLibrary::AddSymbol("AdeptConfig.addLinkerOption@*ubyte", (void*) buildscript_AdeptConfig_addLinkerOption);
}

// Function to create api in syntax tree
void build_add_api(Program* program){
    program->structures.resize(program->structures.size() + 1);
    Struct* structure = &program->structures[program->structures.size()-1];

    structure->name = "AdeptConfig";
    structure->flags = STRUCT_IMPORTED;
    structure->members.push_back( StructField{"", "ptr", 0x00} );

    structure->methods.resize(16);
    structure->methods[0] = Function("create", {}, "void", true, &program->origin_info);
    structure->methods[1] = Function("free", {}, "void", true, &program->origin_info);
    structure->methods[2] = Function("defaults", {}, "void", true, &program->origin_info);

    structure->methods[3] = Function("setTiming", { Field{"", "bool"} }, "void", true, &program->origin_info);
    structure->methods[4] = Function("setTimingVerbose", { Field{"", "bool"} }, "void", true, &program->origin_info);
    structure->methods[5] = Function("setSilent", { Field{"", "bool"} }, "void", true, &program->origin_info);
    structure->methods[6] = Function("setJIT", { Field{"", "bool"} }, "void", true, &program->origin_info);
    structure->methods[7] = Function("setOptimization", { Field{"", "ubyte"} }, "void", true, &program->origin_info);

    structure->methods[8] = Function("isTiming", {}, "bool", true, &program->origin_info);
    structure->methods[9] = Function("isTimingVerbose", {}, "bool", true, &program->origin_info);
    structure->methods[10] = Function("isSilent", {}, "bool", true, &program->origin_info);
    structure->methods[11] = Function("isJIT", {}, "bool", true, &program->origin_info);
    structure->methods[12] = Function("getOptimization", {}, "ubyte", true, &program->origin_info);

    structure->methods[13] = Function("compile", { Field{"", "*ubyte"} }, "int", true, &program->origin_info);
    structure->methods[14] = Function("loadLibrary", { Field{"", "*ubyte"} }, "void", true, &program->origin_info);
    structure->methods[15] = Function("addLinkerOption", { Field{"", "*ubyte"} }, "void", true, &program->origin_info);
}

extern "C" {

void buildscript_AdeptConfig_create(AdeptConfig* config){
    (*config) = new Config();
}
void buildscript_AdeptConfig_free(AdeptConfig* config){
    delete (*config);
}
void buildscript_AdeptConfig_defaults(AdeptConfig* config){
    Config* raw_config = *config;

    raw_config->jit = false;
    raw_config->obj = false;
    raw_config->bytecode = false;
    raw_config->link = true;
    raw_config->silent = false;
    raw_config->time = false;
    raw_config->time_verbose = false;
    raw_config->optimization = 0;
    raw_config->load_dyn = true;

    char* username = getenv("USERNAME");
    if(username == NULL) {
        fail( FAILED_TO_GET_USERNAME );
    }

    raw_config->username = username;
}

void buildscript_AdeptConfig_setTiming(AdeptConfig* config, bool whether){
    (*config)->time = whether;
}
void buildscript_AdeptConfig_setTimingVerbose(AdeptConfig* config, bool whether){
    (*config)->time_verbose = whether;
}
void buildscript_AdeptConfig_setSilent(AdeptConfig* config, bool whether){
    (*config)->silent = whether;
}
void buildscript_AdeptConfig_setJIT(AdeptConfig* config, bool whether){
    (*config)->jit = whether;
}
void buildscript_AdeptConfig_setOptimization(AdeptConfig* config, char value){
    (*config)->optimization = value;
}

bool buildscript_AdeptConfig_isTiming(AdeptConfig* config){
    return (*config)->time;
}
bool buildscript_AdeptConfig_isTimingVerbose(AdeptConfig* config){
    return (*config)->time_verbose;
}
bool buildscript_AdeptConfig_isSilent(AdeptConfig* config){
    return (*config)->silent;
}
bool buildscript_AdeptConfig_isJIT(AdeptConfig* config){
    return (*config)->jit;
}
char buildscript_AdeptConfig_getOptimization(AdeptConfig* config){
    return (*config)->optimization;
}

int buildscript_AdeptConfig_compile(AdeptConfig* config, const char* filename){
    using namespace boost::filesystem;

    Config* raw_config = *config;
    std::string source_filename = absolute(path(filename), current_path()).string();
    source_filename = string_replace_all(source_filename, "\\", "/");
    raw_config->filename = filename_name(source_filename);

    AssemblyData context;
    ErrorHandler errors(raw_config->filename);
    TokenList* tokens = new TokenList;
    Program* program = current_global_cache_manager->newProgram(source_filename);

    // Start clock
    std::cout << std::fixed;
    raw_config->time_verbose_clock.start();

    // Compiler Frontend
    if(program != NULL){
        if( tokenize(*raw_config, source_filename, tokens, errors) != 0 ) return 1;
        if( parse(*raw_config, tokens, *program, errors) != 0 ) return 1;
        free_tokens(*tokens); // Free data held by tokens
        delete tokens; // Free the token list itself
    }
    else {
        // A program with the same filename already exists
        program = current_global_cache_manager->getProgram(source_filename);
    }

    // Compiler Backend
    if( assemble(context, *raw_config, *program, errors) != 0 ) return 1;
    if( finalize(context, *raw_config, *program, errors) != 0 ) return 1;

    // Output divider if needed
    if(raw_config->time_verbose and !raw_config->silent){
        printf("-------------------------------------------------\n");
    }

    return 0;
}
bool buildscript_AdeptConfig_loadLibrary(AdeptConfig* config, const char* filename){
    std::string name = filename;
    std::string local_file = filename_path((*config)->filename) + name;
    std::string public_file = "C:/Users/" + (*config)->username + "/.adept/lib/" + name;

    if( access(local_file.c_str(), F_OK) != -1 ){
        name = local_file;
    }
    else if( access(public_file.c_str(), F_OK) != -1 ){
        name = public_file;
    }
    else if( access(filename, F_OK) == -1 ){
        // File doesn't exist globally either
        fail( UNKNOWN_MODULE(filename) );
    }

    return llvm::sys::DynamicLibrary::LoadLibraryPermanently(name.c_str());
}
void buildscript_AdeptConfig_addLinkerOption(AdeptConfig* config, const char* option){
    (*config)->extra_options += std::string(option) + " ";
}

}
