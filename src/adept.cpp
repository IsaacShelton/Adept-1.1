
#include <string>
#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include "../include/adept.h"

#include "../include/info.h"
#include "../include/cache.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

void AdeptCompiler::execute(int argc, char** argv){
    // Invoke the compiler with the arguments specified

    // ASYNC: This function should be thread-safe, but it is
    //   not recommended that you call it anywhere besides 'main.cpp'

    using namespace boost::filesystem;

    if(argc < 2){
        // Print compiler version info
        std::cout << "Adept Compiler Version 1.0.0 - " << ADEPT_BUILD_NAME << std::endl;
        std::cout << "Copyright (c) 2016 Isaac Shelton" << std::endl << std::endl;

        std::cout << "Usage: adept <filename> [options]" << std::endl << std::endl;
        return 1;
    }

    // Get the full filename of the source file that we want to compile
    std::string source_filename = absolute(path(argv[1]), current_path()).string();

    Configuration config;
    AssembleContext context;
    CacheManager cache_manager;
    ErrorHandler errors( filename_name(source_filename) );
    TokenList* tokens = new TokenList;
    Program* program = cache_manager.newProgram(source_filename);

    // Configure Compiler Options
    if( configure(config, argc, argv, errors) != 0 ) exit(1);

    if(program != NULL){
        // Compiler Frontend
        if( tokenize(config, source_filename, tokens, errors) != 0 ) exit(1);
        if( parse(config, tokens, *program, errors) != 0 ) exit(1);
        free_tokens(*tokens); // Free data held by tokens
        delete tokens; // Free the token list itself
    }
    else {
        program = cache_manager.getProgram(source_filename);
    }

    // Compiler Backend
    if( assemble(context, config, *program, errors) != 0 ) exit(1);
    if( finalize(context, config, *program, errors) != 0 ) exit(1);

    // Free everything cached by our CacheManager
    cache_manager.free();
}

void AdeptCompiler::terminate(){
    // Destorys all library resources used

    if( shutdown() != 0 ) exit(1);
}
