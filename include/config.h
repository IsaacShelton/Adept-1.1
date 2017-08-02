
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "clock.h"
#include "errors.h"

struct Config {
    public:

    bool jit;
    bool obj;
    bool bytecode;
    bool link;
    bool load_dyn;
    bool silent;
    bool time;
    bool time_verbose;
    char optimization;
    bool add_build_api;
    bool wait;

    Clock time_verbose_clock;
    std::string username;
    std::string filename;
    std::string extra_options;
};

int configure(Config& config, std::string filename, ErrorHandler& errors);
int configure(Config& config, int argc, char** argv, ErrorHandler& errors);

#endif // CONFIG_H_INCLUDED
