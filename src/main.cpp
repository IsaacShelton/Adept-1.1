
#include <string>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "../include/adept.h"
#include <boost/filesystem.hpp>

int main(int argc, char** argv) {
    AdeptCompiler::execute(argc, argv);
    AdeptCompiler::terminate();
    return 0;
}
