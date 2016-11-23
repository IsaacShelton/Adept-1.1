
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "../include/lexer.h"
#include "../include/strings.h"

int tokenize(Configuration& config, std::string filename, std::vector<Token>& tokens){
    std::ifstream adept;

    adept.open(filename.c_str());
    if(!adept.is_open()){
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }

    std::string line;
    while( std::getline(adept, line) ){
        tokenize_string(line + "\n", tokens);
    }

    adept.close();

    // Print Lexer Time
    config.clock.print_since("Lexer Finished");
    config.clock.remember();
    return 0;
}
int tokenize_string(const std::string& code, std::vector<Token>& tokens){
    size_t code_size = code.size();

    char prefix_char;
    std::string until_space;

    for(size_t i = 0; i != code_size; i++){
        string_iter_kill_whitespace(code, i);
        prefix_char = code[i];

        if(prefix_char == '\n'){
            tokens.push_back(TOKEN_NEWLINE);
        }
        else if( (prefix_char>=65&&prefix_char<=90) || (prefix_char>=97&&prefix_char<=122) || (prefix_char==95) ){
            std::string word;

            while( (prefix_char>=65 && prefix_char<=90)
            ||  (prefix_char>=48 && prefix_char<=57)
            ||  (prefix_char>=97 && prefix_char<=122)
            ||  (prefix_char==95) ){
                word += prefix_char;
                if(++i != code_size) {
                    prefix_char = code[i];
                } else {
                    break;
                }
            }

            if(word == "return" or word == "def" or word == "type" or word == "extern"){
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
        else if(prefix_char == ','){
            tokens.push_back(TOKEN_NEXT);
        }
        else if(prefix_char == '='){
            tokens.push_back(TOKEN_ASSIGN);
        }
        else if(prefix_char == '+'){
            tokens.push_back(TOKEN_ADD);
        }
        else if(prefix_char == '-'){
            tokens.push_back(TOKEN_SUBTRACT);
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
        else if(prefix_char >= 48 and prefix_char <= 57){
            std::string content;

            while(prefix_char >= 48 and prefix_char <= 57){
                content += prefix_char;
                if(++i != code_size) {
                    prefix_char = code[i];
                } else {
                    break;
                }
            }

            if(prefix_char == '.'){
                if(++i != code_size) {
                    prefix_char = code[i];
                }
                else break;

                content += ".";
                while(prefix_char >= 48 and prefix_char <= 57){
                    content += prefix_char;
                    if(++i != code_size) {
                        prefix_char = code[i];
                    } else {
                        break;
                    }
                }

                tokens.push_back( TOKEN_FLOAT( to_double(content) ) );
                i--;
            } else {
                tokens.push_back( TOKEN_INT( to_int(content) ) );
                i--;
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
