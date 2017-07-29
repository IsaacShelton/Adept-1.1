
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include "../include/die.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/program.h"
#include "../include/assemble.h"
#include "../include/mangling.h"

Function::Function(){
    this->flags = 0x00;
    this->parent_struct_offset = 0;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, bool is_public, OriginInfo* origin){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->flags = (is_public) ? FUNC_PUBLIC : 0x00;
    this->parent_struct_offset = 0;
    this->origin = origin;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::string& return_type, const StatementList& statements,
                   bool is_public, bool is_static, bool is_stdcall, bool is_external, bool is_variable_args, OriginInfo* origin){
    this->name = name;
    this->arguments = arguments;
    this->return_type = return_type;
    this->statements = statements;

    char new_flags = 0x00;
    if(is_public)        new_flags |= FUNC_PUBLIC;
    if(is_static)        new_flags |= FUNC_STATIC;
    if(is_stdcall)       new_flags |= FUNC_STDCALL;
    if(is_external)      new_flags |= FUNC_EXTERNAL;
    if(is_variable_args) new_flags |= FUNC_VARARGS;

    this->flags = new_flags;
    this->parent_struct_offset = 0;
    this->origin = origin;
}
Function::Function(const std::string& name, const std::vector<Field>& arguments, const std::vector<std::string>& extra_return_types, const StatementList& statements,
                   bool is_public, bool is_static, bool is_stdcall, bool is_external, bool is_variable_args, OriginInfo* origin){
    this->name = name;
    this->arguments = arguments;
    this->return_type = "void";
    this->statements = statements;

    char new_flags = FUNC_MULRET;
    if(is_public)        new_flags |= FUNC_PUBLIC;
    if(is_static)        new_flags |= FUNC_STATIC;
    if(is_stdcall)       new_flags |= FUNC_STDCALL;
    if(is_external)      new_flags |= FUNC_EXTERNAL;
    if(is_variable_args) new_flags |= FUNC_VARARGS;

    this->flags = new_flags;
    this->parent_struct_offset = 0;
    this->extra_return_types = extra_return_types;
    this->origin = origin;
}
Function::Function(const Function& other){
    name = other.name;
    arguments = other.arguments;
    return_type = other.return_type;
    statements.resize(other.statements.size());
    flags = other.flags;
    parent_struct_offset = other.parent_struct_offset;
    origin = other.origin;

    for(size_t i = 0; i != other.statements.size(); i++){
        statements[i] = other.statements[i]->clone();
    }

    if(flags & FUNC_MULRET){
        extra_return_types = other.extra_return_types;
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

    char new_flags = 0x00;
    if(is_public)        new_flags |= EXTERN_PUBLIC;
    if(is_mangled)       new_flags |= EXTERN_MANGLED;
    if(is_stdcall)       new_flags |= EXTERN_STDCALL;
    if(is_variable_args) new_flags |= EXTERN_VARARGS;

    this->flags = new_flags;
    this->origin = origin;
}
std::string External::toString(){
    std::string prefix = std::string(flags & EXTERN_PUBLIC ? "public " : "private ");
    std::string arg_str;

    if(flags & EXTERN_STDCALL) prefix += "stdcall ";

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

    char new_flags = 0x00;
    if(is_public) new_flags |= GLOBAL_PUBLIC;
    if(is_external) new_flags |= GLOBAL_EXTERNAL;

    this->flags = new_flags;
    this->errors = errors;
    this->origin = origin;
}

Struct::Struct(){
    this->flags = 0x00;
    this->origin = NULL;
}
Struct::Struct(const std::string& name, bool is_public, bool is_imported, OriginInfo* origin){
    this->name = name;

    char new_flags = 0x00;
    if(is_public)   new_flags |= STRUCT_PUBLIC;
    if(is_imported) new_flags |= STRUCT_IMPORTED;

    this->flags = new_flags;
    this->origin = origin;
}
Struct::Struct(const std::string& name, const std::vector<StructField>& members, bool is_public, OriginInfo* origin){
    this->name = name;
    this->members = members;
    this->flags = (is_public) ? STRUCT_PUBLIC : 0x00;
    this->origin = origin;
}
int Struct::find_index(std::string member_name, int* index){
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
    // TODO: SPEED: This method is probably the slowest in the entire compiler

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
        if(!(new_func.flags & FUNC_PUBLIC)) continue;
        if(this->already_imported(new_func.origin)) continue;
        std::string new_final_name = mangle(other, new_func);

        for(const Function& func : functions){
            if(new_final_name == mangle(other, func)){
                errors.panic(DUPLICATE_FUNC(new_func.name));
                return 1;
            }
        }

        for(const External& external : externs){
            if( new_final_name == (external.flags & EXTERN_MANGLED ? mangle(external.name, external.arguments) : external.name) ){
                errors.panic(DUPLICATE_FUNC(new_func.name));
                return 1;
            }
        }

        std::vector<std::string> arg_typenames(new_func.arguments.size());

        for(size_t i = 0; i != new_func.arguments.size(); i++){
            arg_typenames[i] = new_func.arguments[i].type;
        }

        External created_external(new_func.name, arg_typenames, new_func.return_type, public_import, !(new_func.flags & FUNC_EXTERNAL), new_func.flags & FUNC_STDCALL, new_func.flags & FUNC_VARARGS, new_func.origin);
        externs.push_back(created_external);
    }

    // Merge Structs
    // TODO: SPEED: This is very slow
    for(const Struct& new_struct : other.structures){
        if(!(new_struct.flags & STRUCT_PUBLIC)) continue;
        if(this->already_imported(new_struct.origin)) continue;

        for(const Struct& structure : structures){
            if(new_struct.name == structure.name){
                errors.panic("Imported struct '" + new_struct.name + "' has the same name as a local struct");
                return 1;
            }
        }

        for(const TypeAlias& type_alias : type_aliases){
            if(new_struct.name == type_alias.alias){
                errors.panic("Imported struct '" + new_struct.name + "' has the same name as a local type alias");
                return 1;
            }
        }

        for(const Enum& inum : enums){
            if(new_struct.name == inum.name){
                errors.panic("Imported struct '" + new_struct.name + "' has the same name as a local enum");
                return 1;
            }
        }

        structures.resize(structures.size() + 1);
        Struct* target = &structures[structures.size()-1];
        *target = new_struct;

        // Manipulate the flags to show that this struct was imported
        if((target->flags & STRUCT_PUBLIC) != public_import) target->flags ^= STRUCT_PUBLIC;
        target->flags |= STRUCT_IMPORTED;

        for(size_t i = 0; i != target->methods.size(); i++){
            target->methods[i].parent_struct_offset = structures.size();
        }
    }

    // Merge Externals
    for(const External& new_external : other.externs){
        if(!(new_external.flags & EXTERN_PUBLIC)) continue;
        if(this->already_imported(new_external.origin)) continue;
        std::string new_external_final_name = (new_external.flags & EXTERN_MANGLED) ? mangle(new_external.name, new_external.arguments) : new_external.name;

        for(const External& external : externs){
            if( new_external_final_name == (external.flags & EXTERN_MANGLED ? mangle(external.name, external.arguments) : external.name) ){
                errors.panic(DUPLICATE_FUNC(new_external.name));
            }
        }

        External target = new_external;
        if((target.flags & EXTERN_PUBLIC) != public_import) target.flags ^= EXTERN_PUBLIC;
        externs.push_back(target);
    }

    // Merge Extra Libraries
    // TODO: SPEED: This is very slow
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
        if(!(new_global.flags & GLOBAL_PUBLIC)) continue;
        if(this->already_imported(new_global.origin)) continue;

        for(const Global& global : globals){
            if(new_global.name == global.name){
                errors.panic("Imported global variable '" + new_global.name + "' has the same name as a local global variable");
                return 1;
            }
        }

        Global cloned_global = new_global;
        cloned_global.flags |= GLOBAL_IMPORTED;
        if((cloned_global.flags & GLOBAL_PUBLIC) != public_import) cloned_global.flags ^= GLOBAL_PUBLIC;
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

        for(const Struct& structure : structures){
            if(new_type_alias.alias == structure.name){
                errors.panic("Imported type alias '" + new_type_alias.alias + "' has the same name as a local struct");
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
    // TODO: SPEED: This is very slow
    for(const Enum& new_enum : other.enums){
        if(!new_enum.is_public) continue;
        if(this->already_imported(new_enum.origin)) continue;

        for(const Struct& structure : structures){
            if(new_enum.name == structure.name){
                errors.panic("Imported enum '" + new_enum.name + "' has the same name as a local struct");
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

            std::vector<std::string> visited_aliases;
            size_t recursion_depth = 0;

            while( this->resolve_once_if_alias(type) ){
                // Resolve until there is nothing left to resolve

                if(++recursion_depth > 128){
                    if(std::find(visited_aliases.begin(), visited_aliases.end(), type) != visited_aliases.end()){
                        fail("Encountered recursive type alias '" + type + "'");
                        for(size_t p = 0; p != pointer_count; p++) type = "*" + type;
                        return false;
                    }

                    visited_aliases.push_back(type);
                }
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

int Program::extract_function_pointer_info(const std::string& type_name, std::vector<llvm::Type*>& llvm_args, AssemblyData& context, llvm::Type** return_llvm_type,
                                           std::vector<std::string>& typenames, std::string& return_typename, std::vector<std::string>* extra_return_types, char& flags) const {
    // "def(int, int) int"
    // -----^

    // NOTE: type_name is assumed to be a valid function pointer type

    // NOTE: Will set return_llvm_type to NULL if the function pointer type returns multiple values.
    //           The return types can then be extracted using Program::extract_multireturn_info on return_typename

    ensure(return_llvm_type != NULL);

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
        } else if(type_name[c] != ')') {
            std::cerr << SUICIDE << std::endl;
            std::cerr << "    Reason: Program::extract_function_pointer_info received an invalid type '" << type_name << "'" << std::endl;
            return 1;
        }

        if(arg_type_string.size() > 3 && arg_type_string.substr(0, 3) == "..."){
            arg_type_string = "[]" + arg_type_string.substr(3, arg_type_string.size()-3);
            flags |= FUNC_VARARGS;
        }

        if(this->find_type(arg_type_string, context, &arg_llvm_type) != 0){
            fail(UNDECLARED_TYPE(arg_type_string));
            return 1;
        }

        typenames.push_back(arg_type_string);
        llvm_args.push_back(arg_llvm_type);
    }

    next_index(c, type_name.length());

    if(type_name[c] == '('){
        size_t parentheses_depth = 0;
        next_index(c, type_name.length());

        while(type_name[c] != ')'){
            ensure(extra_return_types != NULL);
            *return_llvm_type = NULL;

            std::string return_type = "";
            while(parentheses_depth != 0 or (type_name[c] != ',' and type_name[c] != ')')){
                if(type_name[c] == '(') parentheses_depth++;
                if(type_name[c] == ')') parentheses_depth--;
                return_type += type_name[c];
                next_index(c, type_name.length());
            }

            if(type_name[c] == ','){
                next_index(c, type_name.length());
                if(type_name[c] == ' '){
                    next_index(c, type_name.length());
                }
            }

            extra_return_types->push_back(return_type);
        }
        return 0;
    } else {
        next_index(c, type_name.length());
        return_typename = type_name.substr(c, type_name.length()-c);
        ensure(return_typename != "");

        if(this->find_type(return_typename, context, return_llvm_type) != 0){
            fail(UNDECLARED_TYPE(return_typename));
            return 1;
        }

        return 0;
    }
}
int Program::function_typename_to_type(const std::string& type_name, AssemblyData& context, llvm::Type** type) const {
    // "def(int, int) int"
    // -----^

    ensure(type != NULL);

    std::vector<llvm::Type*> llvm_args;
    std::vector<llvm::Type*> return_llvm_types;
    llvm::Type* return_llvm_type;

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
        if(arg_type_string.size() > 3 && arg_type_string.substr(0, 3) == "..."){
            arg_type_string = "[]" + arg_type_string.substr(3, arg_type_string.size()-3);
        }
        if(this->find_type(arg_type_string, context, &arg_llvm_type) != 0){
            fail(UNDECLARED_TYPE(arg_type_string));
            return 1;
        }

        llvm_args.push_back(arg_llvm_type);
    }

    next_index(c, type_name.length());

    if(type_name[c] == '('){
        size_t parentheses_depth = 0;
        next_index(c, type_name.length());

        while(type_name[c] != ')'){
            std::string return_type = "";
            while(parentheses_depth != 0 or (type_name[c] != ',' and type_name[c] != ')')){
                if(type_name[c] == '(') parentheses_depth++;
                if(type_name[c] == ')') parentheses_depth--;
                return_type += type_name[c];
                next_index(c, type_name.length());
            }

            if(type_name[c] == ','){
                next_index(c, type_name.length());
                if(type_name[c] == ' '){
                    next_index(c, type_name.length());
                }
            }

            if(this->find_type(return_type, context, &return_llvm_type) != 0){
                fail(UNDECLARED_TYPE(return_type));
                return 1;
            }

            return_llvm_types.push_back(return_llvm_type->getPointerTo());
        }

        llvm_args.insert(llvm_args.end(), return_llvm_types.begin(), return_llvm_types.end());
        return_llvm_type = llvm::Type::getVoidTy(context.context);
        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, llvm_args, false);
        *type = function_type->getPointerTo();
        return 0;
    } else {
        next_index(c, type_name.length());
        std::string return_type_string = type_name.substr(c, type_name.length()-c);
        ensure(return_type_string != "");

        if(this->find_type(return_type_string, context, &return_llvm_type) != 0){
            fail(UNDECLARED_TYPE(return_type_string));
            return 1;
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(return_llvm_type, llvm_args, false);
        *type = function_type->getPointerTo();
        return 0;
    }
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
    type_aliases.push_back( TypeAlias("usize", "ulong", false, &origin_info) );
    type_aliases.push_back( TypeAlias("string", "[]ubyte", false, &origin_info) );
}
int Program::generate_types(AssemblyData& context){
    llvm::LLVMContext& llvm_context = context.context;
    std::vector<Type>& types = context.types;

    // NOTE: Requires types vector to be empty
    ensure(context.types.size() == 0);

    // Create a struct type for arrays
    std::vector<llvm::Type*> elements = { llvm::Type::getInt8PtrTy(llvm_context), llvm::Type::getInt64Ty(llvm_context) };
    llvm_array_type = llvm::StructType::create(elements, ".arr");

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

    size_t start_of_structs = types.size();

    // Create listing of all structures
    // TODO: SPEED: This is definitely not very efficient
    for(Struct& structure : structures){
        types.push_back( Type(structure.name, llvm::StructType::create(llvm_context, structure.name)) );
    }

    // Fill in llvm structures from struct data
    for(size_t i = 0; i != structures.size(); i++){
        Struct& structure = structures[i];
        llvm::Type* member_type;
        std::vector<llvm::Type*> members;

        for(StructField& member : structure.members){
            if(this->find_type(member.type, context, &member_type) != 0) {
                fail_filename(origin_info.filename, "The type '" + member.type + "' does not exist");
                return 1;
            }
            members.push_back(member_type);
        }

        static_cast<llvm::StructType*>(types[start_of_structs + i].type)->setBody(members, bool(structure.flags & STRUCT_PACKED));
    }

    // Create a constructor for arrays
    std::vector<llvm::Type*> array_ctor_args = { llvm::Type::getInt8PtrTy(llvm_context), llvm::Type::getInt64Ty(llvm_context) };
    llvm::FunctionType* array_ctor_type = llvm::FunctionType::get(llvm_array_type, array_ctor_args, false);
    llvm_array_ctor = llvm::Function::Create(array_ctor_type, llvm::Function::ExternalLinkage, ".__adept_core_arrctor", context.module.get());
    return 0;
}

int Program::find_type(const std::string& name, AssemblyData& context, llvm::Type** type) const {
    ensure(type != NULL);

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

    for(Type& t : types){
        if(t.name == type_name){
            if(type != NULL){
                *type = t.type;
                this->apply_type_modifiers(type, type_modifiers);
            }
            return 0;
        }
    }

    return 1;
}
int Program::find_func(const std::string& name, const std::vector<std::string>& args, External* func, bool require_va){
    bool args_match;

    // Check for any matching function
    for(size_t i = 0; i != functions.size(); i++){
        if(functions[i].name == name){
            External external;
            Function* function_found = &functions[i];

            if(require_va and !(function_found->flags & FUNC_VARARGS)) continue;
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
            external.extra_return_types = function_found->extra_return_types;

            char new_flags = 0x00;
            if(function_found->flags & FUNC_PUBLIC)      new_flags |= EXTERN_PUBLIC;
            if(!(function_found->flags & FUNC_EXTERNAL)) new_flags |= EXTERN_MANGLED;
            if(function_found->flags & FUNC_STDCALL)     new_flags |= EXTERN_STDCALL;
            if(function_found->flags & FUNC_VARARGS)     new_flags |= EXTERN_VARARGS;
            if(function_found->flags & FUNC_MULRET)      new_flags |= EXTERN_MULRET;

            external.flags = new_flags;
            for(Field& field : function_found->arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    // Check for any matching external
    for(size_t i = 0; i != externs.size(); i++){
        if(externs[i].name == name){
            External* external = &externs[i];

            if(require_va and !(external->flags & EXTERN_VARARGS)) continue;
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
            if(!(function_found->flags & FUNC_VARARGS)) continue;

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
            external.extra_return_types = function_found->extra_return_types;

            char new_flags = 0x00;
            if(function_found->flags & FUNC_PUBLIC)      new_flags |= EXTERN_PUBLIC;
            if(!(function_found->flags & FUNC_EXTERNAL)) new_flags |= EXTERN_MANGLED;
            if(function_found->flags & FUNC_STDCALL)     new_flags |= EXTERN_STDCALL;
            if(function_found->flags & FUNC_VARARGS)     new_flags |= EXTERN_VARARGS;
            if(function_found->flags & FUNC_MULRET)      new_flags |= EXTERN_MULRET;

            external.flags = new_flags;
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
            if(!(external->flags & EXTERN_VARARGS)) continue;

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
int Program::find_method(const std::string& struct_name, const std::string& name, const std::vector<std::string>& args, External* func){
    Struct structure;
    bool args_match;

    if(this->find_struct(struct_name, &structure) != 0){
        fail( UNDECLARED_STRUCT(struct_name) );
        return 1;
    }

    std::vector<Function>& struct_methods = structure.methods;

    for(size_t i = 0; i != struct_methods.size(); i++){
        if(struct_methods[i].name == name){
            External external;
            Function* found_method = &struct_methods[i];

            if(found_method->flags & FUNC_VARARGS) continue; // Perfer non-var-args methods
            if(args.size() != found_method->arguments.size()) continue;

            args_match = true;
            for(size_t a = 0; a != args.size(); a++){
                if( !assemble_types_mergeable(*this, args[a], found_method->arguments[a].type) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            external.name = found_method->name;
            external.return_type = found_method->return_type;

            char new_flags = EXTERN_MANGLED;
            if(found_method->flags & FUNC_PUBLIC)      new_flags |= EXTERN_PUBLIC;
            if(found_method->flags & FUNC_STDCALL)     new_flags |= EXTERN_STDCALL;
            if(found_method->flags & FUNC_VARARGS)     new_flags |= EXTERN_VARARGS;

            external.flags = new_flags;
            for(Field& field : found_method->arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    for(size_t i = 0; i != struct_methods.size(); i++){
        if(struct_methods[i].name == name){
            External external;
            Function* found_method = &struct_methods[i];

            if(!(found_method->flags & FUNC_VARARGS)) continue; // We already checked for matching non-var-args methods

            size_t a;
            size_t last_arg = found_method->arguments.size() - 1;
            args_match = true;

            for(a = 0; a != last_arg; a++){
                if( !assemble_types_mergeable(*this, args[a], found_method->arguments[a].type) ){
                    args_match = false;
                    break;
                }
            }
            if(!args_match) continue;

            std::string target_va_type = found_method->arguments[last_arg].type;
            target_va_type = target_va_type.substr(2, target_va_type.length() - 2);

            while(a != args.size()){
                if( !assemble_types_mergeable(*this, args[a], target_va_type) ){
                    args_match = false;
                    break;
                }
                a++;
            }
            if(!args_match) continue;

            external.name = found_method->name;
            external.return_type = found_method->return_type;

            char new_flags = EXTERN_MANGLED;
            if(found_method->flags & FUNC_PUBLIC)  new_flags |= EXTERN_PUBLIC;
            if(found_method->flags & FUNC_STDCALL) new_flags |= EXTERN_STDCALL;
            if(found_method->flags & FUNC_VARARGS) new_flags |= EXTERN_VARARGS;

            external.flags = new_flags;
            for(Field& field : found_method->arguments) external.arguments.push_back(field.type);

            *func = external;
            return 0;
        }
    }

    return 1;
}
int Program::find_struct(const std::string& name, Struct* structure){
    for(size_t i = 0; i != structures.size(); i++){
        if(structures[i].name == name){
            if(structure != NULL) *structure = structures[i];
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
    print_structs();
    print_globals();
    print_functions();
    std::cout << std::endl;
}
void Program::print_functions(){
    for(Function& f : functions){
        std::cout << (f.flags & FUNC_PUBLIC ? "public " : "private ");
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
        std::cout << (e.flags & EXTERN_PUBLIC ? "public " : "private ");
        if(e.flags & EXTERN_STDCALL) std::cout << "stdcall ";
        std::cout << "foreign " << e.name << "(";
        for(size_t a = 0; a != e.arguments.size(); a++){
            std::cout << e.arguments[a];
            if(a + 1 != e.arguments.size()) std::cout << ", ";
        }
        std::cout << ") " << e.return_type << std::endl;
    }
}
void Program::print_structs(){
    for(Struct& structure : structures){
        std::cout << (structure.flags & STRUCT_PUBLIC ? "public " : "private ");
        std::cout << "struct " << structure.name << " {" << std::endl;

        for(StructField& f : structure.members){
            std::cout << "    ";
            std::cout << (f.flags & STRUCTFIELD_PUBLIC ? "public " : "private ");
            if(f.flags & STRUCTFIELD_STATIC) std::cout << "static ";
            std::cout << f.name << " " << f.type << std::endl;
        }

        for(Function& m : structure.methods){
            std::cout << "    ";
            std::cout << (m.flags & FUNC_PUBLIC ? "public " : "private ");
            if(m.flags & FUNC_STATIC) std::cout << "static ";
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
        std::cout << (global.flags & GLOBAL_PUBLIC ? "public " : "private ") << global.name << " " << global.type << std::endl;
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
