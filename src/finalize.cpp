
#include "../include/jit.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/config.h"
#include "../include/assemble.h"
#include "../include/finalize.h"

int finalize(Configuration& config, AssembleContext& assemble){
    printf("---------------------------------------\n");
    config.clock.print_since_start("Total Time");
    config.clock.remember();

    if(config.jit){
        std::string result;

        // Run just in time
        jit_init();
        if(jit_main(assemble, result) != 0) return 1;

        // Print Execution Time
        config.clock.print_since("Execution Finished");
        printf("=> %s\n", result.c_str());
        config.clock.remember();
    }

    llvm::llvm_shutdown();
    return 0;
}
