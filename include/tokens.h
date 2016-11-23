
#ifndef TOKENS_H_INCLUDED
#define TOKENS_H_INCLUDED

#include <string>
#include <vector>

#define TOKEN_NONE       Token(0)
#define TOKEN_STRING(a)  Token(1, a)
#define TOKEN_INT(a)     Token(2, new int32_t(a))
#define TOKEN_FLOAT(a)   Token(3, new double(a))
#define TOKEN_WORD(a)    Token(4, a)
#define TOKEN_KEYWORD(a) Token(5, a)
#define TOKEN_OPEN       Token(6)
#define TOKEN_CLOSE      Token(7)
#define TOKEN_BEGIN      Token(8)
#define TOKEN_END        Token(9)
// #define TOKEN_RETIND     Token(10)
#define TOKEN_NEWLINE    Token(11)
#define TOKEN_MEMBER     Token(12)
// #define TOKEN_NAMESPACE  Token(13)
// #define TOKEN_TYPEIND    Token(14)
#define TOKEN_ADD        Token(15)
#define TOKEN_SUBTRACT   Token(16)
#define TOKEN_MULTIPLY   Token(17)
#define TOKEN_DIVIDE     Token(18)
#define TOKEN_ASSIGN     Token(19)
#define TOKEN_NEXT       Token(20)

#define TOKENID_NONE      0
#define TOKENID_STRING    1
#define TOKENID_INT       2
#define TOKENID_FLOAT     3
#define TOKENID_WORD      4
#define TOKENID_KEYWORD   5
#define TOKENID_OPEN      6
#define TOKENID_CLOSE     7
#define TOKENID_BEGIN     8
#define TOKENID_END       9
// #define TOKENID_RETIND    10
#define TOKENID_NEWLINE   11
#define TOKENID_MEMBER    12
// #define TOKENID_NAMESPACE 13
// #define TOKENID_TYPEIND   14
#define TOKENID_ADD       15
#define TOKENID_SUBTRACT  16
#define TOKENID_MULTIPLY  17
#define TOKENID_DIVIDE    18
#define TOKENID_ASSIGN    19
#define TOKENID_NEXT      20

struct Token {
    uint16_t id;
    void* data;

    Token();
    Token(const Token&);
    Token(uint16_t);
    Token(uint16_t, void*);
    ~Token();
    void reset();
    void free();
    std::string getString();
    int32_t getInt();
    double getFloat();
    std::string toString();
    int getPrecedence();
};

typedef std::vector<Token> TokenList;

#endif // TOKENS_H_INCLUDED
