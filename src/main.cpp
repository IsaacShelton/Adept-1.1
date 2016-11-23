
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

int main(int argc, char** argv) {
    Program program;
    TokenList tokens;
    Configuration config;
    AssembleContext context;

    if( configure(config, argc, argv)      != 0 ) return 1;
    if( tokenize(config, argv[1], tokens)  != 0 ) return 1;
    if( parse(config, tokens, program)     != 0 ) return 1;
    if( assemble(context, config, program) != 0 ) return 1;
    if( finalize(config, context)          != 0 ) return 1;

    return 0;
}
