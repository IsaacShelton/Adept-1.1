
// (c) 2017 Isaac Shelton
// API used by adept build scripts

#ifndef BUILD_H_INCLUDED
#define BUILD_H_INCLUDED

#include "config.h"
#include "program.h"

// So build scripts can access program cache
extern CacheManager* current_global_cache_manager;

// Underlying implementation of AdeptConfig is just a pointer
typedef Configuration* AdeptConfig;

// Register functions that build scripts can find
void build_add_symbols();
void build_add_api(Program*);

extern "C" {

void buildscript_AdeptConfig_create(AdeptConfig*);
void buildscript_AdeptConfig_free(AdeptConfig*);
void buildscript_AdeptConfig_defaults(AdeptConfig*);

void buildscript_AdeptConfig_setTiming(AdeptConfig*, bool);
void buildscript_AdeptConfig_setTimingVerbose(AdeptConfig*, bool);
void buildscript_AdeptConfig_setSilent(AdeptConfig*, bool);
void buildscript_AdeptConfig_setJIT(AdeptConfig*, bool);
void buildscript_AdeptConfig_setOptimization(AdeptConfig*, char);

bool buildscript_AdeptConfig_isTiming(AdeptConfig*);
bool buildscript_AdeptConfig_isTimingVerbose(AdeptConfig*);
bool buildscript_AdeptConfig_isSilent(AdeptConfig*);
bool buildscript_AdeptConfig_isJIT(AdeptConfig*);
char buildscript_AdeptConfig_getOptimization(AdeptConfig*);

int buildscript_AdeptConfig_compile(AdeptConfig*, const char*);
bool buildscript_AdeptConfig_loadLibrary(AdeptConfig*, const char*);
void buildscript_AdeptConfig_addLinkerOption(AdeptConfig*, const char*);

}

#endif // BUILD_H_INCLUDED
