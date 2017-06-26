
#ifndef ADEPT_H_INCLUDED
#define ADEPT_H_INCLUDED

#include <string>
#include <vector>

class AdeptCompiler {
    public:
    static std::vector<std::string> build_script_arguments;
    static void execute(int, char**);
    static void terminate();
};

#endif // ADEPT_H_INCLUDED
