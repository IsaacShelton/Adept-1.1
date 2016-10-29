
#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include <vector>
#include "tokens.h"

int tokenize(std::string filename, std::vector<Token>& tokens);

#endif // LEXER_H_INCLUDED
