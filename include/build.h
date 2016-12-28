
#ifndef BUILD_H_INCLUDED
#define BUILD_H_INCLUDED

#include "config.h"
#include "program.h"

struct BuildConfig {
    const char* sourceFilename;
    const char* programFilename;
    const char* objectFilename;
    const char* bytecodeFilename;
    char optimization;
    bool time;
    bool silent;
    bool jit;
};

extern Program* adept_current_program;
extern Configuration* adept_current_config;
extern BuildConfig adept_default_build_config;

void build_add_symbols();
void build_transfer_config(Configuration*, BuildConfig*);

// Function to get pointer to build config
extern "C" BuildConfig adept_config();

// Functions for compiling code
extern "C" int adept_compile(const char*, BuildConfig*);

// Functions for native integration
extern "C" bool adept_loadLibrary(const char*);

// Functions to deal with dependencies
extern "C" void adept_dependency_add(const char*);
extern "C" void adept_dependency_addForced(const char*);
extern "C" bool adept_dependency_exists(const char*);

#endif // BUILD_H_INCLUDED
