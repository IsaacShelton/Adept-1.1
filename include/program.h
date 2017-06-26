
#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include "type.h"
#include "config.h"
#include "statement.h"
#include "asmdata.h"
#include "asmtypes.h"
class CacheManager; // "cache.h" included at end of file

struct Field;
struct ClassField;
class Structure;
class Function;
class External;
class ModuleDependency;
class Constant;
class Class;
class TypeAlias;
class Program;

struct Field {
    std::string name;
    std::string type;
};

struct ClassField {
    std::string name;
    std::string type;
    bool is_public;
    bool is_static;
};

struct OriginInfo {
    std::string filename;
};

class Structure {
    public:
    std::string name;
    std::vector<Field> members;
    bool is_public;
    bool is_packed;

    OriginInfo* origin;

    Structure();
    Structure(const std::string&, const std::vector<Field>&, bool, bool, OriginInfo*);
    int find_index(std::string name, int* index);
};

class Function {
    // NOTE: This class is also used for methods

    public:
    std::string name;
    std::vector<Field> arguments;
    std::string return_type;
    StatementList statements;
    bool is_public;
    bool is_static;
    bool is_stdcall;
    size_t parent_class_offset; // Offset from program.classes + 1 (beacuse 0 is used for none)

    OriginInfo* origin;

    Function();
    Function(const std::string&, const std::vector<Field>&, const std::string&, bool, OriginInfo*);
    Function(const std::string&, const std::vector<Field>&, const std::string&, const StatementList&, bool, OriginInfo*);
    Function(const std::string&, const std::vector<Field>&, const std::string&, const StatementList&, bool, bool, bool, OriginInfo*);
    Function(const Function&);
    ~Function();
    void print_statements();
};

class External {
    public:
    std::string name;
    std::vector<std::string> arguments;
    std::string return_type;
    bool is_public;
    bool is_mangled;
    bool is_stdcall;

    OriginInfo* origin;

    External();
    External(const std::string&, const std::vector<std::string>&, const std::string&, bool, bool, bool, OriginInfo*);
    std::string toString();
};

class ModuleDependency {
    public:
    std::string filename;
    std::string target_bc;
    std::string target_obj;
    Configuration* config;
    bool is_nothing;

    ModuleDependency(const std::string&, const std::string&, const std::string&, Configuration*, bool);
};

class ImportDependency {
    public:
    std::string filename;
    bool is_public;

    ImportDependency(const std::string&, bool);
};

class Constant {
    public:
    std::string name;
    PlainExp* value;
    bool is_public;

    OriginInfo* origin;

    Constant();
    Constant(const std::string&, PlainExp*, bool, OriginInfo*);
};

class Global {
    public:
    std::string name;
    std::string type;
    bool is_public;
    bool is_imported;
    ErrorHandler errors;

    OriginInfo* origin;

    Global();
    Global(const std::string&, const std::string&, bool, ErrorHandler&, OriginInfo*);
};

class Class {
    public:
    std::string name;
    std::vector<ClassField> members;
    std::vector<Function> methods;
    bool is_public;
    bool is_imported;

    OriginInfo* origin;

    Class();
    Class(const std::string&, bool, bool, OriginInfo*);
    Class(const std::string&, const std::vector<ClassField>&, bool, OriginInfo*);
    int find_index(std::string name, int* index);
};

class TypeAlias {
    public:
    std::string alias;
    std::string binding;
    bool is_public;

    OriginInfo* origin;

    TypeAlias();
    TypeAlias(const std::string&, const std::string&, bool, OriginInfo*);
};

class Program {
    public:
    CacheManager* parent_manager;
    std::vector<ModuleDependency> dependencies;
    std::vector<ImportDependency> imports;
    std::vector<std::string> extra_libs;
    std::vector<OriginInfo> origins;
    OriginInfo origin_info;

    std::vector<Function> functions;
    std::vector<Structure> structures;
    std::vector<Class> classes;
    std::vector<Constant> constants;
    std::vector<External> externs;
    std::vector<Global> globals;
    std::vector<TypeAlias> type_aliases;

    llvm::Type* llvm_array_type; // Used for arrays as well as strings
    llvm::Function* llvm_array_ctor;

    enum TypeModifier : unsigned char {
        Pointer = 0,
        Array = 1,
    };

    inline static bool is_function_typename(const std::string&);
    inline static bool is_pointer_typename(const std::string&);
    inline static bool is_array_typename(const std::string&);
    inline static bool is_integer_typename(const std::string&);
    inline static bool function_typename_is_stdcall(const std::string&);

    Program(CacheManager*, const std::string&);
    ~Program();
    int import_merge(Configuration*, Program&, bool, ErrorHandler& errors);

    bool resolve_if_alias(std::string&) const;
    bool resolve_once_if_alias(std::string&) const;

    int extract_function_pointer_info(const std::string&, std::vector<llvm::Type*>&, AssemblyData&, llvm::Type**) const;
    int extract_function_pointer_info(const std::string&, std::vector<llvm::Type*>&, AssemblyData&, llvm::Type**, std::vector<std::string>&, std::string&) const;
    int function_typename_to_type(const std::string&, AssemblyData&, llvm::Type**) const;
    void apply_type_modifiers(llvm::Type**, const std::vector<Program::TypeModifier>&) const;

    bool already_imported(OriginInfo*);
    void generate_type_aliases();
    int generate_types(AssemblyData&);

    int find_type(const std::string&, AssemblyData&, llvm::Type**) const;
    int find_func(const std::string&, External*);
    int find_func(const std::string&, const std::vector<std::string>&, External*);
    int find_method(const std::string&, const std::string&, const std::vector<std::string>&, External*);
    int find_struct(const std::string&, Structure*);
    int find_class(const std::string&, Class*);
    int find_const(const std::string&, Constant*);
    int find_global(const std::string&, Global*);

    void print();
    void print_types();
    void print_functions();
    void print_externals();
    void print_structures();
    void print_classes();
    void print_globals();
};

inline bool Program::is_function_typename(const std::string& type_name){
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
inline bool Program::is_pointer_typename(const std::string& type_name){
    // "**uint" -> true
    // "long"   -> false

    if(type_name.length() < 1) return false;
    if(type_name[0] == '*') return true;
    if(type_name == "ptr") return true;
    return false;
}
inline bool Program::is_array_typename(const std::string& type_name){
    // "[]int"  -> true
    // "**uint" -> false

    if(type_name.length() < 2) return false;
    if(type_name[0] == '[' and type_name[1] == ']') return true;
    return false;
}
inline bool Program::is_integer_typename(const std::string& type_name){
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
inline bool Program::function_typename_is_stdcall(const std::string& type_name){
    // Function Pointer Type Layout ( [] = optional ):
    // "[stdcall] def(type, type, type) type"

    if(type_name.length() < 8) return false;
    if(type_name.substr(0, 8) == "stdcall "){
        return true;
    }

    return false;
}

#include "cache.h"

#endif // PROGRAM_H_INCLUDED
