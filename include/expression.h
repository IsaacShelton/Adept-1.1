
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

class ByteExp : public PlainExp {
    public:
    int8_t value;

    ByteExp();
    ByteExp(int8_t);
    ~ByteExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class ShortExp : public PlainExp {
    public:
    int16_t value;

    ShortExp();
    ShortExp(int16_t);
    ~ShortExp();
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

class LongExp : public PlainExp {
    public:
    int64_t value;

    LongExp();
    LongExp(int64_t);
    ~LongExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedByteExp : public PlainExp {
    public:
    uint8_t value;

    UnsignedByteExp();
    UnsignedByteExp(uint8_t);
    ~UnsignedByteExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedShortExp : public PlainExp {
    public:
    uint16_t value;

    UnsignedShortExp();
    UnsignedShortExp(uint16_t);
    ~UnsignedShortExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedIntegerExp : public PlainExp {
    public:
    uint32_t value;

    UnsignedIntegerExp();
    UnsignedIntegerExp(uint32_t);
    ~UnsignedIntegerExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedLongExp : public PlainExp {
    public:
    uint64_t value;

    UnsignedLongExp();
    UnsignedLongExp(uint64_t);
    ~UnsignedLongExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class FloatExp : public PlainExp {
    public:
    float value;

    FloatExp();
    FloatExp(float);
    ~FloatExp();
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

class StringExp : public PlainExp {
    public:
    std::string value;

    StringExp();
    StringExp(const std::string&);
    ~StringExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class WordExp : public PlainExp {
    public:
    std::string value;

    WordExp();
    WordExp(const std::string&);
    ~WordExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class AddrWordExp : public PlainExp {
    public:
    std::string value;

    AddrWordExp();
    AddrWordExp(const std::string&);
    ~AddrWordExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class LoadExp : public PlainExp {
    public:
    PlainExp* value;

    LoadExp();
    LoadExp(PlainExp*);
    LoadExp(const LoadExp&);
    ~LoadExp();
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

class MemberExp : public PlainExp {
    public:
    PlainExp* value;
    std::string member;

    MemberExp();
    MemberExp(PlainExp*, const std::string&);
    ~MemberExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

#endif // EXPRESSION_H_INCLUDED
