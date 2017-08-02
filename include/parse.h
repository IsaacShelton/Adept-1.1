
#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#include <vector>
#include "config.h"
#include "tokens.h"
#include "program.h"
#include "attribute.h"
#include "statement.h"

#define CONDITIONAL_IF     STATEMENTID_IF
#define CONDITIONAL_UNLESS STATEMENTID_UNLESS
#define CONDITIONAL_WHILE  STATEMENTID_WHILE
#define CONDITIONAL_UNTIL  STATEMENTID_UNTIL

int parse(Config& config, TokenList* tokens, Program& program, ErrorHandler& errors);

int parse_word(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_keyword(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_structure(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_enum(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_struct(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_type_alias(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_function(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_method(Config& config, TokenList& tokens, Program& program, size_t& i, Struct*, size_t, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_external(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_attribute(Config& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors);
int parse_import(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_lib(Config& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors);
int parse_constant(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_global(Config& config, TokenList& tokens, Program& program, size_t& i, const std::string& name, const AttributeInfo& attr_info, ErrorHandler& errors);
int parse_type(Config& config, TokenList& tokens, Program& program, size_t& i, std::string& output_type, ErrorHandler& errors);
int parse_type_funcptr(Config& config, TokenList& tokens, Program& program, size_t& i, std::string& output_type, const std::string& keyword, ErrorHandler& errors);

int parse_block(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors);
int parse_block_keyword(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, const std::string& name, ErrorHandler& errors);
int parse_block_word(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors);
int parse_block_declaration(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, const std::string& name, ErrorHandler& errors);
int parse_block_call(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, const std::string& name, ErrorHandler& errors);
int parse_block_word_expression(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, const std::string& name, int loads, ErrorHandler& errors);
int parse_block_dereference(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors);
int parse_block_conditional(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, uint16_t conditional_type, ErrorHandler& errors);
int parse_block_word_member(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, const std::string& name, ErrorHandler& errors);
int parse_block_member_call(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, PlainExp* value, const std::string& func_name, ErrorHandler& errors);
int parse_block_switch(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors);
int parse_block_multireturn_call(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors);
int parse_block_fixed_array_declaration(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, const std::string& name, ErrorHandler& errors);

int parse_expression(Config& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors);
int parse_expression_primary(Config& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors);
int parse_expression_operator_right(Config& config, TokenList& tokens, Program& program, size_t& i, int precedence, PlainExp** left, bool keep_mutable, ErrorHandler& errors);
int parse_expression_call(Config& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors);

#endif // PARSE_H_INCLUDED
