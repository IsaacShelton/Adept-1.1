
#include <string>
#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include "../include/adept.h"

#include "../include/info.h"
#include "../include/cache.h"
#include "../include/clock.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

std::string AdeptCompiler::username;
std::vector<std::string> AdeptCompiler::build_script_arguments;

void AdeptCompiler::execute(int argc, char** argv){
    // Invoke the compiler with the arguments specified

    // ASYNC: This function should be thread-safe, but it is
    //   not recommended that you call it anywhere besides 'main.cpp'

    using namespace boost::filesystem;

    Clock clock;
    double lexer_time = 0.0;
    double parser_time = 0.0;
    double assembler_time = 0.0;
    double complete_time = 0.0;
    bool arguments_altered = false;

    if(argc == 1 and exists("build.adept")){
        // Alter arguements in order to run the build script
        const char** new_argv = new char*[2];
        arguments_altered = true;
        new_argv[0] = argv[0];
        new_argv[1] = "build.adept";
        argv = new_argv;
        argc = 2;
    }

    if(argc < 2){
        // Print compiler version info
        print_compiler_info();
        if(arguments_altered) delete argv;
        return 1;
    }

    if(strcmp(argv[1], "--help") == 0 or strcmp(argv[1], "-help") == 0){
        print_help();
        return 1;
    }

    clock.start();

    // Get the full filename of the source file that we want to compile
    std::string source_filename = absolute(path(argv[1]), current_path()).string();
    source_filename = string_replace_all(source_filename, "\\", "/");

    Configuration config;
    AssemblyData context;
    CacheManager cache_manager;
    ErrorHandler errors( filename_name(source_filename) );
    TokenList* tokens = new TokenList;
    Program* program = cache_manager.newProgram(source_filename);

    // Configure Compiler Options
    if( configure(config, argc, argv, errors) != 0 ){
        if(arguments_altered) delete argv;
        cache_manager.free();
        delete tokens;

        if(config.wait){
            std::cout << "[ERROR] Press enter to continue...";
            std::cin.get();
        }
        exit(1);
    }

    username = config.username;
    if(config.time) clock.remember();

    if(config.time){
        if(program != NULL){
            // A new program was created successfully

            // Compiler Frontend
            if( tokenize(config, source_filename, tokens, errors) != 0 ) exit(1);
            lexer_time = clock.since();
            clock.remember();

            if( parse(config, tokens, *program, errors) != 0 ) exit(1);
            free_tokens(*tokens); // Free data held by tokens
            delete tokens; // Free the token list itself
            parser_time = clock.since();
            clock.remember();
        }
        else {
            // A program with the same filename already exists
            program = cache_manager.getProgram(source_filename);
        }
    }
    else {
        if(program != NULL){
            // A new program was created successfully

            // Compiler Frontend
            if( tokenize(config, source_filename, tokens, errors) != 0 ) exit(1);
            if( parse(config, tokens, *program, errors) != 0 ) exit(1);
            free_tokens(*tokens); // Free data held by tokens
            delete tokens; // Free the token list itself
        }
        else {
            // A program with the same filename already exists
            program = cache_manager.getProgram(source_filename);
        }
    }

    // Compiler Backend
    if( assemble(context, config, *program, errors) != 0 ){
        if(arguments_altered) delete argv;
        cache_manager.free();

        if(config.wait){
            std::cout << "[ERROR] Press enter to continue...";
            std::cin.get();
        }
        exit(1);
    }

    if(config.time){
        assembler_time = clock.since();
        clock.remember();
    }

    if( finalize(context, config, *program, errors) != 0 ){
        if(arguments_altered) delete argv;
        cache_manager.free();

        if(config.wait){
            std::cout << "[ERROR] Press enter to continue...";
            std::cin.get();
        }
        exit(1);
    }

    if(config.time){
        complete_time = clock.since_start();
        std::cout << "Successfully compiled '" << filename_name(source_filename) << "' in " << complete_time << " seconds" << std::endl;
        std::cout << "    Lexer     : " << lexer_time << " seconds" << std::endl;
        std::cout << "    Parser    : " << parser_time << " seconds" << std::endl;
        std::cout << "    Assembler : " << assembler_time << " seconds" << std::endl;
    }

    // If altered, free argv
    if(arguments_altered) delete argv;

    // Free everything cached by our CacheManager
    cache_manager.free();

    if(config.wait){
        std::cout << "[SUCCESS] Press enter to continue...";
        std::cin.get();
    }
}

void AdeptCompiler::terminate(){
    // Destorys all library resources used
    if( shutdown() != 0 ) exit(1);

    // Attempt to delete all tmp files used
    try {
        boost::filesystem::remove_all(boost::filesystem::path("C:/Users/" + username + "/.adept/obj/tmp"));
    } catch (...) {}
}
