
#ifndef VALIDATE_H_INCLUDED
#define VALIDATE_H_INCLUDED

#include "parse.h"
#include "statement.h"

bool validate_return_statement(Program& program, const ReturnStatement& ret, Function& func);
bool validate_declareas_statement(Program& program, const DeclareAsStatement& declareas, Function& func);
bool validate_assign_statement(Program& program, const AssignStatement& assignto, const Variable& var, Function& func);

#endif // VALIDATE_H_INCLUDED
