
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "clock.h"

struct Configuration {
    std::string username;
    std::string filename;
    Clock clock;
    bool jit;
    bool obj;
    bool bytecode;
    bool link;

    bool silent;
    bool time;
};

int configure(Configuration& config, std::string filename);
int configure(Configuration& config, int argc, char** argv);

#endif // CONFIG_H_INCLUDED
