
#include "../include/mangling.h"

std::string mangle(const Function& func){
    if(func.name == "main") return "main";

    std::string mangled_name = func.name;
    for(const Field& arg : func.arguments){
        mangled_name += "@" + arg.type;
    }
    return mangled_name;
}

std::string mangle(const std::string& name, const std::vector<std::string>& args){
    if(name == "main") return "main";

    std::string mangled_name = name;
    for(const std::string& arg : args){
        mangled_name += "@" + arg;
    }
    return mangled_name;
}

std::string mangle_filename(const std::string& filename){
    std::string mangled_name;
    char byte;

    for(size_t i = 0; i != filename.size(); i++){
        byte = filename[i];

        switch(byte){
        case ':':
            mangled_name += "$!";
            break;
        case '/':
            mangled_name += "$@";
            break;
        case '\\':
            mangled_name += "$#";
            break;
        case '$':
            mangled_name += "$$";
            break;
        case '.':
            mangled_name += "$%";
            break;
        default:
            mangled_name += byte;
        }
    }

    return mangled_name;
}
