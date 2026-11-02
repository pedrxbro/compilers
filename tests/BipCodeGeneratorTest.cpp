
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

    requireContains(result.assembly, "    AND b\n", "AND");
    requireContains(result.assembly, "    OR b\n", "OR");
    requireContains(result.assembly, "    XOR b\n", "XOR");
    requireContains(result.assembly, "    NOT 0\n", "NOT");
    requireContains(result.assembly, "    SLL 2\n", "SLL");
    requireContains(result.assembly, "    SRL 1\n", "SRL");

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

void testVectorAndIo()
{
    const std::string source = R"(programa vetores
inicio
inteiro a, b, valores[3];

a recebe 2;
valores[0] recebe 10;
valores[a] recebe valores[0] mais 5;
b recebe valores[a] eBitBit 7;

leia(valores[1]);
mostre(b);
mostre(valores[1]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Vetores e entrada/saida");

    requireContains(
        result.assembly,
        "a: 0\n"
        "b: 0\n"
        "valores: 0\n"
        "__bip_vec_slot_2_1: 0\n"
        "__bip_vec_slot_2_2: 0\n",
        "Alocacao contigua de vetor"
        );

    requireContains(
        result.assembly,
        "    LDI 0\n"
        "    STO $indr\n"
        "    LDV valores\n"
        "    ADDI 5\n",
        "Leitura de vetor em expressao"
        );

    requireContains(
        result.assembly,
        "    STO __bip_tmp_1\n"
        "    LD __bip_tmp_0\n"
        "    STO $indr\n"
        "    LD __bip_tmp_1\n"
        "    STOV valores\n",
        "Preservacao do indice na atribuicao"
        );

    requireContains(
        result.assembly,
        "    LD a\n"
        "    STO $indr\n"
        "    LDV valores\n"
        "    ANDI 7\n"
        "    STO b\n",
        "Operacao bit a bit com vetor"
        );

    requireContains(
        result.assembly,
        "    LDI 1\n"
        "    STO $indr\n"
        "    LD $in_port\n"
        "    STOV valores\n",
        "Entrada em vetor"
        );

    requireContains(
        result.assembly,
        "    LD b\n"
        "    STO $out_port\n",
        "Saida escalar"
        );

    requireContains(
        result.assembly,
        "    LDI 1\n"
        "    STO $indr\n"
        "    LDV valores\n"
        "    STO $out_port\n",
        "Saida de vetor"
        );

    std::cout << "[OK] Vetores e entrada/saida\n";
}

void testScalarIoAndLiterals()
{
    const std::string source = R"(programa entrada
inicio
inteiro a, valores[1];

leia(a);
valores[0] recebe a;
mostre(42);
mostre(a);
mostre(valores[0]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Entrada escalar e literais");

    requireContains(
        result.assembly,
        "    LD $in_port\n"
        "    STO a\n",
        "Leitura escalar"
        );

    requireContains(
        result.assembly,
        "    LDI 42\n"
        "    STO $out_port\n",
        "Saida literal"
        );

    requireContains(
        result.assembly,
        "    LD a\n"
        "    STO $out_port\n",
        "Saida de variavel"
        );

    requireContains(
        result.assembly,
        "    LDI 0\n"
        "    STO $indr\n"
        "    LDV valores\n"
        "    STO $out_port\n",
        "Saida de vetor unitario"
        );

    std::cout << "[OK] Entrada escalar e literais\n";
}

void testExpressionIndex()
{
    const std::string source = R"(programa indices
inicio
inteiro a, b, valores[4];

a recebe 1;
b recebe 2;

valores[a mais b] recebe 25;
valores[0] recebe valores[a mais b] menos 5;
mostre(valores[0]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Indice com expressao");

    requireContains(
        result.assembly,
        "    LD a\n"
        "    ADD b\n"
        "    STO __bip_tmp_0\n",
        "Calculo do indice de atribuicao"
        );

    requireContains(
        result.assembly,
        "    LD a\n"
        "    ADD b\n"
        "    STO $indr\n"
        "    LDV valores\n"
        "    SUBI 5\n",
        "Indice calculado em leitura"
        );

    std::cout << "[OK] Indices com expressoes\n";
}

void testNestedVectorAccess()
{
    const std::string source = R"(programa aninhado
inicio
inteiro valores[3], resultado;

valores[0] recebe 1;
valores[1] recebe 9;
valores[valores[0]] recebe valores[1] mais valores[0];
resultado recebe valores[1];
mostre(resultado);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Acessos aninhados");

    requireContains(
        result.assembly,
        "    LDI 0\n"
        "    STO $indr\n"
        "    LDV valores\n"
        "    STO __bip_tmp_0\n",
        "Indice obtido de outro elemento"
        );

    requireContains(
        result.assembly,
        "    LD __bip_tmp_0\n"
        "    STO $indr\n"
        "    LD __bip_tmp_1\n"
        "    STOV valores\n",
        "Indice restaurado apos avaliar RHS"
        );

    std::cout << "[OK] Acessos aninhados\n";
}

void testVectorArithmeticAndBitwise()
{
    const std::string source = R"(programa operacoes
inicio
inteiro a, valores[3], resultado;

a recebe 1;
valores[0] recebe 5;
valores[1] recebe 6;

resultado recebe valores[a] mais valores[0];
resultado recebe valores[a] menos valores[0];
resultado recebe valores[a] ouBitBit valores[0];
resultado recebe valores[a] ouExclusivoBitBit 3;
resultado recebe naoBitBit valores[0];
resultado recebe valores[a] deslocaEsquerda 1;
mostre(resultado);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Aritmetica e bits com vetores");

    requireContains(
        result.assembly,
        "    ADD __bip_tmp_1\n",
        "Soma entre elementos"
        );

    requireContains(
        result.assembly,
        "    SUB __bip_tmp_1\n",
        "Subtracao entre elementos"
        );

    requireContains(
        result.assembly,
        "    OR __bip_tmp_1\n",
        "OU entre elementos"
        );

    requireContains(
        result.assembly,
        "    XORI 3\n",
        "XOR com elemento"
        );

    requireContains(
        result.assembly,
        "    NOT 0\n",
        "Complemento de elemento"
        );

    requireContains(
        result.assembly,
        "    SLL 1\n",
        "Deslocamento de elemento"
        );

    std::cout << "[OK] Aritmetica e bits com vetores\n";
}

void testTemporaryReuse()
{
    const std::string source = R"(programa temporarios
inicio
inteiro a, valores[3];

a recebe 0;
valores[a] recebe valores[0] mais 1;
valores[a] recebe valores[0] mais 2;
valores[a] recebe valores[0] mais 3;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Reutilizacao de temporarios");

    requireContains(
        result.assembly,
        "__bip_tmp_0: 0",
        "Primeiro temporario"
        );

    requireContains(
        result.assembly,
        "__bip_tmp_1: 0",
        "Segundo temporario"
        );

    require(
        result.assembly.find("__bip_tmp_2: 0") ==
            std::string::npos,
        "Temporarios nao foram reutilizados."
        );

    std::cout << "[OK] Reutilizacao de temporarios\n";
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

void testOutOfBoundsLiteral()
{
    const std::string source = R"(programa limite
inicio
inteiro valores[3];
valores[3] recebe 10;
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireFailure(
        result,
        "Indice literal fora dos limites"
        );

    std::cout << "[OK] Indice literal invalido recusado\n";
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

        testVectorAndIo();
        testScalarIoAndLiterals();
        testExpressionIndex();
        testNestedVectorAccess();
        testVectorArithmeticAndBitwise();
        testTemporaryReuse();

        testUnsupportedMultiplication();
        testUnsupportedDynamicShift();
        testUnsupportedLargeImmediate();
        testSemanticFailure();
        testOutOfBoundsLiteral();

        std::cout
            << "Todos os testes do gerador BIP passaram.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha nos testes do gerador BIP: "
            << error.what()
            << '\n';

        return 1;
    }
}
