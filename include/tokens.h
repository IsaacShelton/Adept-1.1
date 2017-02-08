
#ifndef TOKENS_H_INCLUDED
#define TOKENS_H_INCLUDED

#include <string>
#include <vector>

// 0-19 : Literals (13 / 20 Used)
#define TOKEN_NONE          Token(0)
#define TOKEN_STRING(a)     Token(1, a)
#define TOKEN_BYTE(a)       Token(2, new int8_t(a))
#define TOKEN_SHORT(a)      Token(3, new int16_t(a))
#define TOKEN_INT(a)        Token(4, new int32_t(a))
#define TOKEN_LONG(a)       Token(5, new int64_t(a))
#define TOKEN_UBYTE(a)      Token(6, new uint8_t(a))
#define TOKEN_USHORT(a)     Token(7, new uint16_t(a))
#define TOKEN_UINT(a)       Token(8, new uint32_t(a))
#define TOKEN_ULONG(a)      Token(9, new uint64_t(a))
#define TOKEN_FLOAT(a)      Token(10, new float(a))
#define TOKEN_DOUBLE(a)     Token(11, new double(a))
#define TOKEN_CONSTANT(a)   Token(12, a)
// 20-39 : Control Flow (10 / 20 Used)
#define TOKEN_WORD(a)       Token(20, a)
#define TOKEN_KEYWORD(a)    Token(21, a)
#define TOKEN_OPEN          Token(22)
#define TOKEN_CLOSE         Token(23)
#define TOKEN_BEGIN         Token(24)
#define TOKEN_END           Token(25)
#define TOKEN_NEWLINE       Token(26)
#define TOKEN_MEMBER        Token(27)
#define TOKEN_BRACKET_OPEN  Token(28)
#define TOKEN_BRACKET_CLOSE Token(29)
// 40-59 : Operators (17 / 20 Used)
#define TOKEN_ADD           Token(40)
#define TOKEN_SUBTRACT      Token(41)
#define TOKEN_MULTIPLY      Token(42)
#define TOKEN_DIVIDE        Token(43)
#define TOKEN_ASSIGN        Token(44)
#define TOKEN_NEXT          Token(45)
#define TOKEN_ADDRESS       Token(46)
#define TOKEN_EQUALITY      Token(47)
#define TOKEN_INEQUALITY    Token(48)
#define TOKEN_NOT           Token(49)
#define TOKEN_AND           Token(50)
#define TOKEN_OR            Token(51)
#define TOKEN_MODULUS       Token(52)
#define TOKEN_LESS          Token(53)
#define TOKEN_GREATER       Token(54)
#define TOKEN_LESSEQ        Token(55)
#define TOKEN_GREATEREQ     Token(56)

// Indices : Literals 0-19
#define TOKENID_NONE          0
#define TOKENID_STRING        1
#define TOKENID_BYTE          2
#define TOKENID_SHORT         3
#define TOKENID_INT           4
#define TOKENID_LONG          5
#define TOKENID_UBYTE         6
#define TOKENID_USHORT        7
#define TOKENID_UINT          8
#define TOKENID_ULONG         9
#define TOKENID_FLOAT         10
#define TOKENID_DOUBLE        11
#define TOKENID_CONSTANT      12
// Indices : Control Flow 20-39
#define TOKENID_WORD          20
#define TOKENID_KEYWORD       21
#define TOKENID_OPEN          22
#define TOKENID_CLOSE         23
#define TOKENID_BEGIN         24
#define TOKENID_END           25
#define TOKENID_NEWLINE       26
#define TOKENID_MEMBER        27
#define TOKENID_BRACKET_OPEN  28
#define TOKENID_BRACKET_CLOSE 29
// Indices : Operators 40-59
#define TOKENID_ADD           40
#define TOKENID_SUBTRACT      41
#define TOKENID_MULTIPLY      42
#define TOKENID_DIVIDE        43
#define TOKENID_ASSIGN        44
#define TOKENID_NEXT          45
#define TOKENID_ADDRESS       46
#define TOKENID_EQUALITY      47
#define TOKENID_INEQUALITY    48
#define TOKENID_NOT           49
#define TOKENID_AND           50
#define TOKENID_OR            51
#define TOKENID_MODULUS       52
#define TOKENID_LESS          53
#define TOKENID_GREATER       54
#define TOKENID_LESSEQ        55
#define TOKENID_GREATEREQ     56

struct Token {
    uint16_t id;
    void* data;

    Token();
    Token(const Token&);
    Token(uint16_t);
    Token(uint16_t, void*);

    // Deallocation
    void reset();
    void free();

    // Cast data and return value
    std::string getString();
    int8_t getByte();
    int16_t getShort();
    int32_t getInt();
    int64_t getLong();
    uint8_t getUByte();
    uint16_t getUShort();
    uint32_t getUInt();
    uint64_t getULong();
    float getFloat();
    double getDouble();

    // Operator Precedence (only for certain tokens)
    int getPrecedence();

    // Debugging
    std::string toString();
};

typedef std::vector<Token> TokenList;

void free_tokens(TokenList& tokens);

#endif // TOKENS_H_INCLUDED
