
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
    Program program;
    Configuration config;
    AssembleContext context;
    ErrorHandler errors( filename_name(argv[1]) );
    TokenList* tokens;

    tokens = new TokenList;
    if( configure(config, argc, argv, errors)      != 0 ) return 1;
    if( tokenize(config, argv[1], tokens, errors)  != 0 ) return 1;
    if( parse(config, tokens, program, errors)     != 0 ) return 1;
    if( assemble(context, config, program, errors) != 0 ) return 1;
    if( finalize(config, context, errors)          != 0 ) return 1;

    llvm::llvm_shutdown();
    return 0;
}
