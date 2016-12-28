
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
    case TOKENID_CONSTANT:
        data = new std::string( *(static_cast<std::string*>(other.data)) );
        break;
    case TOKENID_BYTE:
        data = new int8_t( *(static_cast<int8_t*>(other.data)) );
        break;
    case TOKENID_SHORT:
        data = new int16_t( *(static_cast<int16_t*>(other.data)) );
        break;
    case TOKENID_INT:
        data = new int32_t( *(static_cast<int32_t*>(other.data)) );
        break;
    case TOKENID_LONG:
        data = new int64_t( *(static_cast<int64_t*>(other.data)) );
        break;
    case TOKENID_UBYTE:
        data = new uint8_t( *(static_cast<uint8_t*>(other.data)) );
        break;
    case TOKENID_USHORT:
        data = new uint16_t( *(static_cast<uint16_t*>(other.data)) );
        break;
    case TOKENID_UINT:
        data = new uint32_t( *(static_cast<uint32_t*>(other.data)) );
        break;
    case TOKENID_ULONG:
        data = new uint64_t( *(static_cast<uint64_t*>(other.data)) );
        break;
    case TOKENID_FLOAT:
        data = new float( *(static_cast<float*>(other.data)) );
        break;
    case TOKENID_DOUBLE:
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
    case TOKENID_CONSTANT:
        delete static_cast<std::string*>(data);
        break;
    case TOKENID_BYTE:
        delete static_cast<int8_t*>(data);
        break;
    case TOKENID_SHORT:
        delete static_cast<int16_t*>(data);
        break;
    case TOKENID_INT:
        delete static_cast<int32_t*>(data);
        break;
    case TOKENID_LONG:
        delete static_cast<int64_t*>(data);
        break;
    case TOKENID_UBYTE:
        delete static_cast<uint8_t*>(data);
        break;
    case TOKENID_USHORT:
        delete static_cast<uint16_t*>(data);
        break;
    case TOKENID_UINT:
        delete static_cast<uint32_t*>(data);
        break;
    case TOKENID_ULONG:
        delete static_cast<uint64_t*>(data);
        break;
    case TOKENID_FLOAT:
        delete static_cast<float*>(data);
        break;
    case TOKENID_DOUBLE:
        delete static_cast<double*>(data);
        break;
    }

    data = NULL;
}

void Token::reset(){
    free();
    id = 0;
}

std::string Token::getString(){
    return *( static_cast<std::string*>(data) );
}
int8_t Token::getByte(){
    return *( static_cast<int8_t*>(data) );
}
int16_t Token::getShort(){
    return *( static_cast<int16_t*>(data) );
}
int32_t Token::getInt(){
    return *( static_cast<int32_t*>(data) );
}
int64_t Token::getLong(){
    return *( static_cast<int64_t*>(data) );
}
uint8_t Token::getUByte(){
    return *( static_cast<uint8_t*>(data) );
}
uint16_t Token::getUShort(){
    return *( static_cast<uint16_t*>(data) );
}
uint32_t Token::getUInt(){
    return *( static_cast<uint32_t*>(data) );
}
uint64_t Token::getULong(){
    return *( static_cast<uint64_t*>(data) );
}
float Token::getFloat(){
    return *( static_cast<float*>(data) );
}
double Token::getDouble(){
    return *( static_cast<double*>(data) );
}

std::string Token::toString(){
    std::string str;

    switch(id){
    // Literals
    case TOKENID_NONE:
        str = "none";
        break;
    case TOKENID_STRING:
        str = "string : \"" + getString() + "\"";
        break;
    case TOKENID_BYTE:
        str = "byte : " + to_str(getByte());
        break;
    case TOKENID_SHORT:
        str = "short : " + to_str(getShort());
        break;
    case TOKENID_INT:
        str = "int : " + to_str(getInt());
        break;
    case TOKENID_LONG:
        str = "long : " + to_str(getLong());
        break;
    case TOKENID_UBYTE:
        str = "unsigned byte : " + to_str(getUByte());
        break;
    case TOKENID_USHORT:
        str = "unsigned short : " + to_str(getUShort());
        break;
    case TOKENID_UINT:
        str = "unsigned int : " + to_str(getUInt());
        break;
    case TOKENID_ULONG:
        str = "unsigned long : " + to_str(getULong());
        break;
    case TOKENID_CONSTANT:
        str = "constant : " + getString();
        break;
    // Control Flow
    case TOKENID_WORD:
        str = "word : \"" + getString() + "\"";
        break;
    case TOKENID_KEYWORD:
        str = "keyword : \"" + getString() + "\"";
        break;
    case TOKENID_OPEN:
        str = "open";
        break;
    case TOKENID_CLOSE:
        str = "close";
        break;
    case TOKENID_BEGIN:
        str = "begin";
        break;
    case TOKENID_END:
        str = "end";
        break;
    case TOKENID_NEWLINE:
        str = "newline";
        break;
    case TOKENID_MEMBER:
        str = "member";
        break;
    case TOKENID_BRACKET_OPEN:
        str = "bracket open";
        break;
    case TOKENID_BRACKET_CLOSE:
        str = "bracket close";
        break;
    // Operators
    case TOKENID_ADD:
        str = "add";
        break;
    case TOKENID_SUBTRACT:
        str = "subtract";
        break;
    case TOKENID_MULTIPLY:
        str = "multiply/pointer";
        break;
    case TOKENID_DIVIDE:
        str = "divide";
        break;
    case TOKENID_ASSIGN:
        str = "assign";
        break;
    case TOKENID_NEXT:
        str = "next";
        break;
    case TOKENID_ADDRESS:
        str = "address";
        break;
    case TOKENID_EQUALITY:
        str = "equality";
        break;
    case TOKENID_INEQUALITY:
        str = "inequality";
        break;
    }

    return str;
}

int Token::getPrecedence(){
    switch(id){
    // Low Precedence
    case TOKENID_AND:
    case TOKENID_OR:
        return 1;
    case TOKENID_EQUALITY:
    case TOKENID_INEQUALITY:
        return 2;
    case TOKENID_ADD:
    case TOKENID_SUBTRACT:
        return 3;
    case TOKENID_MULTIPLY:
    case TOKENID_DIVIDE:
        return 4;
    // High Precedence
    case TOKENID_MEMBER:
        return 5;
    // No Precedence
    default:
        return 0;
    }
}
