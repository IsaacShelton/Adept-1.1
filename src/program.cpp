
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include "../include/die.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/program.h"
#include "../include/assemble.h"
#include "../include/mangling.h"

Structure::Structure(){}
Structure::Structure(const std::string& name, const std::vector<Field>& members, bool is_public, bool is_packed, OriginInfo* origin){
    this->name = name;
    this->members = members;
    this->is_public = is_public;
    this->is_packed = is_packed;
    this->origin = origin;
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

Function::Function(){
    this->is_public = false;
    this->is_static = false;
    this->is_stdcall = false;
    this->is_external = false;
    this->is_variable_args = false;
    this->parent_class_offset = 0;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, bool is_public, OriginInfo* origin){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->is_public = is_public;
    this->is_static = false;
    this->is_stdcall = false;
    this->is_external = false;
    this->is_variable_args = false;
    this->parent_class_offset = 0;
    this->origin = origin;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, const StatementList& statements,
                   bool is_public, bool is_static, bool is_stdcall, bool is_external, bool is_variable_args, OriginInfo* origin){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->statements = statements;
    this->is_public = is_public;
    this->is_static = is_static;
    this->is_stdcall = is_stdcall;
    this->is_external = is_external;
    this->is_variable_args = is_variable_args;
    this->parent_class_offset = 0;
    this->origin = origin;
}
Function::Function(const Function& other){
    name = other.name;
    arguments = other.arguments;
    return_type = other.return_type;
    statements.resize(other.statements.size());
    is_public = other.is_public;
    is_static = other.is_static;
    is_stdcall = other.is_stdcall;
    is_external = other.is_external;
    is_variable_args = other.is_variable_args;
    parent_class_offset = other.parent_class_offset;
    origin = other.origin;

    for(size_t i = 0; i != other.statements.size(); i++){
        statements[i] = other.statements[i]->clone();
    }
}
Function::~Function(){
    for(Statement* s : statements) delete s;
}
void Function::print_statements(){
    for(Statement* statement : statements){
        std::cout << statement->toString(0, false) << std::endl;
    }
}

External::External(){}
External::External(const std::string& name, const std::vector<std::string>& arguments, const std::string& return_type, bool is_public,
                   bool is_mangled, bool is_stdcall, bool is_variable_args, OriginInfo* origin){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->is_public = is_public;
    this->is_mangled = is_mangled;
    this->is_stdcall = is_stdcall;
    this->is_variable_args = is_variable_args;
    this->origin = origin;
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

ModuleDependency::ModuleDependency(const std::string& filename, const std::string& target_bc, const std::string& target_obj,
                                   Configuration* config, bool is_nothing){
    this->filename = filename;
    this->target_bc = target_bc;
    this->target_obj = target_obj;
    this->config = config;
    this->is_nothing = is_nothing;
}

ImportDependency::ImportDependency(const std::string& filename, bool is_public){
    this->filename = filename;
    this->is_public = is_public;
}

Constant::Constant(){}
Constant::Constant(const std::string& name, PlainExp* value, bool is_public, OriginInfo* origin){
    this->name = name;
    this->value = value;
    this->is_public = is_public;
    this->origin = origin;
}

Global::Global(){}
Global::Global(const std::string& name, const std::string& type, bool is_public, bool is_external, ErrorHandler& errors, OriginInfo* origin){
    this->name = name;
    this->type = type;
    this->is_public = is_public;
    this->is_imported = false;
    this->is_external = is_external;
    this->errors = errors;
    this->origin = origin;
}

Class::Class(){
    this->is_public = false;
    this->is_imported = false;
    this->origin = NULL;
}
Class::Class(const std::string& name, bool is_public, bool is_imported, OriginInfo* origin){
    this->name = name;
    this->is_public = is_public;
    this->is_imported = is_imported;
    this->origin = origin;
}
Class::Class(const std::string& name, const std::vector<ClassField>& members, bool is_public, OriginInfo* origin){
    this->name = name;
    this->members = members;
    this->is_public = is_public;
    this->is_imported = false;
    this->origin = origin;
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
TypeAlias::TypeAlias(const std::string& alias, const std::string& binding, bool is_public, OriginInfo* origin){
    this->alias = alias;
    this->binding = binding;
    this->is_public = is_public;
    this->origin = origin;
}

EnumField::EnumField(const std::string& name, ErrorHandler& errors){
    this->name = name;
    this->value = NULL;
    this->errors = errors;
}
EnumField::EnumField(const std::string& name, PlainExp* value, ErrorHandler& errors){
    this->name = name;
    this->value = value;
    this->errors = errors;
}
EnumField::EnumField(const EnumField& other){
    name = other.name;
    value = (other.value != NULL) ? other.value->clone() : NULL;
    errors = other.errors;
}
EnumField::~EnumField(){
    delete value;
}

Enum::Enum(const std::string& name, std::vector<EnumField>& fields, bool is_public, OriginInfo* origin){
    this->name = name;
    this->fields = fields;
    this->is_public = is_public;
    this->origin = origin;
    this->bits = 0; // Should be overridden later on in 'Program::generate_types()'
}

Program::Program(CacheManager* parent_manager, const std::string& filename){
    this->parent_manager = parent_manager;
    this->origin_info.filename = filename;
}
Program::~Program(){
    for(ModuleDependency& pkg : dependencies){
        delete pkg.config;
    }

    for(Constant& constant : constants){
        delete constant.value;
    }
}
int Program::import_merge(Configuration* config, Program& other, bool public_import, ErrorHandler& errors){
    // Merge Dependencies
    for(const ModuleDependency& new_dependency : other.dependencies){
        bool already_exists = false;

        for(const ModuleDependency& dependency : dependencies){
            if(new_dependency.target_obj == dependency.target_obj){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            ModuleDependency created_dependency = new_dependency;
            created_dependency.config = new Configuration(*new_dependency.config);
            dependencies.push_back(created_dependency);
        }
    }

    // Merge Functions
    for(const Function& new_func : other.functions){
        if(!new_func.is_public) continue;
        if(this->already_imported(new_func.origin)) continue;
        std::string new_final_name = mangle(other, new_func);

        for(const Function& func : functions){
            if(new_final_name == mangle(other, func)){
                errors.panic(DUPLICATE_FUNC(new_func.name));
                return 1;
            }
        }

        for(const External& external : externs){
            if( new_final_name == (external.is_mangled ? mangle(external.name, external.arguments) : external.name) ){
                errors.panic(DUPLICATE_FUNC(new_func.name));
                return 1;
            }
        }

        std::vector<std::string> arg_typenames(new_func.arguments.size());

        for(size_t i = 0; i != new_func.arguments.size(); i++){
            arg_typenames[i] = new_func.arguments[i].type;
        }

        External created_external(new_func.name, arg_typenames, new_func.return_type, public_import, !(new_func.is_external), new_func.is_stdcall, new_func.is_variable_args, new_func.origin);
        externs.push_back(created_external);
    }

    // Merge Structures
    for(const Structure& new_structure : other.structures){
        if(!new_structure.is_public) continue;
        if(this->already_imported(new_structure.origin)) continue;

        for(const Structure& structure : structures){
            if(new_structure.name == structure.name){
                errors.panic("Imported structure '" + new_structure.name + "' has the same name as a local structure");
                return 1;
            }
        }

        for(const Class& klass : classes){
            if(new_structure.name == klass.name){
                errors.panic("Imported structure '" + new_structure.name + "' has the same name as a local class");
                return 1;
            }
        }

        for(const TypeAlias& type_alias : type_aliases){
            if(new_structure.name == type_alias.alias){
                errors.panic("Imported structure '" + new_structure.name + "' has the same name as a local type alias");
                return 1;
            }
        }

        for(const Enum& inum : enums){
            if(new_structure.name == inum.name){
                errors.panic("Imported structure '" + new_structure.name + "' has the same name as a local enum");
                return 1;
            }
        }

        Structure target = new_structure;
        target.is_public = public_import;
        structures.push_back( std::move(target) );
    }

    // Merge Classes
    for(const Class& new_class : other.classes){
        if(!new_class.is_public) continue;
        if(this->already_imported(new_class.origin)) continue;

        for(const Class& klass : classes){
            if(new_class.name == klass.name){
                errors.panic("Imported class '" + new_class.name + "' has the same name as a local class");
                return 1;
            }
        }

        for(const Structure& structure : structures){
            if(new_class.name == structure.name){
                errors.panic("Imported class '" + new_class.name + "' has the same name as a local structure");
                return 1;
            }
        }

        for(const TypeAlias& type_alias : type_aliases){
            if(new_class.name == type_alias.alias){
                errors.panic("Imported class '" + new_class.name + "' has the same name as a local type alias");
                return 1;
            }
        }

        for(const Enum& inum : enums){
            if(new_class.name == inum.name){
                errors.panic("Imported class '" + new_class.name + "' has the same name as a local enum");
                return 1;
            }
        }

        classes.resize(classes.size() + 1);
        Class* target = &classes[classes.size()-1];

        *target = new_class;
        target->is_public = public_import;
        target->is_imported = true;

        for(size_t i = 0; i != target->methods.size(); i++){
            target->methods[i].parent_class_offset = classes.size();
        }
    }

    // Merge Externals
    for(const External& new_external : other.externs){
        if(!new_external.is_public) continue;
        if(this->already_imported(new_external.origin)) continue;
        std::string new_external_final_name = (new_external.is_mangled) ? mangle(new_external.name, new_external.arguments) : new_external.name;

        for(const External& external : externs){
            if( new_external_final_name == (external.is_mangled ? mangle(external.name, external.arguments) : external.name) ){
                errors.panic(DUPLICATE_FUNC(new_external.name));
            }
        }

        External target = new_external;
        target.is_public = public_import;
        externs.push_back(target);
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
        if(this->already_imported(new_constant.origin)) continue;

        for(const Constant& constant : constants){
            if(new_constant.name == constant.name){
                errors.panic("Imported constant '$" + new_constant.name + "' has the same name as a local constant");
                return 1;
            }
        }

        Constant created_constant(new_constant.name, new_constant.value->clone(), public_import, new_constant.origin);
        constants.push_back(created_constant);
    }

    // Merge Globals
    for(Global& new_global : other.globals){
        if(!new_global.is_public) continue;
        if(this->already_imported(new_global.origin)) continue;

        for(const Global& global : globals){
            if(new_global.name == global.name){
                errors.panic("Imported global variable '" + new_global.name + "' has the same name as a local global variable");
                return 1;
            }
        }

        Global cloned_global = new_global;
        cloned_global.is_imported = true;
        cloned_global.is_public = public_import;
        globals.push_back(std::move(cloned_global));
    }

    // Merge Type Aliases
    for(TypeAlias& new_type_alias : other.type_aliases){
        if(!new_type_alias.is_public) continue;
        if(this->already_imported(new_type_alias.origin)) continue;

        for(const TypeAlias& type_alias : type_aliases){
            if(new_type_alias.alias == type_alias.alias){
                errors.panic("Imported type alias '" + new_type_alias.alias + "' has the same name as a local type alias");
                return 1;
            }
        }

        for(const Structure& structure : structures){
            if(new_type_alias.alias == structure.name){
                errors.panic("Imported type alias '" + new_type_alias.alias + "' has the same name as a local structure");
                return 1;
            }
        }

        for(const Class& klass : classes){
            if(new_type_alias.alias == klass.name){
                errors.panic("Imported type alias '" + new_type_alias.alias + "' has the same name as a local class");
                return 1;
            }
        }

        for(const Enum& inum : enums){
            if(new_type_alias.alias == inum.name){
                errors.panic("Imported type alias '" + new_type_alias.alias + "' has the same name as a local enum");
                return 1;
            }
        }

        TypeAlias created_type_alias = new_type_alias;
        created_type_alias.is_public = public_import;
        type_aliases.push_back(std::move(created_type_alias));
    }

    // Merge Enums
    for(const Enum& new_enum : other.enums){
        if(!new_enum.is_public) continue;
        if(this->already_imported(new_enum.origin)) continue;

        for(const Structure& structure : structures){
            if(new_enum.name == structure.name){
                errors.panic("Imported enum '" + new_enum.name + "' has the same name as a local structure");
                return 1;
            }
        }

        for(const Class& klass : classes){
            if(new_enum.name == klass.name){
                errors.panic("Imported enum '" + new_enum.name + "' has the same name as a local class");
                return 1;
            }
        }

        for(const TypeAlias& type_alias : type_aliases){
            if(new_enum.name == type_alias.alias){
                errors.panic("Imported enum '" + new_enum.name + "' has the same name as a local type alias");
                return 1;
            }
        }

        for(const Enum& inum : enums){
            if(new_enum.name == inum.name){
                errors.panic("Imported enum '" + new_enum.name + "' has the same name as a local enum");
                return 1;
            }
        }

        Enum target = new_enum;
        target.is_public = public_import;
        enums.push_back( std::move(target) );
    }

    // Merge imports last
    for(const ImportDependency& new_import : other.imports){
        if( !(new_import.is_public) ) continue;
        bool already_exists = false;

        for(const ImportDependency& existing_import : imports){
            if(new_import.filename == existing_import.filename){
                already_exists = true;
                break;
            }
        }

        if(!already_exists){
            imports.push_back( ImportDependency(new_import.filename, public_import) );
        }
    }

    return 0;
}

bool Program::resolve_if_alias(std::string& type) const {
    // Returns true if completed a substitution

    size_t pointer_count;
    for(pointer_count = 0; pointer_count != type.size(); pointer_count++){
        if(type[pointer_count] != '*') break;
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

    size_t pointer_count;
    for(pointer_count = 0; pointer_count != type.size(); pointer_count++){
        if(type[pointer_count] != '*') break;
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

int Program::extract_function_pointer_info(const std::string& type_name, std::vector<llvm::Type*>& llvm_args, AssemblyData& context, llvm::Type** return_llvm_type) const {
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
        if(this->find_type(arg_type_string, context, &arg_llvm_type) != 0){
            fail(UNDECLARED_TYPE(arg_type_string));
            return 1;
        }

        llvm_args.push_back(arg_llvm_type);
    }

    next_index(c, type_name.length());
    next_index(c, type_name.length());

    std::string return_type_string = type_name.substr(c, type_name.length()-c);
    ensure(return_type_string != "");

    if(this->find_type(return_type_string, context, return_llvm_type) != 0){
        fail(UNDECLARED_TYPE(return_type_string));
        return 1;
    }

    return 0;
}
int Program::extract_function_pointer_info(const std::string& type_name, std::vector<llvm::Type*>& llvm_args, AssemblyData& context, llvm::Type** return_llvm_type, std::vector<std::string>& typenames, std::string& return_typename) const {
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
        if(this->find_type(arg_type_string, context, &arg_llvm_type) != 0){
            fail(UNDECLARED_TYPE(arg_type_string));
            return 1;
        }

        typenames.push_back(arg_type_string);
        llvm_args.push_back(arg_llvm_type);
    }

    next_index(c, type_name.length());
    next_index(c, type_name.length());

    return_typename = type_name.substr(c, type_name.length()-c);
    ensure(return_typename != "");

    if(this->find_type(return_typename, context, return_llvm_type) != 0){
        fail(UNDECLARED_TYPE(return_typename));
        return 1;
    }

    return 0;
}
int Program::function_typename_to_type(const std::string& type_name, AssemblyData& context, llvm::Type** type) const {
    // "def(int, int) int"
    // -----^

    // NOTE: The first four characters of type_name are assumed to be "def("

    std::vector<llvm::Type*> llvm_args;
    llvm::Type* return_llvm_type;

    // Extract information from function pointer type
    if(this->extract_function_pointer_info(type_name, llvm_args, context, &return_llvm_type) != 0) return 1;

    llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, llvm_args, false);
    if(type != NULL) *type = function_type->getPointerTo();
    return 0;
}
void Program::apply_type_modifiers(llvm::Type** type, const std::vector<Program::TypeModifier>& modifiers) const {
    for(const std::vector<Program::TypeModifier>::const_reverse_iterator i = modifiers.rbegin(); i != modifiers.rend(); ++i){
        switch(*i){
        case Program::TypeModifier::Pointer:
            *type = (*type)->getPointerTo();
            break;
        case Program::TypeModifier::Array:
            *type = llvm_array_type;
            break;
        }
    }
}

bool Program::already_imported(OriginInfo* origin){
    for(const ImportDependency& import_dependency : imports){
        if(import_dependency.filename == origin->filename) return true;
    }
    return false;
}
void Program::generate_type_aliases(){
    // Standard type aliases
    type_aliases.push_back( TypeAlias("usize", "uint", false, &origin_info) );
}
int Program::generate_types(AssemblyData& context){
    // NOTE: Requires types vector to be empty
    ensure(context.types.size() == 0);

    llvm::LLVMContext& llvm_context = context.context;
    std::vector<Type>& types = context.types;

    // Create from structures
    for(size_t i = 0; i != structures.size(); i++){
        llvm::StructType* llvm_struct = llvm::StructType::create(llvm_context, structures[i].name);
        types.push_back( Type(structures[i].name, llvm_struct) );
    }

    // Mark starting point for classes
    size_t classes_start_index = structures.size();

    // Create from classes
    for(Class& klass : classes){
        llvm::StructType* llvm_struct = llvm::StructType::create(llvm_context, klass.name);
        types.push_back( Type(klass.name, llvm_struct) );
    }

    // Standard language types
    types.push_back( Type("void", llvm::Type::getVoidTy(llvm_context)) );
    types.push_back( Type("bool", llvm::Type::getInt1Ty(llvm_context)) );
    types.push_back( Type("ptr", llvm::Type::getInt8PtrTy(llvm_context)) );
    types.push_back( Type("int", llvm::Type::getInt32Ty(llvm_context)) );
    types.push_back( Type("uint", llvm::Type::getInt32Ty(llvm_context)) );
    types.push_back( Type("double", llvm::Type::getDoubleTy(llvm_context)) );
    types.push_back( Type("float", llvm::Type::getFloatTy(llvm_context)) );
    types.push_back( Type("half", llvm::Type::getHalfTy(llvm_context)) );
    types.push_back( Type("byte", llvm::Type::getInt8Ty(llvm_context)) );
    types.push_back( Type("ubyte", llvm::Type::getInt8Ty(llvm_context)) );
    types.push_back( Type("short", llvm::Type::getInt16Ty(llvm_context)) );
    types.push_back( Type("ushort", llvm::Type::getInt16Ty(llvm_context)) );
    types.push_back( Type("long", llvm::Type::getInt64Ty(llvm_context)) );
    types.push_back( Type("ulong", llvm::Type::getInt64Ty(llvm_context)) );

    for(Enum& inum : enums){
        if(inum.fields.size() <= 256){
            inum.bits = 8;
            types.push_back( Type(inum.name, llvm::Type::getInt8Ty(llvm_context)) );
            continue;
        }
        if(inum.fields.size() <= 65536){
            inum.bits = 16;
            types.push_back( Type(inum.name, llvm::Type::getInt16Ty(llvm_context)) );
            continue;
        }
        if(inum.fields.size() <= 4294967296){
            inum.bits = 32;
            types.push_back( Type(inum.name, llvm::Type::getInt32Ty(llvm_context)) );
            continue;
        }

        inum.bits = 64;
        types.push_back( Type(inum.name, llvm::Type::getInt64Ty(llvm_context)) );
    }

    // Create a struct type for arrays
    std::vector<llvm::Type*> elements = { llvm::Type::getInt8PtrTy(llvm_context), llvm::Type::getInt32Ty(llvm_context) };
    llvm_array_type = llvm::StructType::create(elements, ".arr");

    // Data for iterating structures
    std::vector<llvm::Type*> members;
    llvm::Type* member_type;
    size_t index = 0;
    std::vector<Field>::iterator members_end;
    members.reserve(8);

    // Fill in llvm structures from structure data
    for(Structure& struct_data : structures){
        members.clear();
        members_end = struct_data.members.end();

        for(std::vector<Field>::iterator i = struct_data.members.begin(); i != members_end; ++i){
            if(this->find_type(i->type, context, &member_type) != 0) {
                fail_filename(origin_info.filename, "The type '" + i->type + "' does not exist");
                return 1;
            }
            members.push_back(member_type);
        }

        static_cast<llvm::StructType*>(types[index].type)->setBody(members, struct_data.is_packed);
        index++;
    }

    // Fill in llvm structures from class data
    for(size_t i = 0; i != classes.size(); i++){
        Class& class_data = classes[i];
        std::vector<llvm::Type*> members;
        members.reserve(100);

        for(size_t j = 0; j != class_data.members.size(); j++){
            if(this->find_type(class_data.members[j].type, context, &member_type) != 0) {
                fail_filename(origin_info.filename, "The type '" + class_data.members[j].type + "' does not exist");
                return 1;
            }
            members.push_back(member_type);
        }

        static_cast<llvm::StructType*>(types[classes_start_index+i].type)->setBody(members);
    }

    // Create a constructor for arrays
    std::vector<llvm::Type*> array_ctor_args = { llvm::Type::getInt8PtrTy(llvm_context), llvm::Type::getInt32Ty(llvm_context) };
    llvm::FunctionType* array_ctor_type = llvm::FunctionType::get(llvm_array_type, array_ctor_args, false);
    llvm_array_ctor = llvm::Function::Create(array_ctor_type, llvm::Function::ExternalLinkage, ".__adept_core_arrctor", context.module.get());

    return 0;
}

int Program::find_type(const std::string& name, AssemblyData& context, llvm::Type** type) const {
    std::vector<Type>& types = context.types;
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
        if(this->function_typename_to_type(type_name, context, type) != 0) return 1;
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
            Function* found_function = &functions[i];
            External external;
            external.name = found_function->name;
            external.return_type = found_function->return_type;
            external.is_public = found_function->is_public;
            external.is_mangled = !(found_function->is_external);
            external.is_stdcall = found_function->is_stdcall;
            external.is_variable_args = found_function->is_variable_args;
            for(Field& field : found_function->arguments) external.arguments.push_back(field.type);
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
    bool args_match;

    // Check for matching non-var-arg functions
    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].name == name){
            External external;
            Function* function_found = &functions[i];

            if(function_found->is_variable_args) continue; // Prefer non-var-args functions if possible
            if(args.size() != function_found->arguments.size()) continue; // Continue if different amount of arguments

            args_match = true;
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
            external.is_mangled = !(function_found->is_external);
            external.is_stdcall = function_found->is_stdcall;
            external.is_variable_args = function_found->is_variable_args;
            for(Field& field : function_found->arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    // Check for matching non-var-args externals
    for(size_t i = 0; i != externs.size(); i++){
        if(externs[i].name == name){
            External* external = &externs[i];

            if(external->is_variable_args) continue; // Perfer non-var-args externals if possible
            if(args.size() != external->arguments.size()) continue; // Continue if different amount of arguments

            args_match = true;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(*this, args[a], external->arguments[a]) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            *func = externs[i];
            return 0;
        }
    }

    // Check for compatible var-args functions
    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].name == name){
            External external;
            Function* function_found = &functions[i];

            // We already checked for functions that don't take a variable number of arguments
            if(!function_found->is_variable_args) continue;

            size_t a;
            size_t last_arg = function_found->arguments.size() - 1;
            args_match = true;

            for(a = 0; a != last_arg; a++){
                if( !assemble_types_mergeable(*this, args[a], function_found->arguments[a].type) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            std::string target_va_type = function_found->arguments[last_arg].type;
            target_va_type = target_va_type.substr(2, target_va_type.length() - 2);

            while(a != args.size()){
                if( !assemble_types_mergeable(*this, args[a], target_va_type) ){
                    args_match = false;
                    break;
                }
                a++;
            }
            if(!args_match) continue;

            external.name = function_found->name;
            external.return_type = function_found->return_type;
            external.is_public = function_found->is_public;
            external.is_mangled = !(function_found->is_external);
            external.is_stdcall = function_found->is_stdcall;
            external.is_variable_args = function_found->is_variable_args;
            for(Field& field : function_found->arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    // Check for matching var-args externals
    for(size_t i = 0; i != externs.size(); i++){
        if(externs[i].name == name){
            External* external = &externs[i];

            // We already checked for externals that don't take a variable number of arguments
            if(!external->is_variable_args) continue;

            size_t a;
            size_t last_arg = external->arguments.size() - 1;
            args_match = true;

            for(a = 0; a != last_arg; a++){
                if( !assemble_types_mergeable(*this, args[a], external->arguments[a]) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            std::string target_va_type = external->arguments[last_arg];
            target_va_type = target_va_type.substr(2, target_va_type.length() - 2);

            while(a != args.size()){
                if( !assemble_types_mergeable(*this, args[a], target_va_type) ){
                    args_match = false;
                    break;
                }
                a++;
            }
            if(!args_match) continue;

            *func = *external;
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
    std::vector<Global>::iterator globals_end = globals.end();
    for(std::vector<Global>::iterator i = globals.begin(); i != globals_end; ++i){
        if(i->name == name){
            *global = *i;
            return 0;
        }
    }

    return 1;
}
void Program::print(){
    std::cout << std::endl;
    print_externals();
    print_enums();
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
void Program::print_enums(){
    for(const Enum& inum : enums){
        std::cout << (inum.is_public ? "public " : "private ") << "enum " << inum.name << " {\n";
        for(size_t i = 0; i != inum.fields.size(); i++){
            std::cout << inum.fields[i].name;
            if(i + 1 != inum.fields.size()) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
    }
}
