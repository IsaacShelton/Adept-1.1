
#include <iostream>
#include "../include/tokens.h"
#include "../include/strings.h"

Token::Token(){
    id = 0;
    data = NULL;
}

Token::Token(const Token& other){
    id = other.id;

    switch(other.id){
    case TOKENID_STRING:
    case TOKENID_WORD:
    case TOKENID_KEYWORD:
        data = new std::string( *(static_cast<std::string*>(other.data)) );
        break;
    case TOKENID_INT:
        data = new int32_t( *(static_cast<int32_t*>(other.data)) );
        break;
    case TOKENID_FLOAT:
        data = new double( *(static_cast<double*>(other.data)) );
        break;
    default:
        data = NULL;
    }
}

Token::Token(uint16_t i){
    id = i;
    data = NULL;
}

Token::Token(uint16_t i, void* d){
    id = i;
    data = d;
}

Token::~Token(){
    free();
}

void Token::free(){
    switch(id){
    case TOKENID_STRING:
    case TOKENID_WORD:
    case TOKENID_KEYWORD:
        delete static_cast<std::string*>(data);
        break;
    }

    data = NULL;
}

void Token::reset(){
    free();
    id = 0;
}

std::string Token::getString(){
    std::string str;

    switch(id){
    case 0:
        str = "none";
        break;
    case 1:
        str = "string : ";
        str += *( static_cast<std::string*>(data) );
        break;
    case 2:
        str = "int : ";
        str += to_str( *( static_cast<int32_t*>(data) ) );
        break;
    case 3:
        str = "float : ";
        str += to_str( *( static_cast<double*>(data) ) );
        break;
    case 4:
        str = "word : ";
        str += *( static_cast<std::string*>(data) );
        break;
    case 5:
        str = "keyword : ";
        str += *( static_cast<std::string*>(data) );
        break;
    case 6:
        str = "open";
        break;
    case 7:
        str = "close";
        break;
    case 8:
        str = "begin";
        break;
    case 9:
        str = "end";
        break;
    case 10:
        str = "return indicator";
        break;
    case 11:
        str = "newline";
        break;
    case 12:
        str = "member";
        break;
    case 13:
        str = "namespace";
        break;
    case 14:
        str = "type indicator";
        break;
    case 15:
        str = "add";
        break;
    case 16:
        str = "subtract";
        break;
    case 17:
        str = "multiply";
        break;
    case 18:
        str = "divide";
        break;
    case 19:
        str = "assign";
        break;
    case 20:
        str = "next";
        break;
    }

    return str;
}
