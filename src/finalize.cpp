
#include "../include/die.h"
#include "../include/jit.h"
#include "../include/lexer.h"
#include "../include/parse.h"
#include "../include/config.h"
#include "../include/native.h"
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
        AssemblyData build_context;
        llvm::Function* main_function = context.module->getFunction("main");
        std::error_code error_str;
        llvm::raw_ostream* out_stream;
        ModuleBuildOptions module_build_options;
        std::string target_bc   = (config.bytecode) ? filename_change_ext(config.filename, "bc")  : "C:/Users/" + config.username + "/.adept/obj/bytecode.bc";

        if(main_function == NULL){
            // Build function does not exist
            fail_filename(config, "Can't invoke program because of missing 'main' function");
            return 1;
        }

        // Configure module_build_options using config
        module_build_options.optimization = config.optimization;

        out_stream = new llvm::raw_fd_ostream(target_bc.c_str(), error_str, llvm::sys::fs::F_None);
        llvm::WriteBitcodeToFile(context.module.get(), *out_stream);
        out_stream->flush();
        delete out_stream;

        for(size_t i = 0; i != program.dependencies.size(); i++){
            AssemblyData import_context;
            ModuleDependency* dependency = &program.dependencies[i];
            Program* dependency_program = program.parent_manager->getProgram(dependency->filename);

            dependency->config->clock.remember();
            if(assemble(import_context, *dependency->config, *dependency_program, errors) != 0) return 1;

            if(!dependency->is_nothing){
                out_stream = new llvm::raw_fd_ostream(dependency->target_bc.c_str(), error_str, llvm::sys::fs::F_None);
                llvm::WriteBitcodeToFile(import_context.module.get(), *out_stream);
                out_stream->flush();
                delete out_stream;
            }
        }

        // Run 'main()'
        if(jit_run(&config, context, "main", program.dependencies, result) != 0) return 1;

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
