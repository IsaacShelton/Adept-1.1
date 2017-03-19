
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "clock.h"
#include "errors.h"

struct Configuration {
    public:

    bool jit;
    bool obj;
    bool bytecode;
    bool link;
    bool load_dyn;
    bool silent;
    bool time;
    char optimization;
    bool add_build_api;

    Clock clock;
    std::string username;
    std::string filename;
    std::string extra_options;
};

int configure(Configuration& config, std::string filename, ErrorHandler& errors);
int configure(Configuration& config, int argc, char** argv, ErrorHandler& errors);

#endif // CONFIG_H_INCLUDED
