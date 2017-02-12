
#include "../include/mangling.h"

std::string mangle(const Program& program, const Function& func){
    if(func.name == "main") return "main";

    std::string mangled_name = func.name;
    for(const Field& arg : func.arguments){
        mangled_name += "@" + arg.type;
    }

    if(func.parent_class_offset != 0){
        mangled_name = program.classes[func.parent_class_offset-1].name + "." + mangled_name;
    }

    return mangled_name;
}

std::string mangle(const Class& klass, const Function& method){
    std::string mangled_name = klass.name + "." + method.name;
    for(const Field& arg : method.arguments){
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

std::string mangle(const std::string& class_name, const std::string& name, const std::vector<std::string>& args){
    std::string mangled_name = class_name + "." + name;
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
