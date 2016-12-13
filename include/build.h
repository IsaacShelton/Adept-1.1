
#ifndef BUILD_H_INCLUDED
#define BUILD_H_INCLUDED

#include "config.h"
#include "program.h"

struct BuildConfig {
    const char* sourceFilename;
    const char* programFilename;
    const char* objectFilename;
    const char* bytecodeFilename;
};

extern BuildConfig* adept_build_config;
extern Program* adept_current_program;
extern Configuration* adept_current_config;

void build_add_symbols();
void build_config_to_config(Configuration*);

// Function to get pointer to build config
extern "C" void* adept_getBuildConfig();

// Functions for compiling code
extern "C" int adept_compile(const char*);

// Functions to deal with dependencies
extern "C" void adept_dependency_add(const char*);
extern "C" void adept_dependency_addForced(const char*);
extern "C" bool adept_dependency_exists(const char*);

#endif // BUILD_H_INCLUDED
