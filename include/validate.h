
#ifndef VALIDATE_H_INCLUDED
#define VALIDATE_H_INCLUDED

#include "parse.h"
#include "statement.h"

bool validate_return_statement(Program& program, const ReturnStatement& ret, Function& func);

#endif // VALIDATE_H_INCLUDED
