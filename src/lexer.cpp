
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "../include/lexer.h"
#include "../include/strings.h"

int tokenize(std::string filename, std::vector<Token>& tokens){
    std::ifstream adept;
    std::string code;
    size_t code_size;

    adept.open(filename.c_str());
    if(!adept.is_open()){
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }

    std::string line;
    while( std::getline(adept, line) ){
        code += line + "\n";
    }

    adept.close();
    code_size = code.size();

    char prefix_char;
    std::string until_space;

    for(size_t i = 0; i != code_size; i++){
        string_iter_kill_whitespace(code, i);
        prefix_char = code[i];

        if(prefix_char == '\n'){
            tokens.push_back(TOKEN_NEWLINE);
        }
        else if( (prefix_char>=65&&prefix_char<=90) || (prefix_char>=97&&prefix_char<=122) || (prefix_char==95) ){
            std::string* word = new std::string;

            while( (prefix_char>=65 && prefix_char<=90)
            ||  (prefix_char>=48 && prefix_char<=57)
            ||  (prefix_char>=97 && prefix_char<=122)
            ||  (prefix_char==95) ){
                *word += prefix_char;
                if(++i != code_size) {
                    prefix_char = code[i];
                } else {
                    break;
                }
            }

            tokens.push_back( TOKEN_WORD(word) );
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
            if(i + 1 < code_size){
                if(code[++i] == '>'){
                    tokens.push_back(TOKEN_RETIND);
                } else {
                    i--;
                    tokens.push_back(TOKEN_SUBTRACT);
                }
            } else {
                tokens.push_back(TOKEN_SUBTRACT);
            }
        }
        else if(prefix_char == '*'){
            tokens.push_back(TOKEN_MULTIPLY);
        }
        else if(prefix_char == '/'){
            tokens.push_back(TOKEN_DIVIDE);
        }
        else if(prefix_char == '.'){
            tokens.push_back(TOKEN_MEMBER);
        }
        else if(prefix_char == ':'){
            if(i + 1 != code_size){
                if(code[++i] == ':'){
                    tokens.push_back(TOKEN_NAMESPACE);
                } else {
                    i--;
                    tokens.push_back(TOKEN_TYPEIND);
                }
            } else {
                tokens.push_back(TOKEN_TYPEIND);
            }
        }
        else if(prefix_char >= 48 and prefix_char <= 57){
            std::string content;
            content += string_iter_until_or(code, i, " \n(){}[]<>=!");
            tokens.push_back( TOKEN_INT( to_int(content) ) );
            i--;
        }
        else if(prefix_char == '"'){
            std::string* content = new std::string;
            while(code[++i] != '"'){
                *content += code[i];
            }
            tokens.push_back( TOKEN_STRING(content) );
        }
        else {
            until_space = string_itertest_until(code, i, ' ');

            if(until_space == "return"){
                tokens.push_back( TOKEN_KEYWORD("return") );
                i += 6;
            }
            else {
                std::cerr << "lexer error: " + code.substr(i, 1) <<  std::endl;
                return 1;
            }
        }
    }

    return 0;
}
