
#ifndef STATEMENT_H_INCLUDED
#define STATEMENT_H_INCLUDED

#include <string>
#include <vector>
#include <stdint.h>
#include "tokens.h"
#include "errors.h"
#include "expression.h"

// Only used to indicate statement types
#define STATEMENTID_NONE         0
#define STATEMENTID_DECLARE      1
#define STATEMENTID_DECLAREAS    2
#define STATEMENTID_RETURN       3
#define STATEMENTID_ASSIGN       4
#define STATEMENTID_CALL         5
#define STATEMENTID_IF           6
#define STATEMENTID_WHILE        7
#define STATEMENTID_UNLESS       8
#define STATEMENTID_UNTIL        9
#define STATEMENTID_IFELSE       10
#define STATEMENTID_UNLESSELSE   11

class Statement;
typedef std::vector<Statement*> StatementList;

class Statement {
    public:
    ErrorHandler errors;

    Statement();
    Statement(ErrorHandler&);
    Statement(const Statement&);
    virtual ~Statement();
    virtual int assemble(Program&, Function&, AssembleContext&) = 0;
    virtual std::string toString(unsigned int indent, bool skip_initial_indent) = 0;
    virtual Statement* clone() = 0;
    virtual bool isTerminator() = 0;
    virtual bool isConditional();
};

class DeclareStatement : public Statement {
    public:
    std::string variable_name;
    std::string variable_type;

    DeclareStatement(ErrorHandler&);
    DeclareStatement(const std::string&, const std::string&, ErrorHandler&);
    DeclareStatement(const DeclareStatement&);
    ~DeclareStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class DeclareAssignStatement : public Statement {
    public:
    std::string variable_name;
    std::string variable_type;
    PlainExp* variable_value;

    DeclareAssignStatement(ErrorHandler&);
    DeclareAssignStatement(const std::string&, const std::string&, PlainExp*, ErrorHandler&);
    DeclareAssignStatement(const DeclareAssignStatement&);
    ~DeclareAssignStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class ReturnStatement : public Statement {
    public:
    PlainExp* return_value;

    ReturnStatement(ErrorHandler&);
    ReturnStatement(PlainExp*, ErrorHandler&);
    ReturnStatement(const ReturnStatement&);
    ~ReturnStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class AssignStatement : public Statement {
    public:
    PlainExp* location;
    PlainExp* value;

    AssignStatement(ErrorHandler&);
    AssignStatement(PlainExp*, PlainExp*, ErrorHandler&);
    AssignStatement(const AssignStatement&);
    ~AssignStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class CallStatement : public Statement {
    public:
    std::string name;
    std::vector<PlainExp*> args;

    CallStatement(ErrorHandler&);
    CallStatement(const std::string&, const std::vector<PlainExp*>&, ErrorHandler&);
    CallStatement(const CallStatement&);
    ~CallStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class MemberCallStatement : public Statement {
    public:
    PlainExp* object;
    std::string name;
    std::vector<PlainExp*> args;

    MemberCallStatement(ErrorHandler&);
    MemberCallStatement(PlainExp*, const std::string&, const std::vector<PlainExp*>&, ErrorHandler&);
    MemberCallStatement(const MemberCallStatement&);
    ~MemberCallStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class IfStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;

    IfStatement(ErrorHandler&);
    IfStatement(PlainExp*, const StatementList&, ErrorHandler&);
    IfStatement(const IfStatement&);
    ~IfStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class UnlessStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;

    UnlessStatement(ErrorHandler&);
    UnlessStatement(PlainExp*, const StatementList&, ErrorHandler&);
    UnlessStatement(const UnlessStatement&);
    ~UnlessStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class WhileStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;

    WhileStatement(ErrorHandler&);
    WhileStatement(PlainExp*, const StatementList&, ErrorHandler&);
    WhileStatement(const WhileStatement&);
    ~WhileStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class UntilStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;

    UntilStatement(ErrorHandler&);
    UntilStatement(PlainExp*, const StatementList&, ErrorHandler&);
    UntilStatement(const UntilStatement&);
    ~UntilStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class IfElseStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;
    StatementList negative_statements;

    IfElseStatement(ErrorHandler&);
    IfElseStatement(PlainExp*, const StatementList&, const StatementList&, ErrorHandler&);
    IfElseStatement(const IfElseStatement&);
    ~IfElseStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class UnlessElseStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;
    StatementList negative_statements;

    UnlessElseStatement(ErrorHandler&);
    UnlessElseStatement(PlainExp*, const StatementList&, const StatementList&, ErrorHandler&);
    UnlessElseStatement(const UnlessElseStatement&);
    ~UnlessElseStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class DeallocStatement : public Statement {
    public:
    PlainExp* value;

    DeallocStatement(ErrorHandler&);
    DeallocStatement(PlainExp*, ErrorHandler&);
    DeallocStatement(const DeallocStatement&);
    ~DeallocStatement();
    int assemble(Program&, Function&, AssembleContext&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

#endif // STATEMENT_H_INCLUDED
