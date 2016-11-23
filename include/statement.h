
#ifndef STATEMENT_H_INCLUDED
#define STATEMENT_H_INCLUDED

#define STATEMENT_NONE               Statement(0)
#define STATEMENT_DECLARE(a, b)      Statement(1, new DeclareStatement(a, b))
#define STATEMENT_DECLAREAS(a,b,c)   Statement(2, new DeclareAsStatement(a, b, c))
#define STATEMENT_RETURN(a)          Statement(3, new ReturnStatement(a))
#define STATEMENT_ASSIGN(a,b)        Statement(4, new AssignStatement(a, b))
#define STATEMENT_CALL(a,b)          Statement(5, new CallStatement(a, b))

#define STATEMENTID_NONE       0
#define STATEMENTID_DECLARE    1
#define STATEMENTID_DECLAREAS  2
#define STATEMENTID_RETURN     3
#define STATEMENTID_ASSIGN     4
#define STATEMENTID_CALL       5

#include <string>
#include <vector>
#include <stdint.h>
#include "tokens.h"
#include "expression.h"

struct Statement {
    uint16_t id;
    void* data;

    Statement();
    Statement(const Statement&);
    Statement(uint16_t);
    Statement(uint16_t, void*);
    ~Statement();
    void reset();
    void free();
    std::string toString();
};

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

    AssignStatement(const AssignStatement&);
    AssignStatement(std::string, PlainExp*);
    ~AssignStatement();
};

struct CallStatement {
    std::string name;
    std::vector<PlainExp*> args;

    CallStatement(const CallStatement&);
    CallStatement(std::string, std::vector<PlainExp*>);
    ~CallStatement();
};

#endif // STATEMENT_H_INCLUDED
