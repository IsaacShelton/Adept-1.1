
#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include <vector>
#include "config.h"
#include "tokens.h"

int tokenize(Configuration& config, std::string filename, std::vector<Token>& tokens);
int tokenize_string(const std::string&, std::vector<Token>& tokens);

#endif // LEXER_H_INCLUDED
