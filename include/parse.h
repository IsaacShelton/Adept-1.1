
#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#include <vector>
#include "type.h"
#include "config.h"
#include "tokens.h"
#include "program.h"
#include "attribute.h"
#include "statement.h"
#include "asmcontext.h"

#define CONDITIONAL_IF     0
#define CONDITIONAL_UNLESS 1
#define CONDITIONAL_WHILE  2
#define CONDITIONAL_UNTIL  3
#define CONDITIONAL_FOR    4

int parse(Configuration& config, std::vector<Token>* tokens, Program& program);

int parse_token(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_word(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_keyword(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info);
int parse_structure(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info);
int parse_function(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info);
int parse_external(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info);
int parse_attribute(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_import(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info);
int parse_lib(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i);
int parse_constant(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info);

int parse_block(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i);
int parse_block_keyword(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_word(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i);
int parse_block_variable_declaration(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_call(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name, int loads);
int parse_block_member_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name);
int parse_block_dereference(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i);
int parse_block_conditional(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, uint16_t conditional_type);

int parse_expression(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression);
int parse_expression_primary(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression);
int parse_expression_operator_right(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, int precedence, PlainExp** left);
int parse_expression_call(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression);

#endif // PARSE_H_INCLUDED
