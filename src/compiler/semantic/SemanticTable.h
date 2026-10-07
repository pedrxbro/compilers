#ifndef SEMANTIC_TABLE_H
#define SEMANTIC_TABLE_H

#include "SemanticTypes.h"

#include <string>

enum class CompatibilityResult
{
    Error = -1,
    Ok = 0,
    Warning = 1
};

enum class BinaryOperator
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Relational,
    Equality,
    LogicalAnd,
    LogicalOr
};

enum class UnaryOperator
{
    Negate,
    LogicalNot
};

class SemanticTable
{
public:
    static DataType resultType(
        DataType left,
        DataType right,
        BinaryOperator operation
        );

    static DataType resultType(
        DataType operand,
        UnaryOperator operation
        );

    static CompatibilityResult atribType(
        DataType destination,
        DataType source
        );

    static std::string typeName(
        DataType type
        );

    static std::string compatibilityName(
        CompatibilityResult result
        );

    static std::string operatorName(
        BinaryOperator operation
        );

    static std::string operatorName(
        UnaryOperator operation
        );

private:
    static bool isNumeric(
        DataType type
        );

    static bool isTextual(
        DataType type
        );

    static bool isLanguageType(
        DataType type
        );

    static DataType numericResult(
        DataType left,
        DataType right
        );
};

#endif