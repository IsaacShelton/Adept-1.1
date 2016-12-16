
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "../include/lexer.h"
#include "../include/errors.h"
#include "../include/strings.h"

int tokenize(Configuration& config, std::string filename, std::vector<Token>* tokens){
    std::ifstream adept;

    adept.open(filename.c_str());
    if(!adept.is_open()){
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }

    tokens->reserve(10000000);

    std::string line;
    while( std::getline(adept, line) ){
        if(tokenize_string(line + "\n", *tokens) != 0) return 1;
    }

    adept.close();

    // Print Lexer Time
    if(config.time and !config.silent){
        config.clock.print_since("Lexer Finished");
        config.clock.remember();
    }
    return 0;
}
int tokenize_string(const std::string& code, std::vector<Token>& tokens){
    size_t code_size = code.size();

    char prefix_char;
    std::string until_space;

    for(size_t i = 0; i != code_size; i++){
        string_iter_kill_whitespace(code, i);
        prefix_char = code[i];

        // TODO: Should probally use switch
        if(prefix_char == '\n'){
            tokens.push_back(TOKEN_NEWLINE);
        }
        else if( (prefix_char>=65&&prefix_char<=90) || (prefix_char>=97&&prefix_char<=122) || (prefix_char==95) ){
            std::string word;

            while( (prefix_char>=65 && prefix_char<=90)
            ||  (prefix_char>=48 && prefix_char<=57)
            ||  (prefix_char>=97 && prefix_char<=122)
            ||  (prefix_char==95) || (prefix_char==':') ){
                word += prefix_char;
                next_index(i, code_size);
                prefix_char = code[i];
            }

            // TODO: Clean up keyword selection
            if(word == "return" or word == "def" or word == "type" or word == "foreign" or word == "import"
               or word == "public" or word == "private" or word == "link" or word == "true" or word == "false"
               or word == "null" or word == "if" or word == "while" or word == "constant"){
                tokens.push_back( TOKEN_KEYWORD( new std::string(word) ) );
            }
            else {
                tokens.push_back( TOKEN_WORD( new std::string(word) ) );
            }
            i--;
        }
        else if(prefix_char == '('){
            tokens.push_back(TOKEN_OPEN);
        }
        else if(prefix_char == ')'){
            tokens.push_back(TOKEN_CLOSE);
        }
        else if(prefix_char == '{'){
            tokens.push_back(TOKEN_BEGIN);
        }
        else if(prefix_char == '}'){
            tokens.push_back(TOKEN_END);
        }
        else if(prefix_char == '['){
            tokens.push_back(TOKEN_BRACKET_OPEN);
        }
        else if(prefix_char == ']'){
            tokens.push_back(TOKEN_BRACKET_CLOSE);
        }
        else if(prefix_char == ','){
            tokens.push_back(TOKEN_NEXT);
        }
        else if(prefix_char == '='){
            next_index(i, code.size());

            if(code[i] == '='){
                tokens.push_back(TOKEN_EQUALITY);
            }
            else {
                tokens.push_back(TOKEN_ASSIGN);
                i--;
            }
        }
        else if(prefix_char == '!'){
            next_index(i, code.size());

            if(code[i] == '='){
                tokens.push_back(TOKEN_INEQUALITY);
            }
            else {
                i--;
                // TODO: 'not' operator
                std::cerr << "lexer error: " + code.substr(i, 1) <<  std::endl;
                return 1;
            }
        }
        else if(prefix_char == '+'){
            tokens.push_back(TOKEN_ADD);
        }
        else if(prefix_char == '-'){
            next_index(i, code_size);
            prefix_char = code[i];

            if(prefix_char >= 48 and prefix_char <= 57){
                std::string content = "-";

                while(prefix_char >= 48 and prefix_char <= 57){
                    content += prefix_char;
                    next_index(i, code_size);
                    prefix_char = code[i];
                }

                if(prefix_char == '.'){
                    next_index(i, code_size);
                    prefix_char = code[i];

                    content += ".";
                    while(prefix_char >= 48 and prefix_char <= 57){
                        content += prefix_char;
                        next_index(i, code_size);
                        prefix_char = code[i];
                    }

                    tokens.push_back( TOKEN_DOUBLE( to_double(content) ) );
                } else {
                    tokens.push_back( TOKEN_INT( to_int(content) ) );
                }
            }
            else {
                tokens.push_back(TOKEN_SUBTRACT);
            }

            i--;
        }
        else if(prefix_char == '*'){
            tokens.push_back(TOKEN_MULTIPLY);
        }
        else if(prefix_char == '/'){
            if(i + 1 < code_size){
                if(code[++i] == '/'){
                    while(i != code.size() and code[i] != '\n') i++;
                } else {
                    i--;
                    tokens.push_back(TOKEN_DIVIDE);
                }
            } else {
                tokens.push_back(TOKEN_DIVIDE);
            }
        }
        else if(prefix_char == '.'){
            tokens.push_back(TOKEN_MEMBER);
        }
        else if(prefix_char == '$'){
            std::string word;
            next_index(i, code_size);
            prefix_char = code[i];

            while( (prefix_char>=65 && prefix_char<=90)
            ||  (prefix_char>=48 && prefix_char<=57)
            ||  (prefix_char>=97 && prefix_char<=122)
            ||  (prefix_char==95) ){
                word += prefix_char;
                next_index(i, code_size);
                prefix_char = code[i];
            }

            tokens.push_back( TOKEN_CONSTANT( new std::string(word) ) );
            i--;
        }
        else if(prefix_char == '&'){
            tokens.push_back(TOKEN_ADDRESS);
        }
        else if(prefix_char >= 48 and prefix_char <= 57){
            std::string content;

            while(prefix_char >= 48 and prefix_char <= 57){
                content += prefix_char;
                next_index(i, code_size);
                prefix_char = code[i];
            }
            if(prefix_char == '.'){
                next_index(i, code_size);
                prefix_char = code[i];

                content += ".";
                while(prefix_char >= 48 and prefix_char <= 57){
                    content += prefix_char;
                    next_index(i, code_size);
                    prefix_char = code[i];
                }

                if(prefix_char == 'f'){
                    tokens.push_back( TOKEN_FLOAT( to_float(content) ) );
                }
                else if(prefix_char == 'd'){
                    tokens.push_back( TOKEN_DOUBLE( to_double(content) ) );
                }
                else {
                    tokens.push_back( TOKEN_DOUBLE( to_double(content) ) );
                    i--;
                }
            }
            else {
                switch(prefix_char){
                case 'u':
                    next_index(i, code_size);
                    prefix_char = code[i];

                    switch(prefix_char){
                    case 'b':
                        tokens.push_back( TOKEN_UBYTE( to_ubyte(content) ) );
                        break;
                    case 's':
                        tokens.push_back( TOKEN_USHORT( to_ushort(content) ) );
                        break;
                    case 'i':
                        tokens.push_back( TOKEN_UINT( to_uint(content) ) );
                        break;
                    case 'l':
                        tokens.push_back( TOKEN_ULONG( to_ulong(content) ) );
                        break;
                    default:
                        tokens.push_back( TOKEN_UINT( to_uint(content) ) );
                        i--;
                    }
                    break;
                case 's':
                    next_index(i, code_size);
                    prefix_char = code[i];

                    switch(prefix_char){
                    case 'b':
                        tokens.push_back( TOKEN_BYTE( to_byte(content) ) );
                        break;
                    case 's':
                        tokens.push_back( TOKEN_SHORT( to_short(content) ) );
                        break;
                    case 'i':
                        tokens.push_back( TOKEN_INT( to_int(content) ) );
                        break;
                    case 'l':
                        tokens.push_back( TOKEN_LONG( to_long(content) ) );
                        break;
                    default:
                        tokens.push_back( TOKEN_INT( to_int(content) ) );
                        i--;
                    }
                    break;
                default:
                    tokens.push_back( TOKEN_INT( to_int(content) ) );
                    i--;
                }
            }
        }
        else if(prefix_char == '"'){
            std::string* content = new std::string;
            while(code[++i] != '"'){
                *content += code[i];
            }
            tokens.push_back( TOKEN_STRING(content) );
        }
        else {
            std::cerr << "lexer error: " + code.substr(i, 1) <<  std::endl;
            return 1;
        }
    }

    return 0;
}
