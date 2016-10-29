
#include <chrono>
#include <string>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include "../include/lexer.h"

int main(int argc, char** argv) {
    auto start = std::chrono::steady_clock::now(); // Start Timer

    int result;
    std::vector<Token> tokens;

    if(argc != 2){
        std::cerr << "Adept Copyright (c) 2016 Isaac Shelton" << std::endl;
        std::cerr << "Usage: adept <filename>" << std::endl;
        return 1;
    }

    result = tokenize(argv[1], tokens);
    if(result != 0) return 1;

    // Get Total Time
    auto finish = std::chrono::steady_clock::now();
    double seconds = (std::chrono::duration<double, std::milli> (finish - start).count())/1000;
    std::cout << "lexed in " << seconds << "s" << std::endl;

    //for(Token token : tokens) std::cout << token.getString() << " " << std::endl;

    return 0;
}
