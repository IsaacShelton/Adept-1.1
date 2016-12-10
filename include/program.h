
#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include "type.h"
#include "config.h"
#include "statement.h"
#include "asmcontext.h"

struct Variable {
    std::string name;
    std::string type;
    llvm::Value* variable;
};

struct Field {
    std::string name;
    std::string type;
};

struct Structure {
    std::string name;
    std::vector<Field> members;
    bool is_public;

    int find_index(std::string name, int* index);
};

struct Function {
    std::string name;
    std::vector<Field> arguments;
    std::string return_type;
    std::vector<Statement> statements;
    bool is_public;

    std::vector<Variable> variables;
    AssembleFunction asm_func;

    int find_variable(std::string, Variable*);

    void print_statements();
};

struct External {
    std::string name;
    std::vector<std::string> arguments;
    std::string return_type;
    bool is_public;
};

struct ModuleDependency {
    std::string name;
    std::string target_bc;
    std::string target_obj;

    Program* program;
    Configuration* config;

    ModuleDependency(const ModuleDependency&);
    ModuleDependency(std::string, std::string, std::string, Program*, Configuration*);
};

struct Program {
    std::vector<ModuleDependency> imports;

    std::vector<Function> functions;
    std::vector<Structure> structures;
    std::vector<External> externs;
    std::vector<Type> types;

    int import_merge(const Program&);
    int generate_types(AssembleContext&);
    int find_type(std::string, llvm::Type**);
    int find_func(std::string, External*);
    int find_struct(std::string, Structure*);

    void print();
    void print_types();
    void print_functions();
    void print_externals();
    void print_structures();
};

#endif // PROGRAM_H_INCLUDED
