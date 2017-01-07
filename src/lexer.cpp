
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "../include/lexer.h"
#include "../include/errors.h"
#include "../include/strings.h"

int tokenize(Configuration& config, std::string filename, std::vector<Token>* tokens, ErrorHandler& errors){
    std::ifstream adept;

    adept.open(filename.c_str());
    if(!adept.is_open()){
        errors.panic( FAILED_TO_OPEN_FILE(filename_name(filename)) );
        return 1;
    }

    tokens->reserve(10000000);

    std::string line;
    while( std::getline(adept, line) ){
        if(tokenize_line(line + "\n", *tokens, errors) != 0) return 1;
    }

    errors.line = 1;
    adept.close();

    // Print Lexer Time
    if(config.time and !config.silent){
        config.clock.print_since("LEXER DONE", filename_name(config.filename));
        config.clock.remember();
    }
    return 0;
}
int tokenize_line(const std::string& code, std::vector<Token>& tokens, ErrorHandler& errors){
    size_t code_size = code.size();

    char prefix_char;
    std::string until_space;

    for( size_t i = 0; i != code_size; ){
        string_iter_kill_whitespace(code, i);
        prefix_char = code[i];

        switch(prefix_char){
        case '\n':
            tokens.push_back(TOKEN_NEWLINE); i++;
            errors.line++;
            break;
        case '(':
            tokens.push_back(TOKEN_OPEN); i++;
            break;
        case ')':
            tokens.push_back(TOKEN_CLOSE); i++;
            break;
        case '[':
            tokens.push_back(TOKEN_BRACKET_OPEN); i++;
            break;
        case ']':
            tokens.push_back(TOKEN_BRACKET_CLOSE); i++;
            break;
        case '{':
            tokens.push_back(TOKEN_BEGIN); i++;
            break;
        case '}':
            tokens.push_back(TOKEN_END); i++;
            break;
        case ',':
            tokens.push_back(TOKEN_NEXT); i++;
            break;
        case '=':
            next_index(i, code.size());
            if(code[i] == '=') { tokens.push_back(TOKEN_EQUALITY); i++; }
            else { tokens.push_back(TOKEN_ASSIGN); }
            break;
        case '!':
            next_index(i, code.size());
            if(code[i] == '='){ tokens.push_back(TOKEN_INEQUALITY); i++; }
            else { tokens.push_back(TOKEN_NOT); }
            break;
        case '+':
            tokens.push_back(TOKEN_ADD); i++;
            break;
        case '-':
            next_index(i, code_size);
            prefix_char = code[i];

            if(prefix_char >= 48 and prefix_char <= 57){
                if(tokenize_number(true, prefix_char, i, code_size, code, tokens, errors) != 0) return 1;
                i++;
            }
            else { tokens.push_back(TOKEN_SUBTRACT); }
            break;
        case '*':
            tokens.push_back(TOKEN_MULTIPLY); i++;
            break;
        case '/':
            next_index(i, code.size());
            if(code[i] == '/'){
                while(code[i] != '\n') i++;
                tokens.push_back(TOKEN_NEWLINE);
                errors.line++;
                i++;
            }
            else { tokens.push_back(TOKEN_DIVIDE); }
            break;
        case '%':
            tokens.push_back(TOKEN_MODULUS);
            next_index(i, code.size());
            break;
        case '.':
            tokens.push_back(TOKEN_MEMBER); i++;
            break;
        case '&':
            tokens.push_back(TOKEN_ADDRESS); i++;
            break;
        case '<':
            next_index(i, code.size());
            if(code[i] == '=') { tokens.push_back(TOKEN_LESSEQ); i++; }
            else { tokens.push_back(TOKEN_LESS); }
            break;
        case '>':
            next_index(i, code.size());
            if(code[i] == '=') { tokens.push_back(TOKEN_GREATEREQ); i++; }
            else { tokens.push_back(TOKEN_GREATER); }
            break;
        case 65: case 66: case 67:
        case 68: case 69: case 70:
        case 71: case 72: case 73:
        case 74: case 75: case 76:
        case 77: case 78: case 79:
        case 80: case 81: case 82:
        case 83: case 84: case 85:
        case 86: case 87: case 88:
        case 89: case 90: // A-Z
        case 97: case 98: case 99:
        case 100: case 101: case 102:
        case 103: case 104: case 105:
        case 106: case 107: case 108:
        case 109: case 110: case 111:
        case 112: case 113: case 114:
        case 115: case 116: case 117:
        case 118: case 119: case 120:
        case 121: case 122: // a-z
        case 95:
            {
                std::string word;

                // Not perfect, but whatever
                while( prefix_char != '(' and prefix_char != ')' and prefix_char != '.'
                and prefix_char != ' ' and prefix_char != '\n' and prefix_char != '['
                and prefix_char != ']' and prefix_char != '{' and prefix_char != '}'
                and prefix_char != '+' and prefix_char != '-' and prefix_char != '*'
                and prefix_char != '/' and prefix_char != '=' and prefix_char != '!'
                and prefix_char != ',' and prefix_char != '%'){
                    word += prefix_char;
                    next_index(i, code_size);
                    prefix_char = code[i];
                }

                // TODO: Clean up keyword selection
                if(word == "return" or word == "def" or word == "type" or word == "foreign" or word == "import"
                or word == "public" or word == "private" or word == "link" or word == "true" or word == "false"
                or word == "null" or word == "if" or word == "while"  or word == "unless" or word == "until"
                or word == "else" or word == "constant" or word == "dynamic" or word == "cast"){
                    tokens.push_back( TOKEN_KEYWORD( new std::string(word) ) );
                }
                else if(word == "and"){
                    tokens.push_back( TOKEN_AND );
                }
                else if(word == "or"){
                    tokens.push_back( TOKEN_OR );
                }
                else {
                    tokens.push_back( TOKEN_WORD( new std::string(word) ) );
                }
            }
            break;
        case '"':
            {
                std::string content;
                std::string escaped_content;

                while(code[++i] != '"'){
                    content += code[i];
                }
                for(size_t j = 0; j != content.size(); j++){
                    if(content[j] == '\\'){
                        if(++j == content.size()){
                            fail("Unexpected string termination");
                            return 1;
                        }

                        switch(content[j]){
                        case 'n':
                            escaped_content += "\n";
                            break;
                        case '0':
                            escaped_content += "\0";
                            break;
                        case 'a':
                            escaped_content += "\a";
                            break;
                        default:
                            fail("Unknown escape sequence '\\" << content[j] << "'");
                            return 1;
                        }
                    }
                    else {
                        escaped_content += content[j];
                    }
                }

                tokens.push_back( TOKEN_STRING(new std::string(escaped_content)) );
                i++;
            }
            break;
        case '$':
            {
                std::string word;
                next_index(i, code_size);
                prefix_char = code[i];

                while( prefix_char != '(' and prefix_char != ')' and prefix_char != '.'
                and prefix_char != ' ' and prefix_char != '\n' and prefix_char != '['
                and prefix_char != ']' and prefix_char != '{' and prefix_char != '}'
                and prefix_char != '+' and prefix_char != '-' and prefix_char != '*'
                and prefix_char != '/' and prefix_char != '=' and prefix_char != '!'
                and prefix_char != ','){
                    word += prefix_char;
                    next_index(i, code_size);
                    prefix_char = code[i];
                }

                tokens.push_back( TOKEN_CONSTANT( new std::string(word) ) );
            }
            break;
        case 48: case 49: case 50:
        case 51: case 52: case 53:
        case 54: case 55: case 56:
        case 57: // 0-9
            {
                if(tokenize_number(false, prefix_char, i, code_size, code, tokens, errors) != 0) return 1;
                i++;
            }
            break;
        default:
            errors.panic("ERROR: Lexer found unknown token '" + code.substr(i, 1) + "'");
            return 1;
        }
    }

    return 0;
}
int tokenize_number(bool is_negative, char& prefix_char, size_t& i, size_t& code_size, const std::string& code, std::vector<Token>& tokens, ErrorHandler& errors){
    std::string content = (is_negative) ? "-" : "" ;

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

    return 0;
}
