#include "SemanticTable.h"

// Determina o tipo resultante de uma operação binária.
DataType SemanticTable::resultType(
    DataType left,
    DataType right,
    BinaryOperator operation
    )
{
    switch (operation)
    {
    // Soma valores numéricos ou concatena textos.
    case BinaryOperator::Add:
        if (isNumeric(left) &&
            isNumeric(right))
        {
            return numericResult(
                left,
                right
                );
        }

        if (isTextual(left) &&
            isTextual(right))
        {
            return DataType::Text;
        }

        return DataType::Unknown;

    // Subtração e multiplicação exigem operandos numéricos.
    case BinaryOperator::Subtract:
    case BinaryOperator::Multiply:
        if (isNumeric(left) &&
            isNumeric(right))
        {
            return numericResult(
                left,
                right
                );
        }

        return DataType::Unknown;

    // Divisão entre números sempre produz um valor flutuante.
    case BinaryOperator::Divide:
        if (isNumeric(left) &&
            isNumeric(right))
        {
            return DataType::Float;
        }

        return DataType::Unknown;

    // Módulo é permitido somente entre inteiros.
    case BinaryOperator::Modulo:
        if (left == DataType::Integer &&
            right == DataType::Integer)
        {
            return DataType::Integer;
        }

        return DataType::Unknown;

    // Operações relacionais entre números produzem valores lógicos.
    case BinaryOperator::Relational:
        if (isNumeric(left) &&
            isNumeric(right))
        {
            return DataType::Logical;
        }

        return DataType::Unknown;

    // Igualdade pode comparar números ou valores do mesmo tipo.
    case BinaryOperator::Equality:
        if (isNumeric(left) &&
            isNumeric(right))
        {
            return DataType::Logical;
        }

        if (left == right &&
            isLanguageType(left))
        {
            return DataType::Logical;
        }

        return DataType::Unknown;

    // Operadores lógicos exigem operandos lógicos.
    case BinaryOperator::LogicalAnd:
    case BinaryOperator::LogicalOr:
        if (left == DataType::Logical &&
            right == DataType::Logical)
        {
            return DataType::Logical;
        }

        return DataType::Unknown;
    }

    return DataType::Unknown;
}

// Determina o tipo resultante de uma operação unária.
DataType SemanticTable::resultType(
    DataType operand,
    UnaryOperator operation
    )
{
    switch (operation)
    {
    // Negação é permitida para inteiros e números flutuantes.
    case UnaryOperator::Negate:
        if (operand == DataType::Integer ||
            operand == DataType::Float)
        {
            return operand;
        }

        return DataType::Unknown;

    // Negação lógica exige um valor lógico.
    case UnaryOperator::LogicalNot:
        if (operand == DataType::Logical)
        {
            return DataType::Logical;
        }

        return DataType::Unknown;
    }

    return DataType::Unknown;
}

// Verifica a compatibilidade entre tipos em uma atribuição.
CompatibilityResult SemanticTable::atribType(
    DataType destination,
    DataType source
    )
{
    // Tipos desconhecidos ou inválidos não são compatíveis.
    if (!isLanguageType(destination) ||
        !isLanguageType(source))
    {
        return CompatibilityResult::Error;
    }

    // Tipos iguais são diretamente compatíveis.
    if (destination == source)
    {
        return CompatibilityResult::Ok;
    }

    // Conversão implícita de inteiro para flutuante é permitida.
    if (destination == DataType::Float &&
        source == DataType::Integer)
    {
        return CompatibilityResult::Ok;
    }

    // Conversão de flutuante para inteiro pode causar perda de dados.
    if (destination == DataType::Integer &&
        source == DataType::Float)
    {
        return CompatibilityResult::Warning;
    }

    return CompatibilityResult::Error;
}

// Retorna o nome textual de um tipo.
std::string SemanticTable::typeName(
    DataType type
    )
{
    switch (type)
    {
    case DataType::Integer:
        return "INTEIRO";

    case DataType::Float:
        return "FLUTUANTE";

    case DataType::Text:
        return "TEXTO";

    case DataType::Logical:
        return "LOGICO";

    case DataType::Character:
        return "CARACTERE";

    case DataType::Void:
        return "VAZIO";

    case DataType::Unknown:
        return "DESCONHECIDO";
    }

    return "DESCONHECIDO";
}

// Retorna o nome textual de um resultado de compatibilidade.
std::string SemanticTable::compatibilityName(
    CompatibilityResult result
    )
{
    switch (result)
    {
    case CompatibilityResult::Ok:
        return "OK";

    case CompatibilityResult::Warning:
        return "AVISO";

    case CompatibilityResult::Error:
        return "ERRO";
    }

    return "ERRO";
}

// Retorna o nome textual de um operador binário.
std::string SemanticTable::operatorName(
    BinaryOperator operation
    )
{
    switch (operation)
    {
    case BinaryOperator::Add:
        return "mais";

    case BinaryOperator::Subtract:
        return "menos";

    case BinaryOperator::Multiply:
        return "vezes";

    case BinaryOperator::Divide:
        return "divido";

    case BinaryOperator::Modulo:
        return "resto";

    case BinaryOperator::Relational:
        return "relacional";

    case BinaryOperator::Equality:
        return "igualdade";

    case BinaryOperator::LogicalAnd:
        return "e";

    case BinaryOperator::LogicalOr:
        return "ou";
    }

    return "desconhecido";
}

// Retorna o nome textual de um operador unário.
std::string SemanticTable::operatorName(
    UnaryOperator operation
    )
{
    switch (operation)
    {
    case UnaryOperator::Negate:
        return "menos";

    case UnaryOperator::LogicalNot:
        return "nao";
    }

    return "desconhecido";
}

// Verifica se o tipo é numérico.
bool SemanticTable::isNumeric(
    DataType type
    )
{
    return
        type == DataType::Integer ||
        type == DataType::Float;
}

// Verifica se o tipo representa texto ou caractere.
bool SemanticTable::isTextual(
    DataType type
    )
{
    return
        type == DataType::Text ||
        type == DataType::Character;
}

// Verifica se o tipo pertence aos tipos válidos da linguagem.
bool SemanticTable::isLanguageType(
    DataType type
    )
{
    return
        type == DataType::Integer ||
        type == DataType::Float ||
        type == DataType::Text ||
        type == DataType::Logical ||
        type == DataType::Character;
}

// Determina o resultado de uma operação entre tipos numéricos.
DataType SemanticTable::numericResult(
    DataType left,
    DataType right
    )
{
    // Se um dos operandos for flutuante, o resultado também será.
    if (left == DataType::Float ||
        right == DataType::Float)
    {
        return DataType::Float;
    }

    return DataType::Integer;
}
