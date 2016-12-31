
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "clock.h"
#include "errors.h"

struct Configuration {
    std::string username;
    std::string filename;
    Clock clock;
    bool jit;
    bool obj;
    bool bytecode;
    bool link;
    bool load_dyn;

    bool silent;
    bool time;
    char optimization;
};

int configure(Configuration& config, std::string filename, ErrorHandler& errors);
int configure(Configuration& config, int argc, char** argv, ErrorHandler& errors);

#endif // CONFIG_H_INCLUDED
