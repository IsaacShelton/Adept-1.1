
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

int parse(Configuration& config, TokenList* tokens, Program& program, ErrorHandler& errors);

int parse_token(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors);
int parse_word(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors);
int parse_keyword(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_structure(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_function(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_external(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_attribute(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors);
int parse_import(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_lib(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors);
int parse_constant(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);

int parse_block(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors);
int parse_block_keyword(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors);
int parse_block_word(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors);
int parse_block_variable_declaration(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors);
int parse_block_call(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors);
int parse_block_assign(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, int loads, ErrorHandler& errors);
int parse_block_member(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors);
int parse_block_dereference(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors);
int parse_block_conditional(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, uint16_t conditional_type, ErrorHandler& errors);
int parse_block_member_call(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, PlainExp* value, std::string func_name, ErrorHandler& errors);

int parse_expression(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors);
int parse_expression_primary(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors);
int parse_expression_operator_right(Configuration& config, TokenList& tokens, Program& program, size_t& i, int precedence, PlainExp** left, ErrorHandler& errors);
int parse_expression_call(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors);

#endif // PARSE_H_INCLUDED
