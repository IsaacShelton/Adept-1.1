
#ifndef NATIVE_H_INCLUDED
#define NATIVE_H_INCLUDED

#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetSubtargetInfo.h"

#include "../include/asmdata.h"

struct ModuleBuildOptions {
    bool skip = false;
    bool no_verify = false;
    //Triple TargetTriple;
    char optimization = 0;
    bool disable_integrated_asm = false;
    bool show_mc_encoding = false;
    bool enable_dwarf_directory = false;
    bool asm_verbose = false;
    bool preserve_comments = false;
    bool disable_simplify_lib_calls = false;
    bool compile_twice = false;
    std::vector<std::string> run_pass_names;
    llvm::TargetMachine::CodeGenFileType filetype = llvm::TargetMachine::CGFT_ObjectFile;
    llvm::FloatABI::ABIType float_abi_for_calls = llvm::FloatABI::Default;
    std::string start_after;
    std::string stop_after;
};

int native_build_module(AssemblyData& context, std::string bitcode_filename, std::string output_filename, ModuleBuildOptions& options);
bool nativeAddPass(llvm::legacy::PassManagerBase &PM, const char *argv0, llvm::StringRef pass_name, llvm::TargetPassConfig &TPC, ModuleBuildOptions& options);

#endif // NATIVE_H_INCLUDED
