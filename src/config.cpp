
#include <cstring>
#include <iostream>
#include "../include/config.h"
#include "../include/errors.h"

int configure(Configuration& config, int argc, char** argv){
    if(argc < 2){
        std::cout << "Adept Compiler (c) 2016 Isaac Shelton" << std::endl;
        std::cout << "Usage: adept <filename>" << std::endl;
        return 1;
    }

    // Default Config
    config.jit = false;
    config.obj = false;
    config.bytecode = false;
    config.filename = argv[1];
    config.silent = false;
    config.time = true;

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
        else {
            die( UNKNOWN_OPTION(argv[i]) );
        }
    }

    char * username = getenv("USERNAME");
    if(username == NULL) {
        die("Failed to get username");
    }

    config.username = username;
    std::cout << std::fixed;
    config.clock.start();
    return 0;
}
