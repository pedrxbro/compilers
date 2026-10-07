#include "CompilerService.h"
#include "SemanticTable.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

void require(
    bool condition,
    const std::string &message
    )
{
    if (!condition)
    {
        throw std::runtime_error(
            message
            );
    }
}

void expectSuccess(
    const CompilerService &compiler,
    const std::string &source,
    const std::string &testName
    )
{
    const AnalysisResult result =
        compiler.analyze(
            source
            );

    if (!result.success)
    {
        throw std::runtime_error(
            testName +
            " deveria compilar, mas retornou: " +
            result.message
            );
    }
}

void expectFailureContaining(
    const CompilerService &compiler,
    const std::string &source,
    const std::string &expectedText,
    const std::string &testName
    )
{
    const AnalysisResult result =
        compiler.analyze(
            source
            );

    if (result.success)
    {
        throw std::runtime_error(
            testName +
            " deveria falhar, mas compilou com sucesso."
            );
    }

    if (result.message.find(
            expectedText
            ) == std::string::npos)
    {
        throw std::runtime_error(
            testName +
            " falhou com uma mensagem inesperada: " +
            result.message
            );
    }
}

void expectSuccessContaining(
    const CompilerService &compiler,
    const std::string &source,
    const std::string &expectedText,
    const std::string &testName
    )
{
    const AnalysisResult result =
        compiler.analyze(
            source
            );

    if (!result.success)
    {
        throw std::runtime_error(
            testName +
            " deveria compilar, mas retornou: " +
            result.message
            );
    }

    if (result.message.find(
            expectedText
            ) == std::string::npos)
    {
        throw std::runtime_error(
            testName +
            " compilou, mas nao produziu a mensagem esperada: " +
            result.message
            );
    }
}

void testOperationTable()
{
    require(
        SemanticTable::resultType(
            DataType::Integer,
            DataType::Integer,
            BinaryOperator::Add
            ) == DataType::Integer,
        "INTEIRO + INTEIRO deveria resultar em INTEIRO."
        );

    require(
        SemanticTable::resultType(
            DataType::Integer,
            DataType::Integer,
            BinaryOperator::Divide
            ) == DataType::Float,
        "INTEIRO / INTEIRO deveria resultar em FLUTUANTE."
        );

    require(
        SemanticTable::resultType(
            DataType::Integer,
            DataType::Float,
            BinaryOperator::Add
            ) == DataType::Float,
        "INTEIRO + FLUTUANTE deveria resultar em FLUTUANTE."
        );

    require(
        SemanticTable::resultType(
            DataType::Float,
            DataType::Integer,
            BinaryOperator::Multiply
            ) == DataType::Float,
        "FLUTUANTE * INTEIRO deveria resultar em FLUTUANTE."
        );

    require(
        SemanticTable::resultType(
            DataType::Text,
            DataType::Character,
            BinaryOperator::Add
            ) == DataType::Text,
        "TEXTO + CARACTERE deveria resultar em TEXTO."
        );

    require(
        SemanticTable::resultType(
            DataType::Character,
            DataType::Text,
            BinaryOperator::Add
            ) == DataType::Text,
        "CARACTERE + TEXTO deveria resultar em TEXTO."
        );

    require(
        SemanticTable::resultType(
            DataType::Character,
            DataType::Character,
            BinaryOperator::Add
            ) == DataType::Text,
        "CARACTERE + CARACTERE deveria resultar em TEXTO."
        );

    require(
        SemanticTable::resultType(
            DataType::Integer,
            DataType::Float,
            BinaryOperator::Relational
            ) == DataType::Logical,
        "Comparacao numerica deveria resultar em LOGICO."
        );

    require(
        SemanticTable::resultType(
            DataType::Logical,
            DataType::Logical,
            BinaryOperator::LogicalAnd
            ) == DataType::Logical,
        "LOGICO e LOGICO deveria resultar em LOGICO."
        );
}

void testInvalidOperations()
{
    require(
        SemanticTable::resultType(
            DataType::Text,
            DataType::Text,
            BinaryOperator::Multiply
            ) == DataType::Unknown,
        "TEXTO * TEXTO deveria ser invalido."
        );

    require(
        SemanticTable::resultType(
            DataType::Integer,
            DataType::Logical,
            BinaryOperator::Add
            ) == DataType::Unknown,
        "INTEIRO + LOGICO deveria ser invalido."
        );

    require(
        SemanticTable::resultType(
            DataType::Character,
            DataType::Character,
            BinaryOperator::Subtract
            ) == DataType::Unknown,
        "CARACTERE - CARACTERE deveria ser invalido."
        );

    require(
        SemanticTable::resultType(
            DataType::Float,
            DataType::Float,
            BinaryOperator::Modulo
            ) == DataType::Unknown,
        "FLUTUANTE resto FLUTUANTE deveria ser invalido."
        );
}

void testAssignments()
{
    require(
        SemanticTable::atribType(
            DataType::Integer,
            DataType::Integer
            ) == CompatibilityResult::Ok,
        "INTEIRO <- INTEIRO deveria ser OK."
        );

    require(
        SemanticTable::atribType(
            DataType::Float,
            DataType::Integer
            ) == CompatibilityResult::Ok,
        "FLUTUANTE <- INTEIRO deveria ser OK."
        );

    require(
        SemanticTable::atribType(
            DataType::Integer,
            DataType::Float
            ) == CompatibilityResult::Warning,
        "INTEIRO <- FLUTUANTE deveria gerar AVISO."
        );

    require(
        SemanticTable::atribType(
            DataType::Text,
            DataType::Character
            ) == CompatibilityResult::Error,
        "TEXTO <- CARACTERE deveria ser ERRO."
        );

    require(
        SemanticTable::atribType(
            DataType::Logical,
            DataType::Integer
            ) == CompatibilityResult::Error,
        "LOGICO <- INTEIRO deveria ser ERRO."
        );
}

void testUnaryOperations()
{
    require(
        SemanticTable::resultType(
            DataType::Integer,
            UnaryOperator::Negate
            ) == DataType::Integer,
        "menos INTEIRO deveria resultar em INTEIRO."
        );

    require(
        SemanticTable::resultType(
            DataType::Float,
            UnaryOperator::Negate
            ) == DataType::Float,
        "menos FLUTUANTE deveria resultar em FLUTUANTE."
        );

    require(
        SemanticTable::resultType(
            DataType::Logical,
            UnaryOperator::LogicalNot
            ) == DataType::Logical,
        "nao LOGICO deveria resultar em LOGICO."
        );

    require(
        SemanticTable::resultType(
            DataType::Text,
            UnaryOperator::Negate
            ) == DataType::Unknown,
        "menos TEXTO deveria ser invalido."
        );
}

void testCompoundExpression()
{
    const DataType left =
        SemanticTable::resultType(
            DataType::Integer,
            DataType::Integer,
            BinaryOperator::Add
            );

    const DataType right =
        SemanticTable::resultType(
            DataType::Integer,
            DataType::Integer,
            BinaryOperator::Divide
            );

    const DataType result =
        SemanticTable::resultType(
            left,
            right,
            BinaryOperator::Multiply
            );

    require(
        left == DataType::Integer,
        "1 mais 2 deveria resultar em INTEIRO."
        );

    require(
        right == DataType::Float,
        "3 divido 2 deveria resultar em FLUTUANTE."
        );

    require(
        result == DataType::Float,
        "(1 mais 2) vezes (3 divido 2) deveria resultar em FLUTUANTE."
        );
}

void testCompilerIntegration()
{
    CompilerService compiler;

    const std::string validOperations =
        R"(
programa tipos
inicio
inteiro i;
flutuante f;
texto t;
caractere c;
logico l;

i recebe 10;
f recebe 2.5;
c recebe 'a';

f recebe i mais f;
t recebe "valor" mais c;
l recebe i menorQue f;
fim
)";

    expectSuccess(
        compiler,
        validOperations,
        "Operacoes validas"
        );

    const std::string invalidOperation =
        R"(
programa operacaoInvalida
inicio
texto a, b;

a recebe "primeiro";
b recebe "segundo";

a recebe a vezes b;
fim
)";

    expectFailureContaining(
        compiler,
        invalidOperation,
        "incompativel",
        "Operacao invalida"
        );

    const std::string allowedConversion =
        R"(
programa conversaoPermitida
inicio
inteiro i;
flutuante f;

i recebe 10;
f recebe i;
fim
)";

    expectSuccess(
        compiler,
        allowedConversion,
        "Conversao INTEIRO para FLUTUANTE"
        );

    const std::string warningAssignment =
        R"(
programa atribuicaoAviso
inicio
inteiro i;
flutuante f;

f recebe 10.5;
i recebe f;
fim
)";

    expectSuccessContaining(
        compiler,
        warningAssignment,
        "conversao de FLUTUANTE para INTEIRO",
        "Atribuicao com aviso"
        );

    const std::string invalidAssignment =
        R"(
programa atribuicaoInvalida
inicio
logico l;

l recebe 10;
fim
)";

    expectFailureContaining(
        compiler,
        invalidAssignment,
        "Atribuicao incompativel",
        "Atribuicao invalida"
        );

    const std::string compoundExpression =
        R"(
programa expressaoComposta
inicio
flutuante resultado;

resultado recebe (1 mais 2) vezes (3 divido 2);
fim
)";

    expectSuccess(
        compiler,
        compoundExpression,
        "Expressao composta"
        );

    const std::string validLogicalExpression =
        R"(
programa expressaoLogica
inicio
inteiro a;
flutuante b;
logico resultado;

a recebe 10;
b recebe 20.5;

resultado recebe
    (a menorQue b)
    e
    (b diferente 0.0);
fim
)";

    expectSuccess(
        compiler,
        validLogicalExpression,
        "Expressao logica"
        );

    const std::string invalidCondition =
        R"(
programa condicaoInvalida
inicio
inteiro a;

a recebe 10;

se a mais 1 entao
    a recebe 20;
fimse

fim
)";

    expectFailureContaining(
        compiler,
        invalidCondition,
        "Condicao deve resultar em LOGICO",
        "Condicao nao logica"
        );
}

}

int main()
{
    try
    {
        testOperationTable();
        testInvalidOperations();
        testAssignments();
        testUnaryOperations();
        testCompoundExpression();
        testCompilerIntegration();

        std::cout
            << "Todos os testes do sistema de tipos passaram."
            << std::endl;

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha no sistema de tipos: "
            << error.what()
            << std::endl;

        return 1;
    }
}