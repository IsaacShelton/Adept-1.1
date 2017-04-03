
#include <iostream>
#include "../include/tokens.h"
#include "../include/strings.h"

Token::Token(){
    id = 0;
    data = NULL;
}

Token::Token(const Token& other){
    id = other.id;
    data = other.data;
}

Token::Token(uint16_t i){
    id = i;
    data = NULL;
}

Token::Token(uint16_t i, void* d){
    id = i;
    data = d;
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
    case TOKENID_FLOAT:
        str = "float: " + to_str(getFloat());
        break;
    case TOKENID_DOUBLE:
        str = "double : " + to_str(getDouble());
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
    case TOKENID_ASSIGNADD:
        str = "addition assign";
        break;
    case TOKENID_ASSIGNSUB:
        str = "subtraction assign";
        break;
    case TOKENID_ASSIGNMUL:
        str = "multiplication assign";
        break;
    case TOKENID_ASSIGNDIV:
        str = "division assign";
        break;
    case TOKENID_ASSIGNMOD:
        str = "modulus assign";
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
    case TOKENID_NOT:
        str = "not";
        break;
    case TOKENID_AND:
        str = "and";
        break;
    case TOKENID_OR:
        str = "or";
        break;
    case TOKENID_MODULUS:
        str = "modulus";
        break;
    case TOKENID_LESS:
        str = "less than";
        break;
    case TOKENID_GREATER:
        str = "greater than";
        break;
    case TOKENID_LESSEQ:
        str = "less than or equal";
        break;
    case TOKENID_GREATEREQ:
        str = "greater than or equal";
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
    case TOKENID_LESS:
    case TOKENID_GREATER:
    case TOKENID_LESSEQ:
    case TOKENID_GREATEREQ:
        return 2;
    case TOKENID_ADD:
    case TOKENID_SUBTRACT:
    case TOKENID_WORD:
        return 3;
    case TOKENID_MULTIPLY:
    case TOKENID_DIVIDE:
    case TOKENID_MODULUS:
        return 4;
    // High Precedence
    case TOKENID_MEMBER:
    case TOKENID_BRACKET_OPEN:
        return 5;
    // No Precedence
    default:
        return 0;
    }
}

std::string Token::syntax(){
    switch(id){
    // Literals
    case TOKENID_NONE:
        return "none";
        break;
    case TOKENID_STRING:
        return "\"" + this->getString() + "\"";
        break;
    case TOKENID_BYTE:
        return to_str(this->getByte()) + "sb";
        break;
    case TOKENID_SHORT:
        return to_str(this->getShort()) + "ss";
        break;
    case TOKENID_INT:
        return to_str(this->getInt());
        break;
    case TOKENID_LONG:
        return to_str(this->getShort()) + "sl";
        break;
    case TOKENID_UBYTE:
        return to_str(this->getUByte()) + "ub";
        break;
    case TOKENID_USHORT:
        return to_str(this->getUShort()) + "us";
        break;
    case TOKENID_UINT:
        return to_str(this->getUInt()) + "u";
        break;
    case TOKENID_ULONG:
        return to_str(this->getULong()) + "ul";
        break;
    case TOKENID_CONSTANT:
        return "$" + getString();
        break;
    // Control Flow
    case TOKENID_WORD:
        return getString();
        break;
    case TOKENID_KEYWORD:
        return getString();
        break;
    case TOKENID_OPEN:
        return "(";
        break;
    case TOKENID_CLOSE:
        return ")";
        break;
    case TOKENID_BEGIN:
        return "{";
        break;
    case TOKENID_END:
        return "}";
        break;
    case TOKENID_NEWLINE:
        return "<newline>";
        break;
    case TOKENID_MEMBER:
        return ".";
        break;
    case TOKENID_BRACKET_OPEN:
        return "[";
        break;
    case TOKENID_BRACKET_CLOSE:
        return "]";
        break;
    // Operators
    case TOKENID_ADD:
        return "+";
        break;
    case TOKENID_SUBTRACT:
        return "-";
        break;
    case TOKENID_MULTIPLY:
        return "*";
        break;
    case TOKENID_DIVIDE:
        return "/";
        break;
    case TOKENID_ASSIGN:
        return "=";
        break;
    case TOKENID_ASSIGNADD:
        return "+=";
        break;
    case TOKENID_ASSIGNSUB:
        return "-=";
        break;
    case TOKENID_ASSIGNMUL:
        return "*=";
        break;
    case TOKENID_ASSIGNDIV:
        return "/=";
        break;
    case TOKENID_ASSIGNMOD:
        return "%=";
        break;
    case TOKENID_NEXT:
        return ",";
        break;
    case TOKENID_ADDRESS:
        return "&";
        break;
    case TOKENID_EQUALITY:
        return "==";
        break;
    case TOKENID_INEQUALITY:
        return "!=";
        break;
    case TOKENID_NOT:
        return "!";
        break;
    case TOKENID_AND:
        return "and";
        break;
    case TOKENID_OR:
        return "or";
        break;
    case TOKENID_MODULUS:
        return "%";
        break;
    case TOKENID_LESS:
        return "<";
        break;
    case TOKENID_GREATER:
        return ">";
        break;
    case TOKENID_LESSEQ:
        return "<=";
        break;
    case TOKENID_GREATEREQ:
        return ">=";
        break;
    }

    return "";
}

std::string get_tokenid_syntax(uint16_t id){
    switch(id){
    // Literals
    case TOKENID_NONE:
        return "";
        break;
    case TOKENID_STRING:
        return "\"\"";
        break;
    case TOKENID_BYTE:
        return "<byte>";
        break;
    case TOKENID_SHORT:
        return "<short>";
        break;
    case TOKENID_INT:
        return "<int>";
        break;
    case TOKENID_LONG:
        return "<long>";
        break;
    case TOKENID_UBYTE:
        return "<ubyte>";
        break;
    case TOKENID_USHORT:
        return "<ushort>";
        break;
    case TOKENID_UINT:
        return "<uint>";
        break;
    case TOKENID_ULONG:
        return "<ulong>";
        break;
    case TOKENID_CONSTANT:
        return "$";
        break;
    // Control Flow
    case TOKENID_WORD:
        return "<word>";
        break;
    case TOKENID_KEYWORD:
        return "<keyword>";
        break;
    case TOKENID_OPEN:
        return "(";
        break;
    case TOKENID_CLOSE:
        return ")";
        break;
    case TOKENID_BEGIN:
        return "{";
        break;
    case TOKENID_END:
        return "}";
        break;
    case TOKENID_NEWLINE:
        return "<newline>";
        break;
    case TOKENID_MEMBER:
        return ".";
        break;
    case TOKENID_BRACKET_OPEN:
        return "[";
        break;
    case TOKENID_BRACKET_CLOSE:
        return "]";
        break;
    // Operators
    case TOKENID_ADD:
        return "+";
        break;
    case TOKENID_SUBTRACT:
        return "-";
        break;
    case TOKENID_MULTIPLY:
        return "*";
        break;
    case TOKENID_DIVIDE:
        return "/";
        break;
    case TOKENID_ASSIGN:
        return "=";
        break;
    case TOKENID_ASSIGNADD:
        return "+=";
        break;
    case TOKENID_ASSIGNSUB:
        return "-=";
        break;
    case TOKENID_ASSIGNMUL:
        return "*=";
        break;
    case TOKENID_ASSIGNDIV:
        return "/=";
        break;
    case TOKENID_ASSIGNMOD:
        return "%=";
        break;
    case TOKENID_NEXT:
        return ",";
        break;
    case TOKENID_ADDRESS:
        return "&";
        break;
    case TOKENID_EQUALITY:
        return "==";
        break;
    case TOKENID_INEQUALITY:
        return "!=";
        break;
    case TOKENID_NOT:
        return "!";
        break;
    case TOKENID_AND:
        return "and";
        break;
    case TOKENID_OR:
        return "or";
        break;
    case TOKENID_MODULUS:
        return "%";
        break;
    case TOKENID_LESS:
        return "<";
        break;
    case TOKENID_GREATER:
        return ">";
        break;
    case TOKENID_LESSEQ:
        return "<=";
        break;
    case TOKENID_GREATEREQ:
        return ">=";
        break;
    }

    return "";
}

void free_tokens(TokenList& tokens){
    for(Token& token : tokens){
        token.free();
    }
}
