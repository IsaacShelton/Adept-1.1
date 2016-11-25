
#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#include <vector>
#include "type.h"
#include "config.h"
#include "tokens.h"
#include "program.h"
#include "statement.h"
#include "asmcontext.h"

int parse(Configuration& config, std::vector<Token>* tokens, Program& program);

int parse_word(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_keyword(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_token(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_structure(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_function(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_external(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);

int parse_block(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i);
int parse_block_keyword(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_word(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i);
int parse_block_variable_declaration(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_call(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_member_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);

int parse_expression(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression);
int parse_expression_primary(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression);
int parse_expression_operator_right(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, int precedence, PlainExp** left);
int parse_expression_call(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression);

#endif // PARSE_H_INCLUDED
