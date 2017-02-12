
#include <string>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

int main(int argc, char** argv) {
    if(argc < 2){
        std::cout << "Adept Compiler (c) 2016 Isaac Shelton" << std::endl;
        std::cout << "Usage: adept <filename>" << std::endl;
        return 1;
    }

    Program program;
    Configuration config;
    AssembleContext context;
    ErrorHandler errors( filename_name(argv[1]) );
    TokenList* tokens = new TokenList;

    // Compiler Frontend
    if( configure(config, argc, argv, errors)      != 0 ) exit(1);
    if( tokenize(config, argv[1], tokens, errors)  != 0 ) exit(1);
    if( parse(config, tokens, program, errors)     != 0 ) exit(1);
    free_tokens(*tokens);
    delete tokens;

    // Compiler Backend
    if( assemble(context, config, program, errors) != 0 ) exit(1);
    if( finalize(config, program, context, errors) != 0 ) exit(1);
    if( shutdown() != 0 ) exit(1);

    return 0;
}
