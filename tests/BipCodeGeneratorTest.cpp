#include "BipCodeGenerator.h"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

// ============================================================
// FUNCOES AUXILIARES DOS TESTES
// ============================================================

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

// ============================================================
// INTERPRETADOR SEQUENCIAL EXCLUSIVO DOS TESTES
// ============================================================

std::string trim(const std::string &text)
{
    const auto first =
        text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
    {
        return {};
    }

    const auto last =
        text.find_last_not_of(" \t\r\n");

    return text.substr(first, last - first + 1);
}

int word16(int value)
{
    const unsigned int bits =
        static_cast<unsigned int>(value) & 0xFFFFu;

    if (bits >= 0x8000u)
    {
        return static_cast<int>(bits) - 65536;
    }

    return static_cast<int>(bits);
}

std::vector<int> simulateAssembly(
    const std::string &assembly,
    const std::vector<int> &inputs
    )
{
    std::unordered_map<std::string, std::size_t> addresses;

    std::vector<int> memory;
    std::vector<int> outputs;

    std::size_t inputPosition = 0;

    int acc = 0;
    int indr = 0;

    bool inData = false;
    bool inText = false;
    bool halted = false;

    auto addressOf = [&](const std::string &name)
        -> std::size_t
    {
        const auto found = addresses.find(name);

        require(
            found != addresses.end(),
            "Rotulo desconhecido no Assembly: " + name
            );

        return found->second;
    };

    auto vectorAddress = [&](const std::string &name)
        -> std::size_t
    {
        const long long address =
            static_cast<long long>(addressOf(name)) +
            indr;

        require(
            address >= 0 &&
                address < static_cast<long long>(memory.size()),
            "Endereco de vetor invalido: " + name
            );

        return static_cast<std::size_t>(address);
    };

    std::istringstream stream(assembly);
    std::string line;

    while (std::getline(stream, line))
    {
        line = trim(line);

        if (line.empty())
        {
            continue;
        }

        if (line == ".data")
        {
            inData = true;
            inText = false;
            continue;
        }

        if (line == ".text")
        {
            inData = false;
            inText = true;
            continue;
        }

        if (inData)
        {
            const auto colon = line.find(':');

            require(
                colon != std::string::npos,
                "Declaracao invalida: " + line
                );

            const std::string name =
                trim(line.substr(0, colon));

            require(
                addresses.emplace(name, memory.size()).second,
                "Rotulo de memoria duplicado: " + name
                );

            std::istringstream values(
                line.substr(colon + 1)
                );

            std::string value;
            std::size_t count = 0;

            while (std::getline(values, value, ','))
            {
                memory.push_back(
                    word16(std::stoi(trim(value)))
                    );

                ++count;
            }

            require(
                count > 0,
                "Declaracao sem valores: " + name
                );

            continue;
        }

        require(
            inText,
            "Instrucao fora da secao .text."
            );

        if (line.back() == ':')
        {
            continue;
        }

        std::istringstream instruction(line);

        std::string opcode;
        std::string operand;

        instruction >> opcode >> operand;

        require(
            !operand.empty(),
            "Instrucao sem operando: " + line
            );

        if (opcode == "HLT")
        {
            halted = true;
            break;
        }

        if (opcode == "LDI")
        {
            acc = word16(std::stoi(operand));
        }
        else if (opcode == "LD")
        {
            if (operand == "$in_port")
            {
                require(
                    inputPosition < inputs.size(),
                    "Entrada de dados insuficiente."
                    );

                acc = word16(inputs[inputPosition++]);
            }
            else if (operand == "$indr")
            {
                acc = indr;
            }
            else
            {
                acc = memory.at(addressOf(operand));
            }
        }
        else if (opcode == "STO")
        {
            if (operand == "$indr")
            {
                indr = acc;
            }
            else if (operand == "$out_port")
            {
                outputs.push_back(acc);
            }
            else
            {
                memory.at(addressOf(operand)) = acc;
            }
        }
        else if (opcode == "LDV")
        {
            acc = memory.at(vectorAddress(operand));
        }
        else if (opcode == "STOV")
        {
            memory.at(vectorAddress(operand)) = acc;
        }
        else if (opcode == "ADD")
        {
            acc = word16(
                acc + memory.at(addressOf(operand))
                );
        }
        else if (opcode == "ADDI")
        {
            acc = word16(acc + std::stoi(operand));
        }
        else if (opcode == "SUB")
        {
            acc = word16(
                acc - memory.at(addressOf(operand))
                );
        }
        else if (opcode == "SUBI")
        {
            acc = word16(acc - std::stoi(operand));
        }
        else if (opcode == "AND")
        {
            acc = word16(
                acc & memory.at(addressOf(operand))
                );
        }
        else if (opcode == "ANDI")
        {
            acc = word16(acc & std::stoi(operand));
        }
        else if (opcode == "OR")
        {
            acc = word16(
                acc | memory.at(addressOf(operand))
                );
        }
        else if (opcode == "ORI")
        {
            acc = word16(acc | std::stoi(operand));
        }
        else if (opcode == "XOR")
        {
            acc = word16(
                acc ^ memory.at(addressOf(operand))
                );
        }
        else if (opcode == "XORI")
        {
            acc = word16(acc ^ std::stoi(operand));
        }
        else if (opcode == "NOT")
        {
            acc = word16(~acc);
        }
        else if (opcode == "SLL" || opcode == "SRL")
        {
            const int amount = std::stoi(operand);

            require(
                amount >= 0 && amount <= 15,
                "Deslocamento invalido."
                );

            const unsigned int bits =
                static_cast<unsigned int>(acc) & 0xFFFFu;

            unsigned int shifted = 0;

            if (opcode == "SLL")
            {
                shifted = bits << amount;
            }
            else
            {
                shifted = bits >> amount;
            }

            acc = word16(
                static_cast<int>(shifted & 0xFFFFu)
                );
        }
        else
        {
            throw std::runtime_error(
                "Instrucao nao suportada pelo teste: " +
                opcode
                );
        }
    }

    require(
        halted,
        "Programa nao finalizou com HLT."
        );

    require(
        inputPosition == inputs.size(),
        "Nem todas as entradas foram consumidas."
        );

    return outputs;
}

std::string formatValues(const std::vector<int> &values)
{
    std::ostringstream output;

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            output << ", ";
        }

        output << values[i];
    }

    return output.str();
}

void requireOutput(
    const bip::CodegenResult &result,
    const std::vector<int> &inputs,
    const std::vector<int> &expected,
    const std::string &testName
    )
{
    requireSuccess(result, testName);

    const auto actual =
        simulateAssembly(result.assembly, inputs);

    require(
        actual == expected,
        testName +
            ": saida esperada [" +
            formatValues(expected) +
            "], obtida [" +
            formatValues(actual) +
            "]."
        );
}

// ============================================================
// TESTES ORIGINAIS
// ============================================================

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
        "temp1: 0",
        "Temporario esquerdo"
        );

    requireContains(
        result.assembly,
        "temp2: 0",
        "Temporario direito"
        );

    requireContains(
        result.assembly,
        "    LD a\n"
        "    ADDI 2\n"
        "    STO temp1\n"
        "    LD b\n"
        "    SUBI 1\n"
        "    STO temp2\n"
        "    LD temp1\n"
        "    SUB temp2\n"
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
        "valores: 0, 0, 0\n",
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
        "    STO temp2\n"
        "    LD temp1\n"
        "    STO $indr\n"
        "    LD temp2\n"
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

    requireOutput(
        result,
        {99},
        {7, 99},
        "Execucao de vetores e entrada/saida"
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

    requireOutput(
        result,
        {17},
        {42, 17, 17},
        "Execucao de entrada escalar e literais"
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
        "    STO temp1\n",
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

    requireOutput(
        result,
        {},
        {20},
        "Execucao com indice calculado"
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
        "    STO temp1\n",
        "Indice obtido de outro elemento"
        );

    requireContains(
        result.assembly,
        "    LD temp1\n"
        "    STO $indr\n"
        "    LD temp2\n"
        "    STOV valores\n",
        "Indice restaurado apos avaliar RHS"
        );

    requireOutput(
        result,
        {},
        {10},
        "Execucao de acessos aninhados"
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
        "    ADD temp2\n",
        "Soma entre elementos"
        );

    requireContains(
        result.assembly,
        "    SUB temp2\n",
        "Subtracao entre elementos"
        );

    requireContains(
        result.assembly,
        "    OR temp2\n",
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

    requireOutput(
        result,
        {},
        {12},
        "Execucao de aritmetica e bits com vetores"
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
        "temp1: 0",
        "Primeiro temporario"
        );

    requireContains(
        result.assembly,
        "temp2: 0",
        "Segundo temporario"
        );

    require(
        result.assembly.find("temp3: 0") ==
            std::string::npos,
        "Temporarios nao foram reutilizados."
        );

    std::cout << "[OK] Reutilizacao de temporarios\n";
}

void testReadableNamesAndVectorExpression()
{
    const std::string source = R"(programa testeBip
inicio
inteiro i, a, b, c, d;
inteiro v[5], x[6], z[6];

i recebe 1;
a recebe 1;
b recebe 2;
c recebe 3;
d recebe 2;

x[3] recebe 20;
z[4] recebe 7;

v[i mais 1] recebe x[a mais b] menos z[c mais d menos 1];

mostre(v[2]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(
        result,
        "Nomes legiveis e expressao entre vetores"
        );

    requireContains(
        result.assembly,
        "v: 0, 0, 0, 0, 0\n"
        "x: 0, 0, 0, 0, 0, 0\n"
        "z: 0, 0, 0, 0, 0, 0\n",
        "Reserva de memoria com nomes legiveis"
        );

    requireContains(
        result.assembly,
        "temp1: 0\n"
        "temp2: 0\n"
        "temp3: 0\n",
        "Tres temporarios com nomes legiveis"
        );

    requireContains(
        result.assembly,
        "    LD i\n"
        "    ADDI 1\n"
        "    STO temp1\n"
        "    LD a\n"
        "    ADD b\n"
        "    STO $indr\n"
        "    LDV x\n"
        "    STO temp2\n"
        "    LD c\n"
        "    ADD d\n"
        "    SUBI 1\n"
        "    STO $indr\n"
        "    LDV z\n"
        "    STO temp3\n"
        "    LD temp2\n"
        "    SUB temp3\n"
        "    STO temp2\n"
        "    LD temp1\n"
        "    STO $indr\n"
        "    LD temp2\n"
        "    STOV v\n",
        "Indices e valor da atribuicao preservados"
        );

    requireContains(
        result.assembly,
        "    LDI 2\n"
        "    STO $indr\n"
        "    LDV v\n"
        "    STO $out_port\n",
        "Saida do resultado do vetor"
        );

    requireContains(
        result.assembly,
        "    HLT 0\n",
        "Finalizacao do programa"
        );

    requireOutput(
        result,
        {},
        {13},
        "Execucao com expressoes entre vetores"
        );

    std::cout
        << "[OK] Nomes legiveis e expressao entre vetores\n";
}

void testNameCollisions()
{
    const std::string source = R"(programa conflitos
inicio
inteiro v[3], v_1, temp1, resultado;

v_1 recebe 10;
temp1 recebe 7;

v[1] recebe v_1 menos temp1;

resultado recebe v[1];
mostre(resultado);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(result, "Conflitos de nomes auxiliares");

    requireContains(
        result.assembly,
        "v: 0, 0, 0\n"
        "v_1: 0\n",
        "Vetor e variavel do usuario preservados"
        );

    require(
        result.assembly.find("__bip_vec_slot_") ==
            std::string::npos,
        "Rotulo artificial de vetor nao deveria existir."
        );

    requireContains(
        result.assembly,
        "temp1: 0\n"
        "resultado: 0\n"
        "temp2: 0\n"
        "temp3: 0\n",
        "Colisao de nome de temporario"
        );

    requireContains(
        result.assembly,
        "    LD v_1\n"
        "    SUB temp1\n",
        "Variaveis do usuario preservadas"
        );

    requireContains(
        result.assembly,
        "    STO temp3\n"
        "    LD temp2\n"
        "    STO $indr\n"
        "    LD temp3\n"
        "    STOV v\n",
        "Atribuicao ao vetor sem conflito"
        );

    requireOutput(
        result,
        {},
        {3},
        "Execucao sem colisao de nomes"
        );

    std::cout << "[OK] Conflitos de nomes auxiliares\n";
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

// ============================================================
// NOVOS TESTES DE REGRESSAO DA M2.2
// ============================================================

void testEvaluationRegression()
{
    const std::string source = R"(programa testeVetores
inicio
inteiro i, a, v[5], x[5];

i recebe 1;
v[0] recebe 10;
v[i] recebe 20;
v[i mais 1] recebe 30;
x[3] recebe 7;

a recebe v[i mais 1];
mostre(a);

v[i mais 1] recebe v[i] menos x[i mais 2];
mostre(v[i mais 1]);

leia(v[i mais 1]);
mostre(v[i mais 1]);
mostre(v[i]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(
        result,
        "Regressao da avaliacao M2.2"
        );

    requireContains(
        result.assembly,
        "v: 0, 0, 0, 0, 0\n"
        "x: 0, 0, 0, 0, 0\n",
        "Declaracao contigua"
        );

    requireOutput(
        result,
        {99},
        {30, 13, 99, 20},
        "Regressao da avaliacao M2.2"
        );

    std::cout << "[OK] Regressao da avaliacao M2.2\n";
}

void testNestedVectorInputOutput()
{
    const std::string source = R"(programa acessosAninhados
inicio
inteiro v[5], x[5];

v[0] recebe 1;
v[1] recebe 8;
x[1] recebe 2;

v[x[v[0]]] recebe v[v[0]] mais 3;
mostre(v[2]);

leia(v[x[v[0]]]);

mostre(v[x[v[0]]]);
mostre(v[1]);
mostre(x[1]);
mostre(v[0]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireOutput(
        result,
        {77},
        {11, 77, 8, 2, 1},
        "Entrada e saida com acessos aninhados"
        );

    std::cout
        << "[OK] Entrada e saida com acessos aninhados\n";
}

void testCompositeVectorIndices()
{
    const std::string source = R"(programa indicesCompostos
inicio
inteiro i, j, a, v[5];

i recebe 3;
j recebe 1;

v[(i mais 1) menos (j mais 1)] recebe 42;
v[i menos j menos 1] recebe 17;
v[i menos (j menos 1)] recebe 31;

a recebe 50 menos v[(i mais 1) menos (j mais 1)];

mostre(v[2]);
mostre(v[1]);
mostre(v[3]);
mostre(a);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireOutput(
        result,
        {},
        {42, 17, 31, 8},
        "Indices compostos e parenteses"
        );

    std::cout << "[OK] Indices compostos e parenteses\n";
}

void testNestedIndicesInsideBinaryExpression()
{
    const std::string source = R"(programa operandosAninhados
inicio
inteiro v[6], x[6], a;

v[0] recebe 1;
v[1] recebe 20;
v[2] recebe 4;

x[0] recebe 2;
x[2] recebe 7;

a recebe v[v[0]] menos (x[x[0]] menos v[2]);
mostre(a);

v[x[0] mais 1] recebe v[v[0]] menos x[x[0]];

mostre(v[3]);
mostre(v[1]);
mostre(x[2]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireOutput(
        result,
        {},
        {17, 13, 20, 7},
        "Operandos aninhados e temporarios simultaneos"
        );

    std::cout
        << "[OK] Operandos aninhados e temporarios simultaneos\n";
}

void testBitwiseVectorIndices()
{
    const std::string source = R"(programa indicesBitwise
inicio
inteiro i, j, v[8];

i recebe 6;
j recebe 3;

v[i eBitBit j] recebe 12;
v[i ouExclusivoBitBit j] recebe 25;
v[(j deslocaEsquerda 1) ouBitBit 1] recebe 37;
v[(i deslocaDireita 1) menos 2] recebe 44;
v[(naoBitBit 0) eBitBit 3] recebe 55;

mostre(v[i eBitBit j]);
mostre(v[i ouExclusivoBitBit j]);
mostre(v[(j deslocaEsquerda 1) ouBitBit 1]);
mostre(v[(i deslocaDireita 1) menos 2]);
mostre(v[(naoBitBit 0) eBitBit 3]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireOutput(
        result,
        {},
        {12, 25, 37, 44, 55},
        "Operacoes bit a bit nos indices"
        );

    std::cout << "[OK] Operacoes bit a bit nos indices\n";
}

void testTemporaryCollisionRuntime()
{
    const std::string source = R"(programa colisoes
inicio
inteiro i, temp1, temp2, v_1, v[3], x[3];

i recebe 1;
temp1 recebe 7;
temp2 recebe 9;
v_1 recebe 100;

v[0] recebe 6;
x[1] recebe 20;

v[i] recebe x[i] menos v[0];

mostre(v[1]);
mostre(v_1);
mostre(temp1);
mostre(temp2);
mostre(v[0]);
mostre(x[1]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireSuccess(
        result,
        "Colisoes de temporarios em execucao"
        );

    requireContains(
        result.assembly,
        "temp3: 0\n",
        "Temporarios devem ignorar nomes do usuario"
        );

    requireOutput(
        result,
        {},
        {14, 100, 7, 9, 6, 20},
        "Preservacao das variaveis do usuario"
        );

    std::cout
        << "[OK] Colisoes de temporarios em execucao\n";
}

void testVectorBoundaryIndices()
{
    const std::string source = R"(programa fronteiras
inicio
inteiro esquerda, v[3], direita;

esquerda recebe 70;
direita recebe 80;

v[0] recebe 10;
v[2] recebe 20;

mostre(v[0]);
mostre(v[2]);
mostre(esquerda);
mostre(direita);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireOutput(
        result,
        {},
        {10, 20, 70, 80},
        "Primeiro e ultimo indices do vetor"
        );

    std::cout << "[OK] Limites validos do vetor\n";
}

void testLiteralFormatsInIndices()
{
    const std::string source = R"(programa basesNumericas
inicio
inteiro v[5];

v[0b10] recebe 12;
v[0x3] recebe 13;

mostre(v[2]);
mostre(v[3]);
fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    requireOutput(
        result,
        {},
        {12, 13},
        "Literais binarios e hexadecimais nos indices"
        );

    std::cout
        << "[OK] Bases numericas nos indices\n";
}

void testInvalidLiteralIndices()
{
    const std::vector<std::string> commands = {
        "v[3] recebe 10;",
        "v[menos 1] recebe 10;",
        "v[(menos 1)] recebe 10;",
        "leia(v[3]);",
        "leia(v[menos 1]);",
        "mostre(v[3]);",
        "mostre(v[menos 1]);",
        "a recebe v[3];",
        "a recebe v[menos 1];"
    };

    const bip::BipCodeGenerator generator;

    for (const auto &command : commands)
    {
        const std::string source =
            "programa indiceInvalido\n"
            "inicio\n"
            "inteiro a, v[3];\n"
            "v[0] recebe 1;\n" +
            command +
            "\nfim";

        const auto result = generator.generate(source);

        requireFailure(
            result,
            "Indice invalido: " + command
            );

        requireContains(
            result.message,
            "Indice literal fora dos limites",
            "Diagnostico de indice invalido"
            );
    }

    std::cout << "[OK] Indices literais invalidos\n";
}

void testUnsupportedOperationsInIndices()
{
    const std::vector<std::string> commands = {
        "v[i vezes 2] recebe 10;",
        "v[i divido 2] recebe 10;",
        "v[i resto 2] recebe 10;",
        "v[i deslocaEsquerda j] recebe 10;"
    };

    const bip::BipCodeGenerator generator;

    for (const auto &command : commands)
    {
        const std::string source =
            "programa indiceSemSuporte\n"
            "inicio\n"
            "inteiro i, j, v[5];\n"
            "i recebe 1;\n"
            "j recebe 1;\n" +
            command +
            "\nfim";

        const auto result = generator.generate(source);

        requireFailure(
            result,
            "Operacao de indice sem suporte: " + command
            );
    }

    std::cout
        << "[OK] Operacoes fora do escopo recusadas\n";
}

}

int main()
{
    try
    {
        // Testes originais.
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

        testReadableNamesAndVectorExpression();
        testNameCollisions();

        testUnsupportedMultiplication();
        testUnsupportedDynamicShift();
        testUnsupportedLargeImmediate();
        testSemanticFailure();
        testOutOfBoundsLiteral();

        // Novas regressoes da M2.2.
        testEvaluationRegression();
        testNestedVectorInputOutput();
        testCompositeVectorIndices();
        testNestedIndicesInsideBinaryExpression();
        testBitwiseVectorIndices();
        testTemporaryCollisionRuntime();
        testVectorBoundaryIndices();
        testLiteralFormatsInIndices();
        testInvalidLiteralIndices();
        testUnsupportedOperationsInIndices();

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