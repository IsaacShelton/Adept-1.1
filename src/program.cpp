
#include <iostream>
#include <algorithm>
#include "../include/die.h"
#include "../include/errors.h"
#include "../include/program.h"
#include "../include/assemble.h"
#include "../include/mangling.h"

int Structure::find_index(std::string member_name, int* index){
    for(size_t i = 0; i != members.size(); i++){
        if(members[i].name == member_name){
            *index = i;
            return 0;
        }
    }

    return 1;
}

Function::Function(){
    this->is_public = false;
    this->is_static = false;
    this->is_stdcall = false;
    this->parent_class_offset = 0;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, const StatementList& statements, bool is_public){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->statements = statements;
    this->is_public = is_public;
    this->is_static = false;
    this->is_stdcall = false;
    this->parent_class_offset = 0;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, const StatementList& statements, bool is_public,
                   bool is_static, bool is_stdcall){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->statements = statements;
    this->is_public = is_public;
    this->is_static = is_static;
    this->is_stdcall = is_stdcall;
    this->parent_class_offset = 0;
}
Function::Function(const Function& other){
    name = other.name;
    arguments = other.arguments;
    return_type = other.return_type;
    statements.resize(other.statements.size());
    is_public = other.is_public;
    is_static = other.is_static;
    is_stdcall = other.is_stdcall;
    parent_class_offset = other.parent_class_offset;
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

External::External(){}
External::External(const std::string& name, const std::vector<std::string>& arguments, const std::string& return_type, bool is_public, bool is_mangled, bool is_stdcall){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->is_public = is_public;
    this->is_mangled = is_mangled;
    this->is_stdcall = is_stdcall;
}
std::string External::toString(){
    std::string prefix = std::string(is_public ? "public " : "private ");
    std::string arg_str;

    if(this->is_stdcall) prefix += "stdcall ";

    for(size_t a = 0; a != arguments.size(); a++){
        arg_str += arguments[a];
        if(a + 1 != arguments.size()) arg_str += ", ";
    }

    return prefix + "def " + name + "(" + arg_str + ") " + return_type;
}

ModuleDependency::ModuleDependency(const std::string& filename, const std::string& target_bc, const std::string& target_obj, Configuration* config){
    this->filename = filename;
    this->target_bc = target_bc;
    this->target_obj = target_obj;
    this->config = config;
}

Constant::Constant(){}
Constant::Constant(const std::string& n, PlainExp* v, bool p){
    name = n;
    value = v;
    is_public = p;
}

Global::Global(){}
Global::Global(const std::string& name, const std::string& type, bool is_public, ErrorHandler& errors){
    this->name = name;
    this->type = type;
    this->is_public = is_public;
    this->is_imported = false;
    this->errors = errors;
}

Class::Class(){
    this->is_public = false;
    this->is_imported = false;
}
Class::Class(const std::string& name, const std::vector<ClassField>& members, bool is_public){
    this->name = name;
    this->members = members;
    this->is_public = is_public;
    this->is_imported = false;
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

TypeAlias::TypeAlias(){}
TypeAlias::TypeAlias(const std::string& alias, const std::string& binding, bool is_public){
    this->alias = alias;
    this->binding = binding;
    this->is_public = is_public;
}

bool Program::is_function_typename(const std::string& type_name){
    // "def(int, int) ptr" -> true
    // "**int" -> false

    // Function Pointer Type Layout ( [] = optional ):
    // "[stdcall] def(type, type, type) type"

    if(type_name.length() < 6) return false;
    if(type_name.substr(0, 4) == "def("){
        return true;
    }

    if(type_name.length() < 8) return false;
    if(type_name.substr(0, 8) == "stdcall "){
        return true;
    }

    return false;
}
bool Program::is_pointer_typename(const std::string& type_name){
    // "**uint" -> true
    // "long"   -> false

    if(type_name.length() < 1) return false;
    if(type_name[0] == '*') return true;
    return false;
}
bool Program::is_array_typename(const std::string& type_name){
    // "[]int"  -> true
    // "**uint" -> false

    if(type_name.length() < 2) return false;
    if(type_name[0] == '[' and type_name[1] == ']') return true;
    return false;
}
bool Program::is_integer_typename(const std::string& type_name){
    if(type_name == "int")    return true;
    if(type_name == "uint")   return true;
    if(type_name == "long")   return true;
    if(type_name == "ulong")  return true;
    if(type_name == "short")  return true;
    if(type_name == "ushort") return true;
    if(type_name == "byte")   return true;
    if(type_name == "ubyte")  return true;

    return false;
}
bool Program::function_typename_is_stdcall(const std::string& type_name){
    // Function Pointer Type Layout ( [] = optional ):
    // "[stdcall] def(type, type, type) type"

    if(type_name.length() < 8) return false;
    if(type_name.substr(0, 8) == "stdcall "){
        return true;
    }

    return false;
}

Program::Program(CacheManager* parent_manager){
    this->parent_manager = parent_manager;
}
Program::~Program(){
    for(Constant& constant : constants){
        delete constant.value;
    }
}
int Program::import_merge(Program& other, bool public_import){
    // TODO: Clean up importing system, currently there are holes
    //   and edge cases that do not work. Importing things like
    //   functions that have the same names or have the same name as
    //   an external most likely will do something weird.

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
        std::string new_final_name = mangle(other, new_func);

        for(const Function& func : functions){
            if(new_final_name == mangle(other, func)){
                die(DUPLICATE_FUNC(new_func.name));
                already_exists = true;
                break;
            }
        }

        for(const External& external : externs){
            if( new_final_name == (external.is_mangled ? mangle(external.name, external.arguments) : external.name) ){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            std::vector<std::string> arg_typenames(new_func.arguments.size());

            for(size_t i = 0; i != new_func.arguments.size(); i++){
                arg_typenames[i] = new_func.arguments[i].type;
            }

            externs.push_back( External(new_func.name, arg_typenames, new_func.return_type, public_import, true, new_func.is_stdcall) );
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
            structures.push_back( std::move(target) );
        }
    }

    // Merge Classes
    for(const Class& new_class : other.classes){
        // If the class is private, skip over it
        if(!new_class.is_public) continue;
        bool already_exists = false;

        for(const Class& klass : classes){
            if(new_class.name == klass.name){
                if(klass.is_public){
                    die( DUPLICATE_CLASS(new_class.name) );
                }

                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            classes.resize(classes.size() + 1);
            Class* target = &classes[classes.size()-1];

            *target = new_class;
            target->is_public = public_import;
            target->is_imported = true;

            for(size_t i = 0; i != target->methods.size(); i++){
                target->methods[i].parent_class_offset = classes.size();
            }
        }
    }

    // Merge Externals
    for(const External& new_external : other.externs){
        // If the external is private, skip over it
        if(!new_external.is_public) continue;
        bool already_exists = false;
        std::string new_external_final_name = (new_external.is_mangled) ? mangle(new_external.name, new_external.arguments) : new_external.name;

        for(const External& external : externs){
            if( new_external_final_name == (external.is_mangled ? mangle(external.name, external.arguments) : external.name) ){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            External target = new_external;
            target.is_public = public_import;
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

    // Merge Globals
    for(Global& new_global : other.globals){
        if(!new_global.is_public) continue;
        bool already_exists = false;

        for(const Global& global : globals){
            if(new_global.name == global.name){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            Global cloned_global = new_global;

            cloned_global.is_imported = true;
            globals.push_back(std::move(cloned_global));
        }
    }

    // Merge Type Aliases
    for(TypeAlias& new_type_alias : other.type_aliases){
        if(!new_type_alias.is_public) continue;
        bool already_exists = false;

        for(const TypeAlias& type_alias : type_aliases){
            if(new_type_alias.alias == type_alias.alias){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            type_aliases.push_back(new_type_alias);
        }
    }

    return 0;
}

bool Program::resolve_if_alias(std::string& type) const {
    // Returns true if completed a substitution

    size_t pointer_count = 0;
    for(size_t i = 0; i != type.size(); i++){
        if(type[i] != '*') break;
        pointer_count++;
    }

    type = type.substr(pointer_count, type.size()-pointer_count);

    for(const TypeAlias& type_alias : type_aliases){
        if(type == type_alias.alias){
            type = type_alias.binding;

            bool alias_has_alias = this->resolve_once_if_alias(type);
            std::vector<std::string> visited_aliases;

            while(alias_has_alias){
                // Resolve until there is nothing left to resolve
                bool already_visited = (std::find(visited_aliases.begin(), visited_aliases.end(), type) != visited_aliases.end());

                if(type == type_alias.binding or already_visited) {
                    fail("Encountered recursive type alias '" + type + "'");
                    for(size_t p = 0; p != pointer_count; p++) type = "*" + type;
                    return false;
                }
                alias_has_alias = this->resolve_once_if_alias(type);
                visited_aliases.push_back(type);
            }

            for(size_t p = 0; p != pointer_count; p++) type = "*" + type;
            return true; // We found an alias so don't continue searching
        }
    }

    for(size_t p = 0; p != pointer_count; p++) type = "*" + type;
    return false;
}
bool Program::resolve_once_if_alias(std::string& type) const {
    // Returns true if completed a substitution

    size_t pointer_count = 0;
    for(size_t i = 0; i != type.size(); i++){
        if(type[i] != '*') break;
        pointer_count++;
    }

    type = type.substr(pointer_count, type.length()-pointer_count);

    for(const TypeAlias& type_alias : type_aliases){
        if(type == type_alias.alias){
            type = type_alias.binding;

            for(size_t p = 0; p != pointer_count; p++) type = type = "*" + type;
            return true; // We found an alias so don't continue searching
        }
    }

    for(size_t p = 0; p != pointer_count; p++) type = type = "*" + type;
    return false;
}

int Program::extract_function_pointer_info(const std::string& type_name, std::vector<llvm::Type*>& llvm_args, llvm::Type** return_llvm_type) const {
    // "def(int, int) int"
    // -----^

    // NOTE: type_name is assumed to be a valid function pointer type

    size_t c = 0;
    if(type_name.length() > 8){
        if(type_name.substr(0, 8) == "stdcall "){
            c = 8;
        }
    }
    c += 4;

    while(type_name[c] != ')'){
        llvm::Type* arg_llvm_type;
        std::string arg_type_string;
        int balance = 0;

        while((type_name[c] != ')' and type_name[c] != ',') or balance != 0){
            if(type_name[c] == '(') balance += 1;
            if(type_name[c] == ')') balance -= 1;
            arg_type_string += type_name[c];
            next_index(c, type_name.length());
        }
        if(type_name[c] == ','){
            next_index(c, type_name.length());
            next_index(c, type_name.length());
        }
        if(this->find_type(arg_type_string, &arg_llvm_type) != 0){
            fail(UNDECLARED_TYPE(arg_type_string));
            return 1;
        }

        llvm_args.push_back(arg_llvm_type);
    }

    next_index(c, type_name.length());
    next_index(c, type_name.length());

    std::string return_type_string = type_name.substr(c, type_name.length()-c);
    assert(return_type_string != "");

    if(this->find_type(return_type_string, return_llvm_type) != 0){
        fail(UNDECLARED_TYPE(return_type_string));
        return 1;
    }

    return 0;
}
int Program::extract_function_pointer_info(const std::string& type_name, std::vector<llvm::Type*>& llvm_args, llvm::Type** return_llvm_type, std::vector<std::string>& typenames, std::string& return_typename) const {
    // "def(int, int) int"
    // -----^

    // NOTE: type_name is assumed to be a valid function pointer type

    size_t c = 0;
    if(type_name.length() > 8){
        if(type_name.substr(0, 8) == "stdcall "){
            c = 8;
        }
    }

    c += 4;

    while(type_name[c] != ')'){
        llvm::Type* arg_llvm_type;
        std::string arg_type_string;
        int balance = 0;

        while((type_name[c] != ')' and type_name[c] != ',') or balance != 0){
            if(type_name[c] == '(') balance += 1;
            if(type_name[c] == ')') balance -= 1;
            arg_type_string += type_name[c];
            next_index(c, type_name.length());
        }
        if(type_name[c] == ','){
            next_index(c, type_name.length());
            next_index(c, type_name.length());
        }
        if(this->find_type(arg_type_string, &arg_llvm_type) != 0){
            fail(UNDECLARED_TYPE(arg_type_string));
            return 1;
        }

        typenames.push_back(arg_type_string);
        llvm_args.push_back(arg_llvm_type);
    }

    next_index(c, type_name.length());
    next_index(c, type_name.length());

    return_typename = type_name.substr(c, type_name.length()-c);
    assert(return_typename != "");

    if(this->find_type(return_typename, return_llvm_type) != 0){
        fail(UNDECLARED_TYPE(return_typename));
        return 1;
    }

    return 0;
}
int Program::function_typename_to_type(const std::string& type_name, llvm::Type** type) const {
    // "def(int, int) int"
    // -----^

    // NOTE: The first four characters of type_name are assumed to be "def("

    std::vector<llvm::Type*> llvm_args;
    llvm::Type* return_llvm_type;

    // Extract information from function pointer type
    if(this->extract_function_pointer_info(type_name, llvm_args, &return_llvm_type) != 0) return 1;

    llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, llvm_args, false);
    *type = function_type->getPointerTo();
    return 0;
}
void Program::apply_type_modifiers(llvm::Type** type, const std::vector<Program::TypeModifier>& modifiers) const{
    for(size_t i = modifiers.size(); i-- != 0;){
        switch(modifiers[i]){
        case Program::TypeModifier::Pointer:
            *type = (*type)->getPointerTo();
            break;
        case Program::TypeModifier::Array:
            *type = llvm_array_type;
            break;
        }
    }
}

void Program::generate_type_aliases(){
    // Standard type aliases
    type_aliases.push_back( TypeAlias("usize", "uint", false) );
}
int Program::generate_types(AssembleContext& context){
    // NOTE: Requires types vector to be empty
    assert(types.size() == 0);

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

    // Create a struct type for arrays
    std::vector<llvm::Type*> elements = { llvm::Type::getInt8PtrTy(context.context), llvm::Type::getInt32Ty(context.context) };
    llvm_array_type = llvm::StructType::create(elements, ".arr");

    // Fill in llvm structures from structure data
    for(size_t i = 0; i != structures.size(); i++){
        Structure& struct_data = structures[i];
        std::vector<llvm::Type*> members;
        members.reserve(16);

        for(size_t j = 0; j != struct_data.members.size(); j++){
            llvm::Type* member_type;
            if(this->find_type(struct_data.members[j].type, &member_type) != 0) {
                std::cerr << "The type '" << struct_data.members[j].type << "' does not exist" << std::endl;
                return 1;
            }
            members.push_back(member_type);
        }

        static_cast<llvm::StructType*>(types[i].type)->setBody(members, struct_data.is_packed);
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

    // Create a constructor for arrays
    std::vector<llvm::Type*> array_ctor_args = { llvm::Type::getInt8PtrTy(context.context), llvm::Type::getInt32Ty(context.context) };
    llvm::FunctionType* array_ctor_type = llvm::FunctionType::get(llvm_array_type, array_ctor_args, false);
    llvm_array_ctor = llvm::Function::Create(array_ctor_type, llvm::Function::ExternalLinkage, ".arrctor", context.module.get());

    return 0;
}
int Program::find_type(const std::string& name, llvm::Type** type) const{
    // For keeping track of 'pointers to' & 'arrays of'
    std::vector<TypeModifier> type_modifiers;

    std::string type_name;
    size_t type_i = 0;
    size_t alias_type_i = 0;

    for(; name[type_i] == '*' or name[type_i] == '['; type_i++){
        switch(name[type_i]){
        case '*':
            type_modifiers.push_back(TypeModifier::Pointer);
            break;
        case '[':
            type_i += 1; // Next character is assumed to be ']'
            type_modifiers.push_back(TypeModifier::Array);
            break;
        }
    }

    type_name = name.substr(type_i, name.length()-type_i);
    this->resolve_if_alias(type_name);

    for(; type_name[alias_type_i] == '*' or type_name[alias_type_i] == '['; alias_type_i++){
        switch(type_name[alias_type_i]){
        case '*':
            type_modifiers.push_back(TypeModifier::Pointer);
            break;
        case '[':
            alias_type_i += 1; // Next character is assumed to be ']'
            type_modifiers.push_back(TypeModifier::Array);
            break;
        }
    }
    type_name = type_name.substr(alias_type_i, type_name.length()-alias_type_i);

    if(Program::is_function_typename(type_name)){
        // Return after handling function pointer type
        if(this->function_typename_to_type(type_name, type) != 0) return 1;
        this->apply_type_modifiers(type, type_modifiers);
        return 0;
    }

    for(size_t i = 0; i != types.size(); i++){
        if(types[i].name == type_name){
            if(type != NULL){
                *type = types[i].type;
                this->apply_type_modifiers(type, type_modifiers);
            }
            return 0;
        }
    }

    return 1;
}
int Program::find_func(const std::string& name, External* func){
    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].name == name){
            // SPEED: Maybe cache 'functions[i]'?

            External external;
            external.name = functions[i].name;
            external.return_type = functions[i].return_type;
            external.is_public = functions[i].is_public;
            external.is_mangled = true;
            external.is_stdcall = functions[i].is_stdcall;
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
            External external;
            bool args_match = true;
            Function* function_found = &functions[i];

            if(args.size() != function_found->arguments.size()) continue;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(*this, args[a], function_found->arguments[a].type) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            external.name = function_found->name;
            external.return_type = function_found->return_type;
            external.is_public = function_found->is_public;
            external.is_mangled = true;
            external.is_stdcall = function_found->is_stdcall;
            for(Field& field : function_found->arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    for(size_t i = 0; i != externs.size(); i++){
        if(externs[i].name == name){
            bool args_match = true;
            if(args.size() != externs[i].arguments.size()) continue;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(*this, args[a], externs[i].arguments[a]) ){
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
            External external;
            bool args_match = true;
            Function* found_method = &klass.methods[i];

            if(args.size() != found_method->arguments.size()) continue;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(*this, args[a], found_method->arguments[a].type) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            external.name = found_method->name;
            external.return_type = found_method->return_type;
            external.is_public = found_method->is_public;
            external.is_mangled = true;
            external.is_stdcall = found_method->is_stdcall;
            for(Field& field : found_method->arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    return 1;
}
int Program::find_struct(const std::string& name, Structure* structure){
    for(size_t i = 0; i != structures.size(); i++){
        if(structures[i].name == name){
            if(structure != NULL) *structure = structures[i];
            return 0;
        }
    }

    return 1;
}
int Program::find_class(const std::string& name, Class* klass){
    for(size_t i = 0; i != classes.size(); i++){
        if(classes[i].name == name){
            if(klass != NULL) *klass = classes[i];
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
int Program::find_global(const std::string& name, Global* global){
    for(size_t i = 0; i != globals.size(); i++){
        if(globals[i].name == name){
            *global = globals[i];
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
    print_globals();
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
        if(e.is_stdcall) std::cout << "stdcall ";
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
void Program::print_globals(){
    for(const Global& global : globals){
        std::cout << (global.is_public ? "public " : "private ") << global.name << " " << global.type << std::endl;
    }
}
