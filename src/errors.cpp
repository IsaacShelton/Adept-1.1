
#include "../include/die.h"
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
    fail(filename + "(" + to_str(line) + ") WARNING: " + message);
}

void ErrorHandler::panic(std::string message){
    fail(filename + "(" + to_str(line) + "): " + message);
}

void ErrorHandler::warn_plain(std::string message){
    fail("WARNING:  " + message);
}

void ErrorHandler::panic_plain(std::string message){
    fail(message);
}

std::string ErrorHandler::format(const std::string& message){
    return filename + "(" + to_str(line) + "): " + message;
}

void ErrorHandler::panic_undeclared_func(const std::string& name, const std::vector<std::string>& args){
    std::string message = "Undefined function '" + name + "(";
    for(size_t i = 0; i != args.size(); i++){
        message += args[i];
        if(i + 1 != args.size()) message += ", ";
    }
    message += ")'";

    fail(filename + "(" + to_str(line) + "): " + message);
}

void ErrorHandler::panic_undeclared_method(const std::string& class_name, const std::string& name, const std::vector<std::string>& args){
    std::string message = "Undefined method '" + class_name + "." + name + "(";
    for(size_t i = 0; i != args.size(); i++){
        message += args[i];
        if(i + 1 != args.size()) message += ", ";
    }
    message += ")'";

    fail(filename + "(" + to_str(line) + "): " + message);
}
