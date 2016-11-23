
#ifndef EXPRESSION_H_INCLUDED
#define EXPRESSION_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "asmcontext.h"

struct Program;
struct Function;

class PlainExp {
    public:
    virtual ~PlainExp();
    virtual llvm::Value* assemble(Program&, Function&, AssembleContext&) = 0;
    virtual std::string toString() = 0;
    virtual PlainExp* clone() = 0;

    virtual bool getType(Program&, Function&, std::string&) = 0;
};

class OperatorExp : public PlainExp {
    public:
    uint16_t operation; // Uses token id
    PlainExp* left;
    PlainExp* right;

    OperatorExp();
    OperatorExp(const OperatorExp&);
    OperatorExp(uint16_t, PlainExp*, PlainExp*);
    ~OperatorExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class IntegerExp : public PlainExp {
    public:
    int32_t value;

    IntegerExp();
    IntegerExp(int32_t);
    ~IntegerExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class DoubleExp : public PlainExp {
    public:
    double value;

    DoubleExp();
    DoubleExp(double);
    ~DoubleExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class WordExp : public PlainExp {
    public:
    std::string value;

    WordExp();
    WordExp(std::string);
    ~WordExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class CallExp : public PlainExp {
    public:
    std::string name;
    std::vector<PlainExp*> args;

    CallExp();
    CallExp(const CallExp&);
    CallExp(std::string, const std::vector<PlainExp*>&);
    ~CallExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

#endif // EXPRESSION_H_INCLUDED
