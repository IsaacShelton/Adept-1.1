
#include <cstring>
#include <iostream>
#include "../include/adept.h"
#include "../include/config.h"
#include "../include/errors.h"
#include "../include/strings.h"

int configure(Configuration& config, std::string filename, ErrorHandler& errors){
    // Default Config
    config.jit = false;
    config.obj = false;
    config.bytecode = false;
    config.filename = filename;
    config.link = true;
    config.silent = false;
    config.time = false;
    config.optimization = 0;
    config.load_dyn = false;
    config.add_build_api = ("build.adept" == filename_name(filename));
    config.wait = false;

    char* username = getenv("USERNAME");
    if(username == NULL) {
        errors.panic_plain( FAILED_TO_GET_USERNAME );
        return 1;
    }

    config.username = username;
    std::cout << std::fixed;
    config.clock.start();
    return 0;
}

int configure(Configuration& config, int argc, char** argv, ErrorHandler& errors){
    // Default Config
    config.jit = false;
    config.obj = false;
    config.objall = false;
    config.bytecode = false;
    config.filename = argv[1];
    config.link = true;
    config.silent = false;
    config.time = false;
    config.optimization = 0;
    config.load_dyn = true;
    config.add_build_api = ("build.adept" == filename_name(argv[1]));
    config.wait = false;

    for(int i = 2; i != argc; i++){
        if(strcmp(argv[i], "--jit") == 0){
            config.jit = true;
        }
        else if(strcmp(argv[i], "--obj") == 0){
            config.obj = true;
        }
        else if(strcmp(argv[i], "--objall") == 0){
            config.objall = true;
        }
        else if(strcmp(argv[i], "--bc") == 0){
            config.bytecode = true;
        }
        else if(strcmp(argv[i], "--wait") == 0){
            config.wait = true;
        }
        else if(strcmp(argv[i], "--time") == 0){
            config.time = true;
        }
        else if(strcmp(argv[i], "--silent") == 0){
            config.silent = true;
        }
        else if(strcmp(argv[i], "--dontlink") == 0){
            config.link = false;
        }
        else if(strncmp(argv[i], "-=", 2) == 0){
            AdeptCompiler::build_script_arguments.push_back(std::string( (const char*)(argv[i]) + 2 ));
        }
        else {
            errors.panic_plain( UNKNOWN_OPTION(argv[i]) );
            return 1;
        }
    }

    if(config.objall) config.obj = true;

    char* username = getenv("USERNAME");
    if(username == NULL) {
        errors.panic_plain( FAILED_TO_GET_USERNAME );
        return 1;
    }

    config.username = username;
    std::cout << std::fixed;
    config.clock.start();
    return 0;
}
