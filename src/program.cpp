
#include <iostream>
#include "../include/errors.h"
#include "../include/program.h"
#include "../include/assemble.h"


int Structure::find_index(std::string member_name, int* index){
    for(size_t i = 0; i != members.size(); i++){
        if(members[i].name == member_name){
            *index = i;
            return 0;
        }
    }

    return 1;
}

Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, const StatementList& statements, bool is_public){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->statements = statements;
    this->is_public = is_public;
    this->is_static = false;
    this->parent_class = NULL;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, const StatementList& statements, bool is_public, bool is_static){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->statements = statements;
    this->is_public = is_public;
    this->is_static = is_static;
    this->parent_class = NULL;
}
Function::Function(const Function& other){
    name = other.name;
    arguments = other.arguments;
    return_type = other.return_type;
    statements.resize(other.statements.size());
    is_public = other.is_public;
    is_static = other.is_static;
    parent_class = other.parent_class;
    variables = other.variables;
    asm_func = other.asm_func;

    for(size_t i = 0; i != other.statements.size(); i++){
        statements[i] = other.statements[i]->clone();
    }
}
Function::~Function(){
    for(Statement* s : statements) delete s;
}
int Function::find_variable(std::string var_name, Variable* var){
    for(size_t i = 0; i != variables.size(); i++){
        if(variables[i].name == var_name){
            if(var != NULL) *var = variables[i];
            return 0;
        }
    }

    return 1;
}
void Function::print_statements(){
    for(Statement* statement : statements){
        std::cout << statement->toString(0, false) << std::endl;
    }
}

std::string External::toString(){
    std::string prefix = (is_public) ? "public " : "private ";
    std::string arg_str;

    for(size_t a = 0; a != arguments.size(); a++){
        arg_str += arguments[a];
        if(a + 1 != arguments.size()) arg_str += ", ";
    }

    return prefix + "def " + name + "(" + arg_str + ") " + return_type;
}

ModuleDependency::ModuleDependency(std::string mod_name, std::string mod_bc, std::string mod_obj, Program* mod_program, Configuration* mod_config){
    name = mod_name;
    target_bc = mod_bc;
    target_obj = mod_obj;
    program = mod_program;
    config = mod_config;
}

Constant::Constant(){}
Constant::Constant(const std::string& n, PlainExp* v, bool p){
    name = n;
    value = v;
    is_public = p;
}

Class::Class(){}
Class::Class(const std::string& name, const std::vector<ClassField>& members, bool is_public){
    this->name = name;
    this->members = members;
    this->is_public = is_public;
}
int Class::find_index(std::string member_name, int* index){
    for(size_t i = 0; i != members.size(); i++){
        if(members[i].name == member_name){
            *index = i;
            return 0;
        }
    }

    return 1;
}

Program::~Program(){
    for(Constant& constant : constants){
        delete constant.value;
    }
}
int Program::import_merge(const Program& other, bool public_import){
    // Merge Dependencies
    for(const ModuleDependency& new_dependency : other.imports){
        bool already_exists = false;

        for(const ModuleDependency& dependency : imports){
            if(new_dependency.target_obj == dependency.target_obj){
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

            externs.push_back( External{new_func.name, arg_typenames, new_func.return_type, public_import, true} );
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
            Structure target = new_structure;
            target.is_public = public_import;
            structures.push_back(target);
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
            External target = new_external;
            target.is_public = public_import;
            target.is_mangled = new_external.is_mangled;
            externs.push_back(target);
        }
    }

    // Merge Extra Libraries
    for(const std::string& new_lib : other.extra_libs){
        bool already_exists = false;

        for(const std::string& lib : extra_libs){
            if(new_lib == lib){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            extra_libs.push_back(new_lib);
        }
    }

    // Merge Constants
    for(const Constant& new_constant : other.constants){
        if(!new_constant.is_public) continue;
        bool already_exists = false;

        for(const Constant& constant : constants){
            if(new_constant.name == constant.name){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            constants.push_back( Constant(new_constant.name, new_constant.value->clone(), public_import) );
        }
    }

    return 0;
}
int Program::generate_types(AssembleContext& context){
    // Requires types vector to be empty

    // Create from structures
    for(size_t i = 0; i != structures.size(); i++){
        llvm::StructType* llvm_struct = llvm::StructType::create(context.context, structures[i].name);
        types.push_back( Type(structures[i].name, llvm_struct) );
    }

    // Mark starting point for classes
    size_t classes_start_index = structures.size();

    // Create from classes
    for(size_t i = 0; i != classes.size(); i++){
        llvm::StructType* llvm_struct = llvm::StructType::create(context.context, classes[i].name);
        types.push_back( Type(classes[i].name, llvm_struct) );
    }

    // Standard language types
    types.push_back( Type("void", llvm::Type::getInt8Ty(context.context)) );
    types.push_back( Type("bool", llvm::Type::getInt1Ty(context.context)) );
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

    // Fill in llvm structures from structure data
    for(size_t i = 0; i != structures.size(); i++){
        Structure& struct_data = structures[i];
        std::vector<llvm::Type*> members;
        members.reserve(100);

        for(size_t j = 0; j != struct_data.members.size(); j++){
            llvm::Type* member_type;
            if(this->find_type(struct_data.members[j].type, &member_type) != 0) {
                std::cerr << "The type '" << struct_data.members[j].type << "' does not exist" << std::endl;
                return 1;
            }
            members.push_back(member_type);
        }

        static_cast<llvm::StructType*>(types[i].type)->setBody(members);
    }

    // Fill in llvm structures from class data
    for(size_t i = 0; i != classes.size(); i++){
        Class& class_data = classes[i];
        std::vector<llvm::Type*> members;
        members.reserve(100);

        for(size_t j = 0; j != class_data.members.size(); j++){
            llvm::Type* member_type;
            if(this->find_type(class_data.members[j].type, &member_type) != 0) {
                std::cerr << "The type '" << class_data.members[j].type << "' does not exist" << std::endl;
                return 1;
            }
            members.push_back(member_type);
        }

        static_cast<llvm::StructType*>(types[classes_start_index+i].type)->setBody(members);
    }

    return 0;
}
int Program::find_type(const std::string& name, llvm::Type** type){
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
int Program::find_func(const std::string& name, External* func){
    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].name == name){
            External external;
            external.name = functions[i].name;
            external.return_type = functions[i].return_type;
            external.is_public = functions[i].is_public;
            external.is_mangled = true;
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
int Program::find_func(const std::string& name, const std::vector<std::string>& args, External* func){
    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].name == name){
            bool args_match = true;
            if(args.size() != functions[i].arguments.size()) continue;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(args[a], functions[i].arguments[a].type) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            External external;
            external.name = functions[i].name;
            external.return_type = functions[i].return_type;
            external.is_public = functions[i].is_public;
            external.is_mangled = true;
            for(Field& field : functions[i].arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    for(size_t i = 0; i != externs.size(); i++){
        if(externs[i].name == name){
            bool args_match = true;
            if(args.size() != externs[i].arguments.size()) continue;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(args[a], externs[i].arguments[a]) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            *func = externs[i];
            return 0;
        }
    }

    return 1;
}
int Program::find_method(const std::string& class_name, const std::string& name, const std::vector<std::string>& args, External* func){
    Class klass;
    if(this->find_class(class_name, &klass) != 0){
        fail( UNDECLARED_CLASS(class_name) );
        return 1;
    }

    for(size_t i = 0; i != klass.methods.size(); i++){
        if(klass.methods[i].name == name){
            bool args_match = true;
            if(args.size() != klass.methods[i].arguments.size()) continue;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(args[a], klass.methods[i].arguments[a].type) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            External external;
            external.name = klass.methods[i].name;
            external.return_type = klass.methods[i].return_type;
            external.is_public = klass.methods[i].is_public;
            external.is_mangled = true;
            for(Field& field : klass.methods[i].arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    return 1;
}
int Program::find_struct(const std::string& name, Structure* structure){
    for(size_t i = 0; i != structures.size(); i++){
        if(structures[i].name == name){
            *structure = structures[i];
            return 0;
        }
    }

    return 1;
}
int Program::find_class(const std::string& name, Class* klass){
    for(size_t i = 0; i != classes.size(); i++){
        if(classes[i].name == name){
            *klass = classes[i];
            return 0;
        }
    }

    return 1;
}
int Program::find_const(const std::string& name, Constant* constant){
    for(size_t i = 0; i != constants.size(); i++){
        if(constants[i].name == name){
            *constant = constants[i];
            return 0;
        }
    }

    return 1;
}
void Program::print(){
    std::cout << std::endl;
    print_externals();
    print_structures();
    print_classes();
    print_functions();
    std::cout << std::endl;
}
void Program::print_functions(){
    for(Function& f : functions){
        std::cout << (f.is_public ? "public " : "private ");
        std::cout << "def " << f.name << "(";
        for(size_t a = 0; a != f.arguments.size(); a++){
            std::cout << f.arguments[a].name << " " << f.arguments[a].type;
            if(a + 1 != f.arguments.size()) std::cout << ", ";
        }
        std::cout << ") " << f.return_type << " {" << std::endl;
        for(size_t a = 0; a != f.statements.size(); a++){
            std::cout << f.statements[a]->toString(1, false) << std::endl;
        }
        std::cout << "}" << std::endl;
    }
}
void Program::print_externals(){
    for(External& e : externs){
        std::cout << (e.is_public ? "public " : "private ");
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
        std::cout << (s.is_public ? "public " : "private ");
        std::cout << "type " << s.name << " {" << std::endl;

        for(Field f : s.members){
            std::cout << "    " << f.name << " " << f.type << std::endl;
        }

        std::cout << "}" << std::endl;
    }
}
void Program::print_classes(){
    for(Class& klass : classes){
        std::cout << (klass.is_public ? "public " : "private ");
        std::cout << "class " << klass.name << " {" << std::endl;

        for(ClassField& f : klass.members){
            std::cout << "    ";
            std::cout << (f.is_public ? "public " : "private ");
            if(f.is_static) std::cout << "static ";
            std::cout << f.name << " " << f.type << std::endl;
        }

        for(Function& m : klass.methods){
            std::cout << "    ";
            std::cout << (m.is_public ? "public " : "private ");
            if(m.is_static) std::cout << "static ";
            std::cout << "def " << m.name << "(";
            for(size_t a = 0; a != m.arguments.size(); a++){
                std::cout << m.arguments[a].name << " " << m.arguments[a].type;
                if(a + 1 != m.arguments.size()) std::cout << ", ";
            }
            std::cout << ") " << m.return_type << " {" << std::endl;
            for(size_t a = 0; a != m.statements.size(); a++){
                std::cout << m.statements[a]->toString(2, false) << std::endl;
            }
            std::cout << "    }" << std::endl;
        }

        std::cout << "}" << std::endl;
    }
}
