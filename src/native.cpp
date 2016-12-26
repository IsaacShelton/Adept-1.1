
/*
    This file is heavily based off llc.cpp in the llvm project,
    see LLVM_LICENSE for LLVM legal information
*/

#include <memory>
#include <iostream>
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/CodeGen/MIRParser/MIRParser.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "../include/asmcontext.h"

using namespace llvm;

struct ModuleBuildOptions {
    bool skip = false;
    bool no_verify = false;
    //Triple TargetTriple;
    char optimization = '2';
    bool disable_integrated_asm = false;
    bool show_mc_encoding = false;
    bool enable_dwarf_directory = false;
    bool asm_verbose = false;
    bool preserve_comments = false;
    bool disable_simplify_lib_calls = false;
    bool compile_twice = false;
    std::vector<std::string> run_pass_names;
    TargetMachine::CodeGenFileType filetype = TargetMachine::CGFT_ObjectFile;
    llvm::FloatABI::ABIType float_abi_for_calls = FloatABI::Default;
    std::string start_after;
    std::string stop_after;
};

static std::unique_ptr<tool_output_file> getOutputStream(const char*, Triple::OSType, std::string, std::string, ModuleBuildOptions& options);
static bool addPass(PassManagerBase &PM, const char *argv0, StringRef PassName, TargetPassConfig &TPC, ModuleBuildOptions& options);

int native_build_module(AssembleContext& context, std::string bitcode_filename, std::string output_filename) {
    SMDiagnostic err;
    std::unique_ptr<Module> module;
    std::unique_ptr<MIRParser> mir_parser;
    Triple triple;
    ModuleBuildOptions options;

    const char** argv = new const char*[4];
    argv[0] = "llc.exe";
    argv[1] = bitcode_filename.c_str();
    argv[2] = "-filetype=obj";
    argv[3] = ("-o" + output_filename).c_str();

    if (!options.skip) {
        if (StringRef(bitcode_filename).endswith_lower(".mir")) {
            mir_parser = createMIRParserFromFile(bitcode_filename, err, context.context);
            if (mir_parser) module = mir_parser->parseLLVMModule();
        } else module = parseIRFile(bitcode_filename, err, context.context);

        if (!module) {
            err.print(argv[0], errs());
            return 1;
        }

        // Verify module immediately to catch problems before doInitialization() is
        // called on any passes.
        if (!options.no_verify && verifyModule(*module, &errs())) {
            errs() << argv[0] << ": " << bitcode_filename << ": error: input module is broken!\n";
            return 1;
        }

        // If we are supposed to override the target triple, do so now.
        //if (!TargetTriple.empty()) module->setTargetTriple(Triple::normalize(TargetTriple));

        triple = Triple(module->getTargetTriple());
    } else {
        //triple = Triple(Triple::normalize(TargetTriple));
    }

    if (triple.getTriple().empty()) triple.setTriple(sys::getDefaultTargetTriple());

    // Get the target specific parser.
    std::string error_str;
    const Target *TheTarget = TargetRegistry::lookupTarget(MArch, triple, error_str);

    if (!TheTarget) {
        errs() << argv[0] << ": " << error_str;
        return 1;
    }

    std::string cpu = getCPUStr();
    std::string features = getFeaturesStr();

    CodeGenOpt::Level OLvl = CodeGenOpt::Default;
    switch (options.optimization) {
        default:
            errs() << argv[0] << ": invalid optimization level.\n";
            return 1;
        case ' ': break;
        case '0': OLvl = CodeGenOpt::None; break;
        case '1': OLvl = CodeGenOpt::Less; break;
        case '2': OLvl = CodeGenOpt::Default; break;
        case '3': OLvl = CodeGenOpt::Aggressive; break;
    }

    TargetOptions target_options = InitTargetOptionsFromCodeGenFlags();
    target_options.DisableIntegratedAS = options.disable_integrated_asm;
    target_options.MCOptions.ShowMCEncoding = options.show_mc_encoding;
    target_options.MCOptions.MCUseDwarfDirectory = options.enable_dwarf_directory;
    target_options.MCOptions.AsmVerbose = options.asm_verbose;
    target_options.MCOptions.PreserveAsmComments = options.preserve_comments;

    std::unique_ptr<TargetMachine> Target(TheTarget->createTargetMachine(triple.getTriple(), cpu, features, target_options, getRelocModel(), CMModel, OLvl));

    assert(Target && "Could not allocate target machine!");

    // If we don't have a module then just exit now. We do this down
    // here since the CPU/Feature help is underneath the target machine
    // creation.
    if (options.skip) return 0;

    assert(module && "Should have exited if we didn't have a module!");
    if (options.float_abi_for_calls != FloatABI::Default) target_options.FloatABIType = FloatABIForCalls;

    // Figure out where we are going to send the output.
    std::unique_ptr<tool_output_file> output = getOutputStream(TheTarget->getName(), triple.getOS(), bitcode_filename, output_filename, options);
    if (!output) return 1;

    // Build up all of the passes that we want to do to the module.
    legacy::PassManager PM;

    // Add an appropriate TargetLibraryInfo pass for the module's triple.
    TargetLibraryInfoImpl TLII(Triple(module->getTargetTriple()));

    // The -disable-simplify-libcalls flag actually disables all builtin optzns.
    if (options.disable_simplify_lib_calls) TLII.disableAllFunctions();

    PM.add(new TargetLibraryInfoWrapperPass(TLII));

    // Add the target data from the target machine, if it exists, or the module.
    module->setDataLayout(Target->createDataLayout());

    // Override function attributes based on CPUStr, FeaturesStr, and command line
    // flags.
    setFunctionAttributes(cpu, features, *module);

    if (RelaxAll.getNumOccurrences() > 0 && options.filetype != TargetMachine::CGFT_ObjectFile)
        errs() << argv[0] << ": warning: ignoring -mc-relax-all because filetype != obj";

    {
        raw_pwrite_stream *OS = &output->os();

        // Manually do the buffering rather than using buffer_ostream,
        // so we can memcmp the contents in CompileTwice mode
        SmallVector<char, 0> buffer;
        std::unique_ptr<raw_svector_ostream> buffer_stream;
        if ((options.filetype != TargetMachine::CGFT_AssemblyFile && !output->os().supportsSeeking()) || options.compile_twice) {
            buffer_stream = make_unique<raw_svector_ostream>(buffer);
            OS = buffer_stream.get();
        }

        AnalysisID start_before_id = nullptr;
        AnalysisID start_after_id = nullptr;
        AnalysisID stop_after_id = nullptr;
        const PassRegistry* pass_registry = PassRegistry::getPassRegistry();

        if (!options.run_pass_names.empty()) {
            if (!StartAfter.empty() || !StopAfter.empty()) {
                errs() << argv[0] << ": start-after and/or stop-after passes are redundant when run-pass is specified.\n";
                return 1;
            }
            if (!mir_parser) {
                errs() << argv[0] << ": run-pass needs a .mir input.\n";
                return 1;
            }
            LLVMTargetMachine &LLVMTM = static_cast<LLVMTargetMachine&>(*Target);
            TargetPassConfig &TPC = *LLVMTM.createPassConfig(PM);
            PM.add(&TPC);
            LLVMTM.addMachineModuleInfo(PM);
            LLVMTM.addMachineFunctionAnalysis(PM, mir_parser.get());
            TPC.printAndVerify("");

            for (const std::string& run_pass : options.run_pass_names) {
                if (addPass(PM, argv[0], run_pass, TPC, options)) return 1;
            }
            PM.add( createPrintMIRPass(*OS) );
        } else {
            if (!options.start_after.empty()) {
                const PassInfo *pass_info = pass_registry->getPassInfo(options.start_after);
                if (!pass_info) {
                    errs() << argv[0] << ": start-after pass is not registered.\n";
                    return 1;
                }
                start_after_id = pass_info->getTypeInfo();
            }
            if (!options.stop_after.empty()) {
                const PassInfo *pass_info = pass_registry->getPassInfo(options.stop_after);
                if (!pass_info) {
                    errs() << argv[0] << ": stop-after pass is not registered.\n";
                    return 1;
                }
                stop_after_id = pass_info->getTypeInfo();
            }

            // Ask the target to add backend passes as necessary.
            if (Target->addPassesToEmitFile(PM, *OS, options.filetype, options.no_verify,
                              start_before_id, start_after_id, stop_after_id,
                              mir_parser.get())) {
                errs() << argv[0] << ": target does not support generation of this"
                << " file type!\n";
                return 1;
            }
        }

        // Before executing passes, print the final values of the LLVM options.
        cl::PrintOptionValues();

        // If requested, run the pass manager over the same module again,
        // to catch any bugs due to persistent state in the passes. Note that
        // opt has the same functionality, so it may be worth abstracting this out
        // in the future.
        SmallVector<char, 0> compile_twice_buffer;
        if (options.compile_twice) {
            std::unique_ptr<Module> M2(llvm::CloneModule(module.get()));
            PM.run(*M2);
            compile_twice_buffer = buffer;
            buffer.clear();
        }

        PM.run(*module);

        // Compare the two outputs and make sure they're the same
        if (options.compile_twice) {
            if (buffer.size() != compile_twice_buffer.size() || (memcmp(buffer.data(), compile_twice_buffer.data(), buffer.size()) != 0)) {
                errs() << "Running the pass manager twice changed the output.\n"
                "Writing the result of the second run to the specified output\n"
                "To generate the one-run comparison binary, just run without\n"
                "the compile-twice option\n";

                output->os() << buffer;
                output->keep();
                return 1;
            }
        }

        if (buffer_stream) {
            output->os() << buffer;
        }
    }

    output->keep();
    delete argv;
    return 0;
}

static std::unique_ptr<tool_output_file> getOutputStream(const char *target_name, Triple::OSType os_type, std::string bitcode_filename,
                                                            std::string output_filename, ModuleBuildOptions& options) {
    // If we don't yet have an output filename, make one.
    if (output_filename.empty()) {
        if (bitcode_filename == "-")
            output_filename = "-";
        else {
            // If bitcode_filename ends in .bc or .ll, remove it.
            StringRef IFN = bitcode_filename;
            if (IFN.endswith(".bc") || IFN.endswith(".ll"))
                output_filename = IFN.drop_back(3);
            else if (IFN.endswith(".mir"))
                output_filename = IFN.drop_back(4);
            else
                output_filename = IFN;

            switch (options.filetype) {
            case TargetMachine::CGFT_AssemblyFile:
                if (target_name[0] == 'c') {
                    if (target_name[1] == 0)
                        output_filename += ".cbe.c";
                    else if (target_name[1] == 'p' && target_name[2] == 'p')
                        output_filename += ".cpp";
                    else
                        output_filename += ".s";
                } else
                    output_filename += ".s";
                break;
            case TargetMachine::CGFT_ObjectFile:
                if (os_type == Triple::Win32)
                    output_filename += ".obj";
                else
                    output_filename += ".o";
                break;
            case TargetMachine::CGFT_Null:
                output_filename += ".null";
                break;
            }
        }
    }

    // Decide if we need "binary" output.
    bool Binary = false;
    switch (options.filetype) {
        case TargetMachine::CGFT_AssemblyFile:
        break;
        case TargetMachine::CGFT_ObjectFile:
        case TargetMachine::CGFT_Null:
        Binary = true;
        break;
    }

    // Open the file.
    std::error_code EC;
    sys::fs::OpenFlags OpenFlags = sys::fs::F_None;
    if (!Binary)
    OpenFlags |= sys::fs::F_Text;
    auto FDOut = llvm::make_unique<tool_output_file>(output_filename, EC, OpenFlags);
    if (EC) {
        errs() << EC.message() << '\n';
        return nullptr;
    }

    return FDOut;
}

static bool addPass(PassManagerBase &PM, const char *argv0, StringRef pass_name, TargetPassConfig &TPC, ModuleBuildOptions& options) {
    if (pass_name == "none") return false;

    const PassRegistry* pass_registry = PassRegistry::getPassRegistry();
    const PassInfo* pass_info = pass_registry->getPassInfo(pass_name);
    if (!pass_info) {
        errs() << argv0 << ": run-pass " << pass_name << " is not registered.\n";
        return true;
    }

    Pass* pass;
    if (pass_info->getTargetMachineCtor())
        pass = pass_info->getTargetMachineCtor()(&TPC.getTM<TargetMachine>());
    else if (pass_info->getNormalCtor())
        pass = pass_info->getNormalCtor()();
    else {
        errs() << argv0 << ": cannot create pass: " << pass_info->getPassName() << "\n";
        return true;
    }

    std::string Banner = std::string("After ") + std::string(pass->getPassName());
    PM.add(pass);
    TPC.printAndVerify(Banner);
    return false;
}
