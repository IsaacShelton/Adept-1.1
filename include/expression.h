
#ifndef EXPRESSION_H_INCLUDED
#define EXPRESSION_H_INCLUDED

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "errors.h"
#include "asmcontext.h"

struct Program;
struct Function;
struct Structure;
struct Class;

class PlainExp {
    public:
    // Indicates whether or not this value can be modified
    bool is_mutable;

    // Filename & line in case of error
    ErrorHandler errors;

    PlainExp();
    PlainExp(const PlainExp&);
    PlainExp(ErrorHandler&);
    virtual ~PlainExp();
    virtual llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*) = 0;
    virtual std::string toString() = 0;
    virtual PlainExp* clone() = 0;

    llvm::Value* assemble_immutable(Program&, Function&, AssembleContext&, std::string*);
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
};

class BoolExp : public PlainExp {
    public:
    bool value;

    BoolExp(ErrorHandler&);
    BoolExp(bool, ErrorHandler&);
    BoolExp(const BoolExp&);
    ~BoolExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class ByteExp : public PlainExp {
    public:
    int8_t value;

    ByteExp(ErrorHandler&);
    ByteExp(int8_t, ErrorHandler&);
    ByteExp(const ByteExp&);
    ~ByteExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class ShortExp : public PlainExp {
    public:
    int16_t value;

    ShortExp(ErrorHandler&);
    ShortExp(int16_t, ErrorHandler&);
    ShortExp(const ShortExp&);
    ~ShortExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class IntegerExp : public PlainExp {
    public:
    int32_t value;

    IntegerExp(ErrorHandler&);
    IntegerExp(int32_t, ErrorHandler&);
    IntegerExp(const IntegerExp&);
    ~IntegerExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class LongExp : public PlainExp {
    public:
    int64_t value;

    LongExp(ErrorHandler&);
    LongExp(int64_t, ErrorHandler&);
    LongExp(const LongExp&);
    ~LongExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class UnsignedByteExp : public PlainExp {
    public:
    uint8_t value;

    UnsignedByteExp(ErrorHandler&);
    UnsignedByteExp(uint8_t, ErrorHandler&);
    UnsignedByteExp(const UnsignedByteExp&);
    ~UnsignedByteExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class UnsignedShortExp : public PlainExp {
    public:
    uint16_t value;

    UnsignedShortExp(ErrorHandler&);
    UnsignedShortExp(uint16_t, ErrorHandler&);
    UnsignedShortExp(const UnsignedShortExp&);
    ~UnsignedShortExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class UnsignedIntegerExp : public PlainExp {
    public:
    uint32_t value;

    UnsignedIntegerExp(ErrorHandler&);
    UnsignedIntegerExp(uint32_t, ErrorHandler&);
    UnsignedIntegerExp(const UnsignedIntegerExp&);
    ~UnsignedIntegerExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class UnsignedLongExp : public PlainExp {
    public:
    uint64_t value;

    UnsignedLongExp(ErrorHandler&);
    UnsignedLongExp(uint64_t, ErrorHandler&);
    UnsignedLongExp(const UnsignedLongExp&);
    ~UnsignedLongExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class FloatExp : public PlainExp {
    public:
    float value;

    FloatExp(ErrorHandler&);
    FloatExp(float, ErrorHandler&);
    FloatExp(const FloatExp&);
    ~FloatExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class DoubleExp : public PlainExp {
    public:
    double value;

    DoubleExp(ErrorHandler&);
    DoubleExp(double, ErrorHandler&);
    DoubleExp(const DoubleExp&);
    ~DoubleExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class StringExp : public PlainExp {
    public:
    std::string value;

    StringExp(ErrorHandler&);
    StringExp(const std::string&, ErrorHandler&);
    StringExp(const StringExp&);
    ~StringExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class WordExp : public PlainExp {
    public:
    std::string value;

    WordExp(ErrorHandler&);
    WordExp(const std::string&, ErrorHandler&);
    WordExp(const WordExp&);
    ~WordExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class AddrWordExp : public PlainExp {
    public:
    PlainExp* value;

    AddrWordExp(ErrorHandler&);
    AddrWordExp(PlainExp*, ErrorHandler&);
    AddrWordExp(const AddrWordExp&);
    ~AddrWordExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
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
};

class MemberExp : public PlainExp {
    public:
    PlainExp* value;
    std::string member;

    MemberExp(ErrorHandler&);
    MemberExp(PlainExp*, const std::string&, ErrorHandler&);
    MemberExp(const MemberExp&);
    ~MemberExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();

    private:
    llvm::Value* assemble_struct(Program&, Function&, AssembleContext&, std::string*, Structure&, llvm::Value*);
    llvm::Value* assemble_class(Program&, Function&, AssembleContext&, std::string*, Class&, llvm::Value*, std::string&);
};

class MemberCallExp : public PlainExp {
    public:
    PlainExp* object;
    std::string name;
    std::vector<PlainExp*> args;

    MemberCallExp(ErrorHandler&);
    MemberCallExp(PlainExp*, const std::string&, const std::vector<PlainExp*>&, ErrorHandler&);
    MemberCallExp(const MemberCallExp&);
    ~MemberCallExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

class NullExp : public PlainExp {
    public:
    NullExp(ErrorHandler&);
    NullExp(const NullExp&);
    ~NullExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
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

    private:
    llvm::Value* cast_to_bool(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_byte(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_short(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_int(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_long(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_float(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_double(Program&, Function&, AssembleContext&);
    llvm::Value* cast_to_ptr(Program&, Function&, AssembleContext&);
};

class FuncptrExp : public PlainExp {
    public:
    std::string function_name;
    std::vector<std::string> function_arguments;

    FuncptrExp(ErrorHandler&);
    FuncptrExp(const std::string&, const std::vector<std::string>&, ErrorHandler&);
    FuncptrExp(const FuncptrExp&);
    ~FuncptrExp();
    llvm::Value* assemble(Program&, Function&, AssembleContext&, std::string*);
    std::string toString();
    PlainExp* clone();
};

#endif // EXPRESSION_H_INCLUDED
