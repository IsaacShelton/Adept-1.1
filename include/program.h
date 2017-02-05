
#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include "type.h"
#include "config.h"
#include "statement.h"
#include "asmcontext.h"

struct Variable;
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

struct Variable {
    std::string name;
    std::string type;
    llvm::Value* variable;
};

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

class Structure {
    public:
    std::string name;
    std::vector<Field> members;
    bool is_public;
    bool is_packed;

    int find_index(std::string name, int* index);
};

class Function {
    public:
    std::string name;
    std::vector<Field> arguments;
    std::string return_type;
    StatementList statements;
    bool is_public;
    bool is_static;
    Class* parent_class;

    std::vector<Variable> variables;
    AssembleFunction asm_func;

    Function(const std::string&, const std::vector<Field>&, const std::string&, const StatementList&, bool);
    Function(const std::string&, const std::vector<Field>&, const std::string&, const StatementList&, bool, bool);
    Function(const Function&);
    ~Function();
    int find_variable(std::string, Variable*);
    void print_statements();
};

class External {
    public:
    std::string name;
    std::vector<std::string> arguments;
    std::string return_type;
    bool is_public;
    bool is_mangled;

    std::string toString();

};

class ModuleDependency {
    public:
    std::string name;
    std::string target_bc;
    std::string target_obj;

    Program* program;
    Configuration* config;

    ModuleDependency(std::string, std::string, std::string, Program*, Configuration*);
};

class Constant {
    public:
    std::string name;
    PlainExp* value;
    bool is_public;

    Constant();
    Constant(const std::string&, PlainExp*, bool);
};

class Global {
    public:
    std::string name;
    std::string type;
    bool is_public;
    llvm::Value* variable;
    ErrorHandler errors;

    Global();
    Global(const std::string&, const std::string&, bool, ErrorHandler&);
};

class Class {
    public:
    std::string name;
    std::vector<ClassField> members;
    std::vector<Function> methods;
    bool is_public;
    bool is_imported;

    Class();
    Class(const std::string&, const std::vector<ClassField>&, bool);
    int find_index(std::string name, int* index);
};

class TypeAlias {
    public:
    std::string alias;
    std::string binding;
    bool is_public;

    TypeAlias();
    TypeAlias(const std::string&, const std::string&, bool);
};

class Program {
    public:
    std::vector<ModuleDependency> imports;
    std::vector<std::string> extra_libs;

    std::vector<Function> functions;
    std::vector<Structure> structures;
    std::vector<Class> classes;
    std::vector<Constant> constants;
    std::vector<External> externs;
    std::vector<Type> types;
    std::vector<Global> globals;
    std::vector<TypeAlias> type_aliases;

    ~Program();
    int import_merge(Program&, bool);
    bool resolve_if_alias(std::string&);
    bool resolve_once_if_alias(std::string&);
    int extract_function_pointer_info(const std::string&, std::vector<llvm::Type*>&, llvm::Type**);
    int extract_function_pointer_info(const std::string&, std::vector<llvm::Type*>&, llvm::Type**, std::vector<std::string>&, std::string&);
    int function_typename_to_type(const std::string&, llvm::Type**);
    int generate_types(AssembleContext&);
    int find_type(const std::string&, llvm::Type**);
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

#endif // PROGRAM_H_INCLUDED
