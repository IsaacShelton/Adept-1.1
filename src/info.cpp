
#include <iostream>
#include "../include/info.h"

void print_compiler_info(){
    std::cout << "Adept Compiler Version 1.2.0 - " << ADEPT_BUILD_NAME << std::endl;
    std::cout << "Copyright (c) 2016-2017 Isaac Shelton" << std::endl << std::endl;

    std::cout << "Usage: adept <filename> [options]" << std::endl << std::endl;
}

void print_help(){
    print_compiler_info();

    std::cout << "Compiler Options:" << std::endl;
    std::cout << "--jit          : Run the program just-in-time instead of compiling it" << std::endl;
    std::cout << "--obj          : Output native object files" << std::endl;
    std::cout << "--bitcode      : Output LLVM bitcode (experimental)" << std::endl;
    std::cout << "--time         : Print information about the time taken" << std::endl;
    std::cout << "--time-verbose : Print information about how much time specific tasks took" << std::endl;
    std::cout << "--nolink       : Don't link the program" << std::endl;
    std::cout << "--silent       : Don't print any messages to the terminal" << std::endl;
    std::cout << "--wait         : Wait for user input after compilation" << std::endl;
    std::cout << "--help         : Display this message" << std::endl;
}
