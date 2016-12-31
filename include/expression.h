
#ifndef EXPRESSION_H_INCLUDED
#define EXPRESSION_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "errors.h"
#include "asmcontext.h"

struct Program;
struct Function;

class PlainExp {
    public:
    // Filename & line in case of error
    ErrorHandler errors;

    PlainExp();
    PlainExp(const PlainExp&);
    PlainExp(ErrorHandler&);
    virtual ~PlainExp();
    virtual llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*) = 0;
    virtual std::string toString() = 0;
    virtual PlainExp* clone() = 0;
};

class OperatorExp : public PlainExp {
    public:
    uint16_t operation; // Uses token id
    PlainExp* left;
    PlainExp* right;

    OperatorExp(ErrorHandler&);
    OperatorExp(const OperatorExp&);
    OperatorExp(uint16_t, PlainExp*, PlainExp*, ErrorHandler&);
    ~OperatorExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class BoolExp : public PlainExp {
    public:
    bool value;

    BoolExp(ErrorHandler&);
    BoolExp(bool, ErrorHandler&);
    ~BoolExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class ByteExp : public PlainExp {
    public:
    int8_t value;

    ByteExp(ErrorHandler&);
    ByteExp(int8_t, ErrorHandler&);
    ~ByteExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class ShortExp : public PlainExp {
    public:
    int16_t value;

    ShortExp(ErrorHandler&);
    ShortExp(int16_t, ErrorHandler&);
    ~ShortExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class IntegerExp : public PlainExp {
    public:
    int32_t value;

    IntegerExp(ErrorHandler&);
    IntegerExp(int32_t, ErrorHandler&);
    ~IntegerExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class LongExp : public PlainExp {
    public:
    int64_t value;

    LongExp(ErrorHandler&);
    LongExp(int64_t, ErrorHandler&);
    ~LongExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedByteExp : public PlainExp {
    public:
    uint8_t value;

    UnsignedByteExp(ErrorHandler&);
    UnsignedByteExp(uint8_t, ErrorHandler&);
    ~UnsignedByteExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedShortExp : public PlainExp {
    public:
    uint16_t value;

    UnsignedShortExp(ErrorHandler&);
    UnsignedShortExp(uint16_t, ErrorHandler&);
    ~UnsignedShortExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedIntegerExp : public PlainExp {
    public:
    uint32_t value;

    UnsignedIntegerExp(ErrorHandler&);
    UnsignedIntegerExp(uint32_t, ErrorHandler&);
    ~UnsignedIntegerExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class UnsignedLongExp : public PlainExp {
    public:
    uint64_t value;

    UnsignedLongExp(ErrorHandler&);
    UnsignedLongExp(uint64_t, ErrorHandler&);
    ~UnsignedLongExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class FloatExp : public PlainExp {
    public:
    float value;

    FloatExp(ErrorHandler&);
    FloatExp(float, ErrorHandler&);
    ~FloatExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class DoubleExp : public PlainExp {
    public:
    double value;

    DoubleExp(ErrorHandler&);
    DoubleExp(double, ErrorHandler&);
    ~DoubleExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class StringExp : public PlainExp {
    public:
    std::string value;

    StringExp(ErrorHandler&);
    StringExp(const std::string&, ErrorHandler&);
    ~StringExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class WordExp : public PlainExp {
    public:
    std::string value;

    WordExp(ErrorHandler&);
    WordExp(const std::string&, ErrorHandler&);
    ~WordExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class AddrWordExp : public PlainExp {
    public:
    std::string value;

    AddrWordExp(ErrorHandler&);
    AddrWordExp(const std::string&, ErrorHandler&);
    ~AddrWordExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class LoadExp : public PlainExp {
    public:
    PlainExp* value;

    LoadExp(ErrorHandler&);
    LoadExp(PlainExp*, ErrorHandler&);
    LoadExp(const LoadExp&);
    ~LoadExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class IndexLoadExp : public PlainExp {
    public:
    PlainExp* value;
    PlainExp* index;

    IndexLoadExp(ErrorHandler&);
    IndexLoadExp(PlainExp*, PlainExp*, ErrorHandler&);
    IndexLoadExp(const IndexLoadExp&);
    ~IndexLoadExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class CallExp : public PlainExp {
    public:
    std::string name;
    std::vector<PlainExp*> args;

    CallExp(ErrorHandler&);
    CallExp(const CallExp&);
    CallExp(std::string, const std::vector<PlainExp*>&, ErrorHandler&);
    ~CallExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class MemberExp : public PlainExp {
    public:
    PlainExp* value;
    std::string member;

    MemberExp(ErrorHandler&);
    MemberExp(PlainExp*, const std::string&, ErrorHandler&);
    ~MemberExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class NullExp : public PlainExp {
    public:
    NullExp(ErrorHandler&);
    ~NullExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class NotExp : public PlainExp {
    public:
    PlainExp* value;

    NotExp(ErrorHandler&);
    NotExp(PlainExp*, ErrorHandler&);
    NotExp(const NotExp&);
    ~NotExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);
};

class CastExp : public PlainExp {
    public:
    PlainExp* value;
    std::string target_typename;

    CastExp(ErrorHandler&);
    CastExp(PlainExp*, std::string, ErrorHandler&);
    CastExp(const CastExp&);
    ~CastExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
    bool getType(Program&, Function&, std::string&);

    private:
    llvm::Value* cast_to_bool(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_byte(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_short(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_int(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_long(Program&, Function&, AssembleContext&);
};

#endif // EXPRESSION_H_INCLUDED
