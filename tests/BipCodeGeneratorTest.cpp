
#include "BipCodeGenerator.h"

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
        throw std::runtime_error(message);
    }
}

void requireContains(
    const std::string &text,
    const std::string &expected,
    const std::string &testName
    )
{
    require(
        text.find(expected) != std::string::npos,
        testName + ": trecho ausente: " + expected
        );
}

void requireSuccess(
    const bip::CodegenResult &result,
    const std::string &testName
    )
{
    require(
        result.success,
        testName + ": geracao falhou: " + result.message
        );

    require(
        !result.assembly.empty(),
        testName + ": Assembly vazio em resultado de sucesso."
        );
}

void requireFailure(
    const bip::CodegenResult &result,
    const std::string &testName
    )
{
    require(
        !result.success,
        testName + ": falha esperada nao ocorreu."
        );

    require(
        result.assembly.empty(),
        testName + ": Assembly parcial foi retornado."
        );

    require(
        !result.message.empty(),
        testName + ": diagnostico de falha ausente."
        );
}

void testBasicProgram()
{
    const std::string source = R"(programa calculo
inicio
inteiro a, b, resultado;

a recebe 10;
b recebe 5;
resultado recebe a mais b;
resultado recebe resultado menos 2;
fim)";

    const std::string expected = R"(.data
a: 0
b: 0
resultado: 0
.text
_PRINCIPAL:
    LDI 10
    STO a
    LDI 5
    STO b
    LD a
    ADD b
    STO resultado
    LD resultado
    SUBI 2
    STO resultado
    HLT 0
)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Programa basico");

    require(
        result.assembly == expected,
        "Programa basico: Assembly diferente do esperado."
        );

    std::cout << "[OK] Programa basico\n";
}

void testCompositeExpressions()
{
    const std::string source = R"(programa expressoes
inicio
inteiro a, b, resultado;

a recebe 10;
b recebe 4;

resultado recebe (a mais 2) menos (b menos 1);
resultado recebe a mais (b menos 3);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Expressoes compostas");

    requireContains(
        result.assembly,
        "__bip_tmp_0: 0",
        "Temporario esquerdo"
        );

    requireContains(
        result.assembly,
        "__bip_tmp_1: 0",
        "Temporario direito"
        );

    requireContains(
        result.assembly,
        "    LD a\n"
        "    ADDI 2\n"
        "    STO __bip_tmp_0\n"
        "    LD b\n"
        "    SUBI 1\n"
        "    STO __bip_tmp_1\n"
        "    LD __bip_tmp_0\n"
        "    SUB __bip_tmp_1\n"
        "    STO resultado\n",
        "Ordem dos operandos"
        );

    std::cout << "[OK] Expressoes compostas\n";
}

void testBitwiseOperators()
{
    const std::string source = R"(programa bitwise
inicio
inteiro a, b, resultado;

a recebe 6;
b recebe 3;

resultado recebe a eBitBit b;
resultado recebe a ouBitBit b;
resultado recebe a ouExclusivoBitBit b;
resultado recebe naoBitBit a;
resultado recebe a deslocaEsquerda 2;
resultado recebe a deslocaDireita 1;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Operacoes bit a bit");

    requireContains(
        result.assembly,
        "    AND b\n",
        "AND"
        );

    requireContains(
        result.assembly,
        "    OR b\n",
        "OR"
        );

    requireContains(
        result.assembly,
        "    XOR b\n",
        "XOR"
        );

    requireContains(
        result.assembly,
        "    NOT 0\n",
        "NOT"
        );

    requireContains(
        result.assembly,
        "    SLL 2\n",
        "SLL"
        );

    requireContains(
        result.assembly,
        "    SRL 1\n",
        "SRL"
        );

    std::cout << "[OK] Operacoes bit a bit\n";
}

void testPrecedence()
{
    const std::string source = R"(programa precedencia
inicio
inteiro a, b, resultado;

a recebe 6;
b recebe 3;

resultado recebe a mais b deslocaEsquerda 1;
resultado recebe a ouBitBit b eBitBit 1;
resultado recebe (a ouBitBit b) eBitBit 1;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Precedencia");

    requireContains(
        result.assembly,
        "    LD a\n"
        "    ADD b\n"
        "    SLL 1\n",
        "Adicao antes do deslocamento"
        );

    requireContains(
        result.assembly,
        "    LD b\n"
        "    ANDI 1\n",
        "AND com precedencia sobre OR"
        );

    requireContains(
        result.assembly,
        "    LD a\n"
        "    OR b\n"
        "    ANDI 1\n",
        "Parenteses"
        );

    std::cout << "[OK] Precedencia\n";
}

void testUnsupportedMultiplication()
{
    const std::string source = R"(programa multiplicacao
inicio
inteiro a, b, resultado;

a recebe 2;
b recebe 3;
resultado recebe a vezes b;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireFailure(result, "Multiplicacao sem suporte");

    std::cout << "[OK] Multiplicacao recusada\n";
}

void testUnsupportedVector()
{
    const std::string source = R"(programa vetor
inicio
inteiro numeros[3];
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireFailure(result, "Vetor sem suporte");

    std::cout << "[OK] Vetor recusado\n";
}

void testUnsupportedDynamicShift()
{
    const std::string source = R"(programa deslocamento
inicio
inteiro a, quantidade, resultado;

a recebe 8;
quantidade recebe 2;

resultado recebe a deslocaEsquerda quantidade;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireFailure(
        result,
        "Deslocamento dinamico sem suporte"
        );

    std::cout << "[OK] Deslocamento dinamico recusado\n";
}

void testUnsupportedLargeImmediate()
{
    const std::string source = R"(programa imediato
inicio
inteiro a;
a recebe 2048;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireFailure(
        result,
        "Imediato fora da faixa suportada"
        );

    std::cout << "[OK] Imediato recusado\n";
}

void testSemanticFailure()
{
    const std::string source = R"(programa invalido
inicio
inteiro a;
a recebe b mais 1;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireFailure(result, "Erro semantico");

    std::cout << "[OK] Erro semantico bloqueia geracao\n";
}

void testUnsupportedOutput()
{
    const std::string source = R"(programa saida
inicio
inteiro a;
a recebe 10;
mostre(a);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireFailure(result, "Saida sem suporte");

    std::cout << "[OK] Saida recusada\n";
}

}

int main()
{
    try
    {
        testBasicProgram();
        testCompositeExpressions();
        testBitwiseOperators();
        testPrecedence();
        testUnsupportedMultiplication();
        testUnsupportedVector();
        testUnsupportedDynamicShift();
        testUnsupportedLargeImmediate();
        testSemanticFailure();
        testUnsupportedOutput();

        std::cout
            << "Todos os testes do nucleo BIP passaram.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha nos testes do nucleo BIP: "
            << error.what()
            << '\n';

        return 1;
    }
}
