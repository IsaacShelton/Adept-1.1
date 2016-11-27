
#ifndef VALIDATE_H_INCLUDED
#define VALIDATE_H_INCLUDED

#include "parse.h"
#include "statement.h"

bool validate_types(const std::string&, const std::string&, std::string*);
bool validate_return_statement(Program& program, ReturnStatement& ret, Function& func);
bool validate_declareas_statement(Program& program, DeclareAsStatement& declareas, Function& func);
bool validate_assign_statement(Program& program, AssignStatement& assignto, const Variable& var, Function& func);

#endif // VALIDATE_H_INCLUDED
