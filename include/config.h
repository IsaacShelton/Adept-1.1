
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "clock.h"

struct Configuration {
    Clock clock;
    bool jit;
};

int configure(Configuration& config, int argc, char** argv);

#endif // CONFIG_H_INCLUDED
