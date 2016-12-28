
#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include <vector>
#include "config.h"
#include "tokens.h"
#include "errors.h"

int tokenize(Configuration& config, std::string filename, std::vector<Token>* tokens, ErrorHandler& errors);
int tokenize_line(const std::string&, std::vector<Token>& tokens, ErrorHandler& errors);
int tokenize_number(bool is_negative, char& prefix_char, size_t& i, size_t& code_size, const std::string& code, std::vector<Token>& tokens, ErrorHandler& errors);

#endif // LEXER_H_INCLUDED
