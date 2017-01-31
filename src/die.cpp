
#include "../include/die.h"
#include "../include/strings.h"

void fail(const std::string& message){
    std::cerr << message << std::endl;
}

void fail_filename(const Configuration& config, const std::string& message){
    std::cerr << filename_name(config.filename) + ": " + message << std::endl;
}
