
#ifndef STATEMENT_H_INCLUDED
#define STATEMENT_H_INCLUDED

#define STATEMENT_NONE(E)               Statement(0, E)
#define STATEMENT_DECLARE(a,b,E)        Statement(1, new DeclareStatement(a, b), E)
#define STATEMENT_DECLAREAS(a,b,c,E)    Statement(2, new DeclareAsStatement(a, b, c),E)
#define STATEMENT_RETURN(a,E)           Statement(3, new ReturnStatement(a), E)
#define STATEMENT_ASSIGN(a,b,c,d,E)     Statement(4, new AssignStatement(a, b, c, d), E)
#define STATEMENT_ASSIGNMEMBER(a,b,c,E) Statement(5, new AssignMemberStatement(a, b, c), E)
#define STATEMENT_CALL(a,b,E)           Statement(6, new CallStatement(a, b), E)
#define STATEMENT_IF(a,b,E)             Statement(7, new ConditionalStatement(a, b), E)
#define STATEMENT_WHILE(a,b,E)          Statement(8, new ConditionalStatement(a, b), E)
#define STATEMENT_UNLESS(a,b,E)         Statement(9, new ConditionalStatement(a, b), E)
#define STATEMENT_UNTIL(a,b,E)          Statement(10, new ConditionalStatement(a, b), E)
#define STATEMENT_IFELSE(a,b,c,E)       Statement(11, new SplitConditionalStatement(a, b, c), E)
#define STATEMENT_UNLESSELSE(a,b,c,E)   Statement(12, new SplitConditionalStatement(a, b, c), E)

#define STATEMENTID_NONE         0
#define STATEMENTID_DECLARE      1
#define STATEMENTID_DECLAREAS    2
#define STATEMENTID_RETURN       3
#define STATEMENTID_ASSIGN       4
#define STATEMENTID_ASSIGNMEMBER 5
#define STATEMENTID_CALL         6
#define STATEMENTID_IF           7
#define STATEMENTID_WHILE        8
#define STATEMENTID_UNLESS       9
#define STATEMENTID_UNTIL        10
#define STATEMENTID_IFELSE       11
#define STATEMENTID_UNLESSELSE   12

#include <string>
#include <vector>
#include <stdint.h>
#include "tokens.h"
#include "errors.h"
#include "expression.h"

// Main Statement Structure
struct Statement {
    uint16_t id;
    void* data;
    ErrorHandler errors;

    Statement();
    Statement(const Statement&);
    Statement(uint16_t, ErrorHandler&);
    Statement(uint16_t, void*, ErrorHandler&);
    ~Statement();
    void reset();
    void free();
    std::string toString(unsigned int indent = 0, bool skip_initial_indent = false);
};

// Helper Structures
struct AssignMemberPathNode { std::string name; std::vector<PlainExp*> gep_loads; };
typedef std::vector<AssignMemberPathNode> AssignMemberPath;
typedef std::vector<Statement> StatementList;

// Possible structures pointed to by 'void* Statement::data'...

struct DeclareStatement {
    std::string name;
    std::string type;

    DeclareStatement(std::string, std::string);
};

struct DeclareAsStatement {
    std::string name;
    std::string type;
    PlainExp* value;

    DeclareAsStatement(const DeclareAsStatement&);
    DeclareAsStatement(std::string, std::string, PlainExp*);
    ~DeclareAsStatement();
};

struct ReturnStatement {
    PlainExp* value;

    ReturnStatement(const ReturnStatement&);
    ReturnStatement(PlainExp*);
    ~ReturnStatement();
};

struct AssignStatement {
    std::string name;
    PlainExp* value;
    int loads; // For '*'
    std::vector<PlainExp*> gep_loads; // For '[]'

    AssignStatement(const AssignStatement&);
    AssignStatement(std::string, PlainExp*, int);
    AssignStatement(std::string, PlainExp*, int, const std::vector<PlainExp*>&);
    ~AssignStatement();
};

struct AssignMemberStatement {
    std::vector<AssignMemberPathNode> path;
    PlainExp* value;
    int loads; // For '*'

    AssignMemberStatement(const AssignMemberStatement&);
    AssignMemberStatement(const std::vector<std::string>&, PlainExp*, int);
    AssignMemberStatement(const std::vector<AssignMemberPathNode>&, PlainExp*, int);
    ~AssignMemberStatement();
};

struct CallStatement {
    std::string name;
    std::vector<PlainExp*> args;

    CallStatement(const CallStatement&);
    CallStatement(std::string, const std::vector<PlainExp*>&);
    ~CallStatement();
};

struct ConditionalStatement {
    PlainExp* condition;
    std::vector<Statement> statements;

    ConditionalStatement(const ConditionalStatement&);
    ConditionalStatement(PlainExp*, const std::vector<Statement>&);
    ~ConditionalStatement();
};

struct SplitConditionalStatement {
    PlainExp* condition;
    std::vector<Statement> true_statements;
    std::vector<Statement> false_statements;

    SplitConditionalStatement(const SplitConditionalStatement&);
    SplitConditionalStatement(PlainExp*, const std::vector<Statement>&, const std::vector<Statement>&);
    ~SplitConditionalStatement();
};

#endif // STATEMENT_H_INCLUDED
