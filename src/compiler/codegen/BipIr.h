
#ifndef BIP_IR_H
#define BIP_IR_H

#include "AnalysisResult.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace bip
{

enum class Operation
{
    Literal,
    Variable,
    VectorElement,
    Add,
    Subtract,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    BitwiseNot,
    Negate,
    ShiftLeft,
    ShiftRight
};

struct Expression;

using ExpressionPtr =
    std::shared_ptr<const Expression>;

struct Expression
{
    Operation operation = Operation::Literal;

    int value = 0;
    std::string name;

    ExpressionPtr left;
    ExpressionPtr right;
    ExpressionPtr index;
};

struct Declaration
{
    std::string name;
    std::size_t size = 1;
    bool isVector = false;
};

enum class StatementKind
{
    Assignment,
    Input,
    Output
};

struct Statement
{
    StatementKind kind = StatementKind::Assignment;

    std::string destination;
    ExpressionPtr destinationIndex;
    ExpressionPtr expression;
};

struct Program
{
    std::vector<Declaration> declarations;
    std::vector<Statement> statements;
};

class CodegenError : public std::runtime_error
{
public:
    explicit CodegenError(
        const std::string &message,
        int position = -1
        )
        : std::runtime_error(message),
        position_(position)
    {
    }

    int position() const noexcept
    {
        return position_;
    }

private:
    int position_;
};

class BipIrBuilder
{
public:
    Program build(
        const std::string &source,
        const AnalysisResult &analysis
        ) const;
};

class BipEmitter
{
public:
    std::string emit(const Program &program) const;
};

}

#endif
