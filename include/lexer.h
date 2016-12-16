
#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include <vector>
#include "config.h"
#include "tokens.h"

int tokenize(Configuration& config, std::string filename, std::vector<Token>* tokens);
int tokenize_line(const std::string&, std::vector<Token>& tokens);
int tokenize_number(bool is_negative, char& prefix_char, size_t& i, size_t& code_size, const std::string& code, std::vector<Token>& tokens);

#endif // LEXER_H_INCLUDED
