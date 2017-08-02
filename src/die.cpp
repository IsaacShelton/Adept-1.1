
#include "../include/die.h"
#include "../include/strings.h"

#ifdef USE_ENSURES
void _ensure(const char* expression, const char* filename, int line){
    std::cerr << "\n[FATAL ERROR] " << filename_name(filename) << "(" << to_str(line) << "): ensure(" << expression << ") failed!" << std::endl;
    std::abort();
}
#endif // USE_ENSURES

void fail(const std::string& message){
    std::cerr << message << std::endl;
}

void fail_filename(const Config& config, const std::string& message){
    std::cerr << filename_name(config.filename) + ": " + message << std::endl;
}

void fail_filename(const std::string& filename, const std::string& message){
    std::cerr << filename_name(filename) + ": " + message << std::endl;
}
