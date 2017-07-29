
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "../include/die.h"
#include "../include/lexer.h"
#include "../include/search.h"
#include "../include/errors.h"
#include "../include/strings.h"

int tokenize(Configuration& config, std::string filename, std::vector<Token>* tokens, ErrorHandler& errors){
    if(config.time_verbose) config.time_verbose_clock.remember();
    std::ifstream input_file(filename.c_str());

    if(!input_file.is_open()){
        errors.panic( FAILED_TO_OPEN_FILE(filename_name(filename)) );
        return 1;
    }

    tokens->reserve(1024);

    std::stringstream string_buffer;
    string_buffer << input_file.rdbuf();
    std::string file_contents = string_buffer.str() + "\n";

    if(tokenize_code(file_contents, *tokens, errors) != 0) return 1;

    errors.line = 1;
    input_file.close();

    // Print Lexer Time
    if(config.time_verbose and !config.silent){
        config.time_verbose_clock.print_since("LEXER DONE", filename_name(config.filename));
        config.time_verbose_clock.remember();
    }

    return 0;
}

int tokenize_code(const std::string& code, std::vector<Token>& tokens, ErrorHandler& errors){
    size_t code_size = code.size();

    char prefix_char;
    char peek_char;
    std::string until_space;

    for( size_t i = 0; i != code_size; ){
        prefix_char = code[i];

        while(prefix_char == ' ' or prefix_char == '\t'){
            prefix_char = code[++i];
        }

        if(i + 1 != code_size){
            peek_char = code[i + 1];
        } else {
            peek_char = '\0';
        }

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
            if(peek_char == '=') {
                tokens.push_back(TOKEN_EQUALITY);
                i += 2; break;
            }
            tokens.push_back(TOKEN_ASSIGN);
            i++; break;
        case '!':
            if(peek_char == '=') {
                tokens.push_back(TOKEN_INEQUALITY);
                i += 2; break;
            }
            else {
                tokens.push_back(TOKEN_NOT);
                i++; break;
            }
        case '+':
            if(peek_char == '='){
                tokens.push_back(TOKEN_ASSIGNADD);
                i += 2; break;
            }
            tokens.push_back(TOKENID_ADD);
            i++; break;
        case '-':
            if(peek_char == '=') {
                tokens.push_back(TOKEN_ASSIGNSUB);
                i += 2; break;
            }

            if(peek_char >= 48 and peek_char <= 57) {
                if(tokenize_number(true, peek_char, ++i, code_size, code, tokens, errors) != 0) return 1;
                i++; break;
            }

            tokens.push_back(TOKEN_SUBTRACT);
            i++; break;
        case '*':
            if(peek_char == '='){
                tokens.push_back(TOKEN_ASSIGNMUL);
                i += 2; break;
            }

            tokens.push_back(TOKENID_MULTIPLY);
            i++; break;
        case '/':
            if(peek_char == '=') {
                tokens.push_back(TOKEN_ASSIGNDIV);
                i += 2; break;
            }

            if(peek_char == '/'){
                i++; while(code[++i] != '\n');
                tokens.push_back(TOKEN_NEWLINE);
                errors.line++; i++; break;
            }

            if(peek_char == '*'){
                while(++i != code_size){
                    if(code[i] == '\n'){
                        tokens.push_back(TOKEN_NEWLINE);
                        errors.line++;
                    }
                    else if(code[i] == '*') if(i + 1 != code_size) if(code[i+1] == '/') {
                        i += 2; break;
                    }
                }

                break;
            }

            tokens.push_back(TOKEN_DIVIDE);
            i++; break;
        case ':':
            if(peek_char == ':'){
                tokens.push_back(TOKEN_NAMESPACE);
                i += 2; break;
            } else {
                errors.panic("ERROR: Lexer found unknown token ':'");
                return 1;
            }
            break;
        case '%':
            if(peek_char == '=') {
                tokens.push_back(TOKEN_ASSIGNMOD);
                i += 2; break;
            }

            tokens.push_back(TOKENID_MODULUS);
            i++; break;
        case '.':
            if(i + 2 < code_size && code.substr(i, 2) == ".."){
                tokens.push_back(TOKEN_ELLIPSIS);
                i += 3;
            } else {
                tokens.push_back(TOKEN_MEMBER); i++;
            }
            break;
        case '&':
            tokens.push_back(TOKEN_ADDRESS); i++;
            break;
        case '<':
            if(peek_char == '=') {
                tokens.push_back(TOKEN_LESSEQ);
                i += 2; break;
            }

            tokens.push_back(TOKEN_LESS);
            i++; break;
        case '>':
            if(peek_char == '=') { tokens.push_back(TOKEN_GREATEREQ); i += 2; }
            else { tokens.push_back(TOKEN_GREATER); i++; }
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

                while(true){
                    word += prefix_char;
                    prefix_char = peek_char;
                    i += 1;

                    if(i + 1 != code_size){
                        peek_char = code[i + 1];
                    } else {
                        peek_char = '\0';
                    }

                    if(prefix_char == 95) continue;
                    if(prefix_char >= 65 and prefix_char <= 90) continue;
                    if(prefix_char >= 97 and prefix_char <= 122) continue;
                    if(prefix_char >= 48 and prefix_char <= 57) continue;

                    break;
                }

                // NOTE: MUST be pre sorted alphabetically (used for string_search)
                //         Make sure to update switch statement with correct indices after add or removing a type
                const size_t keywords_size = 39;
                const std::string keywords[keywords_size] = {
                    "alias", "and", "break", "case", "cast", "constant", "continue", "dangerous", "def", "default", "defer",
                    "delete", "dynamic", "else", "enum", "external", "false", "for", "foreign", "funcptr", "if",
                    "import", "link", "new", "null", "or", "packed", "private", "public", "return", "sizeof",
                    "static", "stdcall", "struct", "switch", "true", "unless", "until", "while"
                };

                int keyword_index = string_search(keywords, keywords_size, word);

                switch(keyword_index){
                case -1:
                    // Not a keyword, just an identifier
                    tokens.push_back( TOKEN_WORD( new std::string(word) ) );
                    break;
                case 1:
                    // 'and' keyword
                    tokens.push_back( TOKEN_AND );
                    break;
                case 25:
                    // 'or' keyword
                    tokens.push_back( TOKEN_OR );
                    break;
                default:
                    // It's a regular keyword
                    tokens.push_back( TOKEN_KEYWORD( new std::string(word) ) );
                }

                break;
            }
        case '"':
            {
                std::string content;
                std::string escaped_content;

                while(i + 1 != code_size and (code[++i] != '"' or code[i-1] == '\\')){
                    content += code[i];
                }
                for(size_t j = 0; j != content.size(); j++){
                    if(content[j] == '\\'){
                        if(++j == content.size()){
                            errors.panic("Unexpected string termination");
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
                        case '\\':
                            escaped_content += "\\";
                            break;
                        case '"':
                            escaped_content += "\"";
                            break;
                        default:
                            errors.panic("Unknown escape sequence '\\" + content.substr(j, content.size()-j) + "'");
                            return 1;
                        }
                    } else {
                        escaped_content += content[j];
                    }
                }

                tokens.push_back( TOKEN_STRING(new std::string(escaped_content)) );
                i++;
            }
            break;
        case '\'':
            {
                std::string content;
                std::string escaped_content;

                while(i + 1 != code_size and (code[++i] != '\'' or code[i-1] == '\\')){
                    content += code[i];
                }
                for(size_t j = 0; j != content.size(); j++){
                    if(content[j] == '\\'){
                        if(++j == content.size()){
                            errors.panic("Unexpected string termination");
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
                        case '\\':
                            escaped_content += "\\";
                            break;
                        case '\'':
                            escaped_content += "'";
                            break;
                        default:
                            errors.panic("Unknown escape sequence '\\" + content.substr(j, content.size()-j) + "'");
                            return 1;
                        }
                    }
                    else {
                        escaped_content += content[j];
                    }
                }

                tokens.push_back( TOKEN_LENGTHSTRING(new std::string(escaped_content)) );
                i++;
            }
            break;
        case '$':
            {
                std::string word;
                next_index(i, code_size);
                prefix_char = code[i];

                if( prefix_char != 95 and !(prefix_char >= 65 and prefix_char <= 90) and !(prefix_char >= 97 and prefix_char <= 122) ){
                    errors.panic("Expected identifier after '$' operator");
                    return 1;
                }

                while(true){
                    word += prefix_char;
                    next_index(i, code_size);
                    prefix_char = code[i];

                    if(prefix_char == 95) continue;
                    if(prefix_char >= 65 and prefix_char <= 90) continue;
                    if(prefix_char >= 97 and prefix_char <= 122) continue;
                    if(prefix_char >= 48 and prefix_char <= 57) continue;

                    break;
                }

                tokens.push_back( TOKEN_CONSTANT( new std::string(word) ) );
            }
            break;
        case '@':
            {
                std::string word;
                next_index(i, code_size);
                prefix_char = code[i];

                if( prefix_char != 95 and !(prefix_char >= 65 and prefix_char <= 90) and !(prefix_char >= 97 and prefix_char <= 122) ){
                    errors.panic("Expected identifier after '$' operator");
                    return 1;
                }

                while(true){
                    word += prefix_char;
                    next_index(i, code_size);
                    prefix_char = code[i];

                    if(prefix_char == 95) continue;
                    if(prefix_char >= 65 and prefix_char <= 90) continue;
                    if(prefix_char >= 97 and prefix_char <= 122) continue;
                    if(prefix_char >= 48 and prefix_char <= 57) continue;

                    break;
                }

                tokens.push_back( TOKEN_POLYMORPHIC( new std::string(word) ) );
            }
            break;
        case 48: case 49: case 50:
        case 51: case 52: case 53:
        case 54: case 55: case 56:
        case 57: // 0-9s
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
    std::string content;

    content = prefix_char;
    next_index(i, code_size);
    prefix_char = code[i];

    if(prefix_char == 'x'){
        // Handle hex then return

        content = "";
        next_index(i, code_size);
        prefix_char = code[i];

        while( (prefix_char >= 48 and prefix_char <= 57) or (prefix_char >= 65 and prefix_char <= 70) or (prefix_char >= 97 and prefix_char <= 102)  or prefix_char == '_' ){
            content += prefix_char;
            next_index(i, code_size);
            prefix_char = code[i];
        }

        uint64_t hex_result;
        std::stringstream string_stream;
        string_stream << std::hex << content;
        string_stream >> hex_result;

        switch(prefix_char){
        case 'u':
            next_index(i, code_size);
            prefix_char = code[i];

            switch(prefix_char){
            case 'b':
                tokens.push_back( TOKEN_UBYTE(static_cast<unsigned char>(hex_result)) );
                break;
            case 's':
                tokens.push_back( TOKEN_USHORT(static_cast<uint16_t>(hex_result)) );
                break;
            case 'i':
                tokens.push_back( TOKEN_UINT(static_cast<uint32_t>(hex_result)) );
                break;
            case 'l':
                tokens.push_back( TOKEN_ULONG(hex_result) );
                break;
            case 'z':
                tokens.push_back( TOKEN_USIZE(hex_result) );
                break;
            default:
                tokens.push_back( TOKEN_UINT(static_cast<uint32_t>(hex_result)) );
                i--;
            }
            break;
        case 's':
            next_index(i, code_size);
            prefix_char = code[i];

            switch(prefix_char){
            case 'b':
                tokens.push_back( TOKEN_BYTE(static_cast<char>(hex_result)) );
                break;
            case 's':
                tokens.push_back( TOKEN_SHORT(static_cast<int16_t>(hex_result)) );
                break;
            case 'i':
                tokens.push_back( TOKEN_INT(static_cast<int32_t>(hex_result)) );
                break;
            case 'l':
                tokens.push_back( TOKEN_LONG(static_cast<int64_t>(hex_result)) );
                break;
            default:
                tokens.push_back( TOKEN_INT(static_cast<int32_t>(hex_result)) );
                i--;
            }
            break;
        default:
            tokens.push_back( TOKEN_INT(static_cast<int>(hex_result)) );
            i--;
        }

        return 0;
    }
    else {
        content = (is_negative ? "-" : "") + content;

        while( (prefix_char >= 48 and prefix_char <= 57) or prefix_char == '_' ){
            content += prefix_char;
            next_index(i, code_size);
            prefix_char = code[i];
        }
    }

    if(prefix_char == '.'){
        next_index(i, code_size);
        prefix_char = code[i];

        content += ".";
        while( (prefix_char >= 48 and prefix_char <= 57) or prefix_char == '_' ){
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
        else if(prefix_char == 'h'){
            tokens.push_back( TOKEN_HALF( to_float(content) ) );
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
            case 'z':
                tokens.push_back( TOKEN_USIZE( to_ulong(content) ) );
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
