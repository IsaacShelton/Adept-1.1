
#include <iostream>
#include "../include/errors.h"
#include "../include/program.h"

ModuleDependency::ModuleDependency(const ModuleDependency& other){
    name = other.name;
    target_bc = other.target_bc;
    target_obj = other.target_obj;
    program = new Program(*other.program);
    config = new Configuration(*other.config);
}
ModuleDependency::ModuleDependency(std::string mod_name, std::string mod_bc, std::string mod_obj, Program* mod_program, Configuration* mod_config){
    name = mod_name;
    target_bc = mod_bc;
    target_obj = mod_obj;
    program = mod_program;
    config = mod_config;
}

int Program::import_merge(const Program& other){
    // Merge Dependencies
    for(const ModuleDependency& new_dependency : other.imports){
        bool already_exists = false;

        for(const ModuleDependency& dependency : imports){
            if(new_dependency.name == dependency.name){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            imports.push_back(new_dependency);
        }
    }

    // Merge Functions
    for(const Function& new_func : other.functions){
        // If the function is private, skip over it
        if(!new_func.is_public) continue;
        bool already_exists = false;

        for(const Function& func : functions){
            if(new_func.name == func.name){
                if(func.is_public){
                    die(DUPLICATE_FUNC(new_func.name));
                }

                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            std::vector<std::string> arg_typenames(new_func.arguments.size());

            for(size_t i = 0; i != new_func.arguments.size(); i++){
                arg_typenames[i] = new_func.arguments[i].type;
            }

            externs.push_back( External{new_func.name, arg_typenames, new_func.return_type, false} );
        }
    }

    // Merge Structures
    for(const Structure& new_structure : other.structures){
        // If the structure is private, skip over it
        if(!new_structure.is_public) continue;
        bool already_exists = false;

        for(const Structure& structure : structures){
            if(new_structure.name == structure.name){
                if(structure.is_public){
                    die(DUPLICATE_STRUCT(new_structure.name));
                }

                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            structures.push_back(new_structure);
        }
    }

    // Merge Externals
    for(const External& new_external : other.externs){
        // If the external is private, skip over it
        if(!new_external.is_public) continue;
        bool already_exists = false;

        for(const External& external : externs){
            if(new_external.name == external.name){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            externs.push_back(new_external);
        }
    }

    // Merge LLVM Types
    for(const Type& new_type : other.types){
        bool already_exists = false;

        for(const Type& type : types){
            if(new_type.name == type.name){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            types.push_back(new_type);
        }
    }

    return 0;
}
int Program::generate_types(AssembleContext& context){
    // Requires types vector to be empty

    for(size_t i = 0; i != structures.size(); i++){
        llvm::StructType* llvm_struct = llvm::StructType::create(context.context, structures[i].name);
        types.push_back( Type(structures[i].name, llvm_struct) );
    }

    types.push_back( Type("void", llvm::Type::getInt8Ty(context.context)) );
    types.push_back( Type("ptr", llvm::Type::getInt8PtrTy(context.context)) );
    types.push_back( Type("int", llvm::Type::getInt32Ty(context.context)) );
    types.push_back( Type("uint", llvm::Type::getInt32Ty(context.context)) );
    types.push_back( Type("double", llvm::Type::getDoubleTy(context.context)) );
    types.push_back( Type("float", llvm::Type::getFloatTy(context.context)) );
    types.push_back( Type("byte", llvm::Type::getInt8Ty(context.context)) );
    types.push_back( Type("ubyte", llvm::Type::getInt8Ty(context.context)) );
    types.push_back( Type("short", llvm::Type::getInt16Ty(context.context)) );
    types.push_back( Type("ushort", llvm::Type::getInt16Ty(context.context)) );
    types.push_back( Type("long", llvm::Type::getInt64Ty(context.context)) );
    types.push_back( Type("ulong", llvm::Type::getInt64Ty(context.context)) );

    for(size_t i = 0; i != structures.size(); i++){
        Structure& struct_data = structures[i];
        std::vector<llvm::Type*> members;
        members.reserve(100);

        for(size_t j = 0; j != struct_data.members.size(); j++){
            llvm::Type* member_type;
            if(this->find_type(struct_data.members[j].type, &member_type) != 0) {
                std::cout << "The type '" << struct_data.members[j].type << "' does not exist" << std::endl;
                return 1;
            }
            members.push_back(member_type);
        }

        static_cast<llvm::StructType*>(types[i].type)->setBody(members);
    }

    return 0;
}
int Program::find_type(std::string name, llvm::Type** type){
    size_t pointers;
    std::string type_name;

    for(pointers = 0; pointers != name.length(); pointers++){
        if(name[pointers] != '*') break;
    }

    type_name = name.substr(pointers, name.length()-pointers);
    for(size_t i = 0; i != types.size(); i++){
        if(types[i].name == type_name){
            *type = types[i].type;
            for(size_t i = 0; i != pointers; i++) *type = (*type)->getPointerTo();
            return 0;
        }
    }

    return 1;
}
int Program::find_func(std::string name, External* func){
    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].name == name){
            External external;
            external.name = functions[i].name;
            external.return_type = functions[i].return_type;
            for(Field& field : functions[i].arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    for(size_t i = 0; i != externs.size(); i++){
        if(externs[i].name == name){
            *func = externs[i];
            return 0;
        }
    }

    return 1;
}
int Program::find_struct(std::string name, Structure* structure){
    for(size_t i = 0; i != structures.size(); i++){
        if(structures[i].name == name){
            *structure = structures[i];
            return 0;
        }
    }

    return 1;
}
void Program::print(){
    std::cout << std::endl;
    print_externals();
    print_structures();
    print_functions();
    std::cout << std::endl;
}
void Program::print_functions(){
    for(Function& f : functions){
        std::cout << "def " << f.name << "(";
        for(size_t a = 0; a != f.arguments.size(); a++){
            std::cout << f.arguments[a].name << " " << f.arguments[a].type;
            if(a + 1 != f.arguments.size()) std::cout << ", ";
        }
        std::cout << ") " << f.return_type << " {" << std::endl;
        for(size_t a = 0; a != f.statements.size(); a++){
            std::cout << "    " << f.statements[a].toString() << std::endl;
        }
        std::cout << "}" << std::endl;
    }
}
void Program::print_externals(){
    for(External& e : externs){
        std::cout << "foreign " << e.name << "(";
        for(size_t a = 0; a != e.arguments.size(); a++){
            std::cout << e.arguments[a];
            if(a + 1 != e.arguments.size()) std::cout << ", ";
        }
        std::cout << ") " << e.return_type << std::endl;
    }
}
void Program::print_structures(){
    for(Structure& s : structures){
        std::cout << "type " << s.name << " {" << std::endl;

        for(Field f : s.members){
            std::cout << "    " << f.name << " " << f.type << std::endl;
        }

        std::cout << "}" << std::endl;
    }
}

int Function::find_variable(std::string var_name, Variable* var){
    for(size_t i = 0; i != variables.size(); i++){
        if(variables[i].name == var_name){
            *var = variables[i];
            return 0;
        }
    }

    return 1;
}
void Function::print_statements(){
    for(Statement statement : statements){
        std::cout << statement.toString() << std::endl;
    }
}

int Structure::find_index(std::string member_name, int* index){
    for(size_t i = 0; i != members.size(); i++){
        if(members[i].name == member_name){
            *index = i;
            return 0;
        }
    }

    return 1;
}
