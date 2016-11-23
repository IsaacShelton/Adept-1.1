
#include <iostream>
#include "../include/config.h"

int configure(Configuration& config, int argc, char** argv){
    config.clock.start();

    if(argc < 2){
        std::cout << "Adept Compiler (c) 2016 Isaac Shelton" << std::endl;
        std::cout << "Usage: adept <filename>" << std::endl;
        return 1;
    }

    std::cout << std::fixed;
    config.jit = true;
    return 0;
}
