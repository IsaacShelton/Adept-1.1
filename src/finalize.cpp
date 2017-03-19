
#include "../include/jit.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/config.h"
#include "../include/strings.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

int finalize(AssemblyData& context, Configuration& config, Program& program, ErrorHandler& errors){
    if(config.time and !config.silent){
        config.clock.print_since_start("DONE", filename_name(config.filename));
        config.clock.remember();
    }

    if(config.jit){
        std::string result;

        // Run just in time
        if(jit_run(context, "main", program.imports, result) != 0) return 1;

        // Print Execution Time
        if(config.time and !config.silent){
            config.clock.print_since("EXECUTION DONE", filename_name(config.filename));
            printf("=> %s\n", result.c_str());
            config.clock.remember();
        }
    }

    return 0;
}

int shutdown(){
    llvm::llvm_shutdown();
    return 0;
}
