
#include "../include/jit.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/config.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

int finalize(Configuration& config, AssembleContext& assemble, ErrorHandler& errors){
    if(config.time and !config.silent){
        config.clock.print_since_start("DONE", filename_name(config.filename));
        config.clock.remember();
    }

    if(config.jit){
        std::string result;

        // Run just in time
        if(jit_run(assemble, "main", result) != 0) return 1;

        // Print Execution Time
        config.clock.print_since("EXECUTION DONE", filename_name(config.filename));
        printf("=> %s\n", result.c_str());
        config.clock.remember();
    }

    return 0;
}
