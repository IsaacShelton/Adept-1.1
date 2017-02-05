
#include <cstring>
#include <iostream>
#include "../include/config.h"
#include "../include/errors.h"

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

    char* username = getenv("USERNAME");
    if(username == NULL) {
        errors.panic_plain( FAILED_TO_GET_USERNAME );
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
    config.bytecode = false;
    config.filename = argv[1];
    config.link = true;
    config.silent = false;
    config.time = false;
    config.optimization = 0;
    config.load_dyn = true;

    for(int i = 2; i != argc; i++){
        if(strcmp(argv[i], "--jit") == 0){
            config.jit = true;
        }
        else if(strcmp(argv[i], "--obj") == 0){
            config.obj = true;
        }
        else if(strcmp(argv[i], "--bc") == 0){
            config.bytecode = true;
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
        else {
            errors.panic_plain( UNKNOWN_OPTION(argv[i]) );
        }
    }

    char* username = getenv("USERNAME");
    if(username == NULL) {
        errors.panic_plain( FAILED_TO_GET_USERNAME );
    }

    config.username = username;
    std::cout << std::fixed;
    config.clock.start();
    return 0;
}
