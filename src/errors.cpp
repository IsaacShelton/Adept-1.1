
#include "../include/errors.h"
#include "../include/strings.h"

ErrorHandler::ErrorHandler() {
    filename = "";
    line = 1;
}

ErrorHandler::ErrorHandler(std::string file) {
    filename = file;
    line = 1;
}

void ErrorHandler::warn(std::string message){
    fail("(warning)" + filename + "(" + to_str(line) + "): " + message);
}

void ErrorHandler::panic(std::string message){
    fail(filename + "(" + to_str(line) + "): " + message);
}

void ErrorHandler::warn_plain(std::string message){
    fail("(warning)  " + message);
}

void ErrorHandler::panic_plain(std::string message){
    fail(message);
}
