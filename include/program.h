
#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include "config.h"
#include "statement.h"
#include "asmdata.h"
#include "asmtypes.h"
class CacheManager; // "cache.h" included at end of file

struct Field;
struct StructField;
class Function;
class External;
class ModuleDependency;
class Constant;
class Struct;
class TypeAlias;
class Program;

struct Field {
    std::string name;
    std::string type;
};

#define STRUCTFIELD_PUBLIC 0x01
#define STRUCTFIELD_STATIC 0x02

struct StructField {
    std::string name;
    std::string type;
    char flags;
};

struct OriginInfo {
    std::string filename;
};

#define FUNC_PUBLIC   0x01
#define FUNC_STATIC   0x02
#define FUNC_STDCALL  0x04
#define FUNC_EXTERNAL 0x08
#define FUNC_VARARGS  0x10
#define FUNC_MULRET   0x20

class Function {
    // NOTE: This class is also used for methods

    public:
    std::string name;
    std::vector<Field> arguments;
    std::string return_type;
    StatementList statements;
    char flags;
    size_t parent_struct_offset; // Offset from program.structures + 1 (beacuse 0 is used for none)
    std::vector<std::string> extra_return_types;

    OriginInfo* origin;

    Function();
    Function(const std::string&, const std::vector<Field>&, const std::string&, bool, OriginInfo*);
    Function(const std::string&, const std::vector<Field>&, const std::string&, const StatementList&, bool, bool, bool, bool, bool, OriginInfo*);
    Function(const std::string&, const std::vector<Field>&, const std::vector<std::string>&, const StatementList&, bool, bool, bool, bool, bool, OriginInfo*);
    Function(const Function&);
    ~Function();
    void print_statements();
};

#define EXTERN_PUBLIC  0x01
#define EXTERN_MANGLED 0x02
#define EXTERN_STDCALL 0x04
#define EXTERN_VARARGS 0x08
#define EXTERN_MULRET  0x10

class External {
    public:
    std::string name;
    std::vector<std::string> arguments;
    std::string return_type;
    char flags;
    std::vector<std::string> extra_return_types;

    OriginInfo* origin;

    External();
    External(const std::string&, const std::vector<std::string>&, const std::string&, bool, bool, bool, bool, OriginInfo*);
    std::string toString();
};

class ModuleDependency {
    public:
    std::string filename;
    std::string target_bc;
    std::string target_obj;
    Config* config;
    bool is_nothing;

    ModuleDependency(const std::string&, const std::string&, const std::string&, Config*, bool);
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

#define GLOBAL_PUBLIC   0x01
#define GLOBAL_IMPORTED 0x02
#define GLOBAL_EXTERNAL 0x04

class Global {
    public:
    std::string name;
    std::string type;
    char flags;
    ErrorHandler errors;

    OriginInfo* origin;

    Global();
    Global(const std::string&, const std::string&, bool, bool, ErrorHandler&, OriginInfo*);
};

#define STRUCT_PUBLIC   0x01
#define STRUCT_IMPORTED 0x02
#define STRUCT_PACKED   0x04

class Struct {
    public:
    std::string name;
    std::vector<StructField> members;
    std::vector<Function> methods;
    char flags;

    OriginInfo* origin;

    Struct();
    Struct(const std::string&, bool, bool, OriginInfo*);
    Struct(const std::string&, const std::vector<StructField>&, bool, OriginInfo*);
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

class EnumField {
    public:
    std::string name;
    PlainExp* value;
    ErrorHandler errors;

    EnumField(const std::string&, ErrorHandler&);
    EnumField(const std::string&, PlainExp*, ErrorHandler&);
    EnumField(const EnumField&);
    ~EnumField();
};

class Enum {
    public:
    std::string name;
    std::vector<EnumField> fields;
    bool is_public;
    size_t bits;

    OriginInfo* origin;

    Enum(const std::string&, std::vector<EnumField>&, bool, OriginInfo*);
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
    std::vector<Struct> structures;
    std::vector<Constant> constants;
    std::vector<External> externs;
    std::vector<Global> globals;
    std::vector<TypeAlias> type_aliases;
    std::vector<Enum> enums;

    llvm::Type* llvm_array_type; // Used for arrays as well as strings
    llvm::Function* llvm_array_ctor;

    enum TypeModifier : unsigned char {
        Pointer = 0,
        Array = 1,
        FixedArray = 2,
    };

    inline static bool is_function_typename(const std::string&);
    inline static bool is_pointer_typename(const std::string&);
    inline static bool is_array_typename(const std::string&);
    inline static bool is_integer_typename(const std::string&);
    inline static bool function_typename_is_stdcall(const std::string&);

    Program(CacheManager*, const std::string&);
    ~Program();
    int import_merge(Config*, Program&, bool, ErrorHandler& errors);

    bool resolve_if_alias(std::string&) const;
    bool resolve_once_if_alias(std::string&) const;

    int extract_function_pointer_info(const std::string&, std::vector<llvm::Type*>&, AssemblyData&, llvm::Type**,
                                      std::vector<std::string>&, std::string&, std::vector<std::string>*, char&) const;
    int function_typename_to_type(const std::string&, AssemblyData&, llvm::Type**) const;
    void apply_type_modifiers(llvm::Type**, const std::vector<Program::TypeModifier>&, const std::vector<int>& fixed_array_sizes) const;

    bool already_imported(OriginInfo*);
    void generate_type_aliases();
    int generate_types(AssemblyData&);

    int find_type(const std::string&, AssemblyData&, llvm::Type**) const;
    int find_func(const std::string&, const std::vector<std::string>&, External*, bool require_va = false);
    int find_method(const std::string&, const std::string&, const std::vector<std::string>&, External*);
    int find_struct(const std::string&, Struct*);
    int find_const(const std::string&, Constant*);
    int find_global(const std::string&, Global*);

    void print();
    void print_types();
    void print_functions();
    void print_externals();
    void print_structs();
    void print_globals();
    void print_enums();
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
    if(type_name.length() == 0) return false;

    if(type_name[0] == 'u' and (type_name == "uint" or type_name == "ulong" or type_name == "ushort" or type_name == "ubyte")){
        return true;
    }

    if(type_name == "int" or type_name == "long" or type_name == "short" or type_name == "byte"){
        return true;
    }

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
