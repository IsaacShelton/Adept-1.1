
#ifndef STATEMENT_H_INCLUDED
#define STATEMENT_H_INCLUDED

#include <string>
#include <vector>
#include <stdint.h>
#include "tokens.h"
#include "errors.h"
#include "expression.h"

// Only used to indicate statement types
#define STATEMENTID_NONE            0
#define STATEMENTID_DECLARE         1
#define STATEMENTID_DECLAREAS       2
#define STATEMENTID_MULTIDECLARE    3
#define STATEMENTID_DECLAREAS       4
#define STATEMENTID_RETURN          5
#define STATEMENTID_ASSIGN          6
#define STATEMENTID_ASSIGNADD       7
#define STATEMENTID_ASSIGNSUB       8
#define STATEMENTID_ASSIGNMUL       9
#define STATEMENTID_ASSIGNDIV       10
#define STATEMENTID_ASSIGNMOD       11
#define STATEMENTID_CALL            12
#define STATEMENTID_IF              13
#define STATEMENTID_WHILE           14
#define STATEMENTID_UNLESS          15
#define STATEMENTID_UNTIL           16
#define STATEMENTID_IFELSE          17
#define STATEMENTID_UNLESSELSE      18
#define STATEMENTID_IFWHILEELSE     19
#define STATEMENTID_UNLESSUNTILELSE 20
#define STATEMENTID_DEALLOC         21
#define STATEMENTID_SWITCH          22

class Statement;
typedef std::vector<Statement*> StatementList;

class Statement {
    public:
    ErrorHandler errors;

    Statement();
    Statement(ErrorHandler&);
    Statement(const Statement&);
    virtual ~Statement();
    virtual int assemble(Program&, Function&, AssemblyData&) = 0;
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class MultiDeclareStatement : public Statement {
    public:
    std::vector<std::string> variable_names;
    std::string variable_type; // Same for each variable

    MultiDeclareStatement(ErrorHandler&);
    MultiDeclareStatement(const std::vector<std::string>&, const std::string&, ErrorHandler&);
    MultiDeclareStatement(const MultiDeclareStatement&);
    ~MultiDeclareStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class MultiDeclareAssignStatement : public Statement {
    public:
    std::vector<std::string> variable_names;
    std::string variable_type; // Same for each variable
    PlainExp* variable_value; // Same for each variable

    MultiDeclareAssignStatement(ErrorHandler&);
    MultiDeclareAssignStatement(const std::vector<std::string>&, const std::string&, PlainExp*, ErrorHandler&);
    MultiDeclareAssignStatement(const MultiDeclareAssignStatement&);
    ~MultiDeclareAssignStatement();
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class AdditionAssignStatement : public Statement {
    public:
    PlainExp* location;
    PlainExp* value;

    AdditionAssignStatement(ErrorHandler&);
    AdditionAssignStatement(PlainExp*, PlainExp*, ErrorHandler&);
    AdditionAssignStatement(const AdditionAssignStatement&);
    ~AdditionAssignStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class SubtractionAssignStatement : public Statement {
    public:
    PlainExp* location;
    PlainExp* value;

    SubtractionAssignStatement(ErrorHandler&);
    SubtractionAssignStatement(PlainExp*, PlainExp*, ErrorHandler&);
    SubtractionAssignStatement(const SubtractionAssignStatement&);
    ~SubtractionAssignStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class MultiplicationAssignStatement : public Statement {
    public:
    PlainExp* location;
    PlainExp* value;

    MultiplicationAssignStatement(ErrorHandler&);
    MultiplicationAssignStatement(PlainExp*, PlainExp*, ErrorHandler&);
    MultiplicationAssignStatement(const MultiplicationAssignStatement&);
    ~MultiplicationAssignStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class DivisionAssignStatement : public Statement {
    public:
    PlainExp* location;
    PlainExp* value;

    DivisionAssignStatement(ErrorHandler&);
    DivisionAssignStatement(PlainExp*, PlainExp*, ErrorHandler&);
    DivisionAssignStatement(const DivisionAssignStatement&);
    ~DivisionAssignStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
};

class ModulusAssignStatement : public Statement {
    public:
    PlainExp* location;
    PlainExp* value;

    ModulusAssignStatement(ErrorHandler&);
    ModulusAssignStatement(PlainExp*, PlainExp*, ErrorHandler&);
    ModulusAssignStatement(const ModulusAssignStatement&);
    ~ModulusAssignStatement();
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
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
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class IfWhileElseStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;
    StatementList negative_statements;

    IfWhileElseStatement(ErrorHandler&);
    IfWhileElseStatement(PlainExp*, const StatementList&, const StatementList&, ErrorHandler&);
    IfWhileElseStatement(const IfWhileElseStatement&);
    ~IfWhileElseStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class UnlessUntilElseStatement : public Statement {
    public:
    PlainExp* condition;
    StatementList positive_statements;
    StatementList negative_statements;

    UnlessUntilElseStatement(ErrorHandler&);
    UnlessUntilElseStatement(PlainExp*, const StatementList&, const StatementList&, ErrorHandler&);
    UnlessUntilElseStatement(const UnlessUntilElseStatement&);
    ~UnlessUntilElseStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class DeallocStatement : public Statement {
    public:
    PlainExp* value;
    bool dangerous; // Whether or not this statement was marked as 'dangerous'

    DeallocStatement(ErrorHandler&);
    DeallocStatement(PlainExp*, ErrorHandler&);
    DeallocStatement(PlainExp*, bool, ErrorHandler&);
    DeallocStatement(const DeallocStatement&);
    ~DeallocStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class SwitchStatement : public Statement {
    public:

    struct Case {
        PlainExp* value;
        StatementList statements;

        Case();
        Case(PlainExp*, const StatementList&);
    };

    PlainExp* condition;
    std::vector<Case> cases;
    StatementList default_statements;

    SwitchStatement(ErrorHandler&);
    SwitchStatement(PlainExp*, const std::vector<SwitchStatement::Case>&, const StatementList&, ErrorHandler&);
    SwitchStatement(const SwitchStatement&);
    ~SwitchStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

class ForStatement : public Statement {
    public:
    Statement* initialization_statement;
    PlainExp* condition;
    Statement* increament_statement;
    StatementList statements;

    ForStatement(ErrorHandler&);
    ForStatement(Statement*, PlainExp*, Statement*, const StatementList&, ErrorHandler&);
    ForStatement(const ForStatement&);
    ~ForStatement();
    int assemble(Program&, Function&, AssemblyData&);
    std::string toString(unsigned int indent, bool skip_initial_indent);
    Statement* clone();
    bool isTerminator();
    bool isConditional();
};

void initialize_string(AssemblyData&, Program&, llvm::Value*);
int assign_string(AssemblyData&, Program&, Function&, llvm::Value*, PlainExp*, ErrorHandler&);

#endif // STATEMENT_H_INCLUDED
