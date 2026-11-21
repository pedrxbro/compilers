#include "BipCodeGenerator.h"
#include "CompilerService.h"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

struct Instruction
{
    std::string opcode;
    std::string operand;
};

class AssemblyMachine
{
public:
    explicit AssemblyMachine(const std::string &assembly)
    {
        parse(assembly);
    }

    std::vector<int> execute(
        const std::vector<int> &inputs = {}
        )
    {
        std::vector<int> outputs;

        std::size_t inputPosition = 0;
        std::size_t pc = codeLabels_.at("_PRINCIPAL");

        int acc = 0;
        int indr = 0;

        bool halted = false;

        for (std::size_t steps = 0;
             steps < 100000;
             ++steps)
        {
            require(
                pc < instructions_.size(),
                "PC fora da memoria de instrucoes."
                );

            const Instruction &instruction =
                instructions_.at(pc);

            const std::string &opcode =
                instruction.opcode;

            const std::string &operand =
                instruction.operand;

            auto addressOf =
                [&](const std::string &name)
                -> std::size_t
            {
                const auto found =
                    dataLabels_.find(name);

                require(
                    found != dataLabels_.end(),
                    "Variavel nao encontrada: " + name
                    );

                return found->second;
            };

            auto vectorAddress =
                [&](const std::string &name)
                -> std::size_t
            {
                const long long address =
                    static_cast<long long>(
                        addressOf(name)
                        ) +
                    static_cast<long long>(indr);

                require(
                    address >= 0 &&
                        address <
                            static_cast<long long>(
                                memory_.size()
                                ),
                    "Endereco de vetor invalido: " + name
                    );

                return static_cast<std::size_t>(address);
            };

            bool shouldJump = false;

            if (opcode == "HLT")
            {
                halted = true;
                break;
            }
            else if (opcode == "LDI")
            {
                acc = word16(std::stoi(operand));
            }
            else if (opcode == "LD")
            {
                if (operand == "$in_port")
                {
                    require(
                        inputPosition < inputs.size(),
                        "Entrada insuficiente."
                        );

                    acc = word16(inputs[inputPosition++]);
                }
                else if (operand == "$indr")
                {
                    acc = indr;
                }
                else
                {
                    acc = memory_.at(addressOf(operand));
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
                    memory_.at(addressOf(operand)) = acc;
                }
            }
            else if (opcode == "LDV")
            {
                acc = memory_.at(vectorAddress(operand));
            }
            else if (opcode == "STOV")
            {
                memory_.at(vectorAddress(operand)) = acc;
            }
            else if (opcode == "ADD")
            {
                acc = word16(
                    acc + memory_.at(addressOf(operand))
                    );
            }
            else if (opcode == "ADDI")
            {
                acc = word16(
                    acc + std::stoi(operand)
                    );
            }
            else if (opcode == "SUB")
            {
                acc = word16(
                    acc - memory_.at(addressOf(operand))
                    );
            }
            else if (opcode == "SUBI")
            {
                acc = word16(
                    acc - std::stoi(operand)
                    );
            }
            else if (opcode == "AND")
            {
                acc = word16(
                    acc & memory_.at(addressOf(operand))
                    );
            }
            else if (opcode == "ANDI")
            {
                acc = word16(
                    acc & std::stoi(operand)
                    );
            }
            else if (opcode == "OR")
            {
                acc = word16(
                    acc | memory_.at(addressOf(operand))
                    );
            }
            else if (opcode == "ORI")
            {
                acc = word16(
                    acc | std::stoi(operand)
                    );
            }
            else if (opcode == "XOR")
            {
                acc = word16(
                    acc ^ memory_.at(addressOf(operand))
                    );
            }
            else if (opcode == "XORI")
            {
                acc = word16(
                    acc ^ std::stoi(operand)
                    );
            }
            else if (opcode == "NOT")
            {
                acc = word16(~acc);
            }
            else if (opcode == "SLL" ||
                     opcode == "SRL")
            {
                const int amount = std::stoi(operand);

                require(
                    amount >= 0 && amount <= 15,
                    "Deslocamento invalido."
                    );

                unsigned int bits =
                    static_cast<unsigned int>(acc) &
                    0xFFFFu;

                if (opcode == "SLL")
                {
                    bits <<= amount;
                }
                else
                {
                    bits >>= amount;
                }

                acc = word16(
                    static_cast<int>(bits & 0xFFFFu)
                    );
            }
            else if (opcode == "JMP")
            {
                shouldJump = true;
            }
            else if (opcode == "BEQ")
            {
                shouldJump = acc == 0;
            }
            else if (opcode == "BNE")
            {
                shouldJump = acc != 0;
            }
            else if (opcode == "BGT")
            {
                shouldJump = acc > 0;
            }
            else if (opcode == "BGE")
            {
                shouldJump = acc >= 0;
            }
            else if (opcode == "BLT")
            {
                shouldJump = acc < 0;
            }
            else if (opcode == "BLE")
            {
                shouldJump = acc <= 0;
            }
            else
            {
                throw std::runtime_error(
                    "Instrucao desconhecida: " + opcode
                    );
            }

            if (shouldJump)
            {
                const auto found =
                    codeLabels_.find(operand);

                require(
                    found != codeLabels_.end(),
                    "Destino de salto inexistente: " +
                        operand
                    );

                pc = found->second;
            }
            else
            {
                ++pc;
            }
        }

        require(
            halted,
            "Programa nao terminou com HLT."
            );

        require(
            inputPosition == inputs.size(),
            "Nem todas as entradas foram consumidas."
            );

        return outputs;
    }

private:
    void parse(const std::string &assembly)
    {
        std::istringstream input(assembly);

        std::string line;

        bool inData = false;
        bool inText = false;

        std::unordered_set<std::string> allLabels;

        while (std::getline(input, line))
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
                    allLabels.insert(name).second,
                    "Colisao de rotulo: " + name
                    );

                dataLabels_[name] = memory_.size();

                std::istringstream values(
                    line.substr(colon + 1)
                    );

                std::string item;
                std::size_t count = 0;

                while (std::getline(values, item, ','))
                {
                    memory_.push_back(
                        word16(
                            std::stoi(trim(item))
                            )
                        );

                    ++count;
                }

                require(
                    count > 0,
                    "Declaracao sem elementos."
                    );

                continue;
            }

            require(
                inText,
                "Instrucao fora da secao .text."
                );

            if (line.back() == ':')
            {
                const std::string name =
                    line.substr(0, line.size() - 1);

                require(
                    allLabels.insert(name).second,
                    "Colisao de rotulo: " + name
                    );

                codeLabels_[name] =
                    instructions_.size();

                continue;
            }

            std::istringstream parts(line);

            Instruction instruction;
            parts >> instruction.opcode;
            parts >> instruction.operand;

            require(
                !instruction.operand.empty(),
                "Instrucao sem operando: " + line
                );

            instructions_.push_back(
                std::move(instruction)
                );
        }

        require(
            codeLabels_.count("_PRINCIPAL") == 1,
            "Ponto de entrada ausente."
            );
    }

    std::unordered_map<std::string, std::size_t>
        dataLabels_;

    std::unordered_map<std::string, std::size_t>
        codeLabels_;

    std::vector<int> memory_;
    std::vector<Instruction> instructions_;
};

void requireOutput(
    const std::string &source,
    const std::vector<int> &inputs,
    const std::vector<int> &expected,
    const std::string &testName
    )
{
    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    require(
        result.success,
        testName + ": " + result.message
        );

    require(
        !result.assembly.empty(),
        testName + ": Assembly vazio."
        );

    AssemblyMachine machine(result.assembly);

    const auto actual = machine.execute(inputs);

    require(
        actual == expected,
        testName + ": saida incorreta."
        );
}

bool expectedComparison(
    const std::string &operation,
    int left,
    int right
    )
{
    if (operation == "igual")
        return left == right;

    if (operation == "diferente")
        return left != right;

    if (operation == "maiorQue")
        return left > right;

    if (operation == "menorQue")
        return left < right;

    if (operation == "maiorIgualQue")
        return left >= right;

    if (operation == "menorIgualQue")
        return left <= right;

    throw std::runtime_error(
        "Operador de teste desconhecido."
        );
}

void testInitialProgram()
{
    const std::string source = R"(programa testeCondicional
inicio
inteiro a, b;

a recebe 10;
b recebe 5;

se a maiorQue b entao
    mostre(1);
fimse

se a menorQue b entao
    mostre(2);
senao
    mostre(3);
fimse

fim)";

    requireOutput(
        source,
        {},
        {1, 3},
        "Teste inicial M3.1"
        );
}

void testAllRelationalOperators()
{
    const std::vector<std::string> operations = {
        "igual",
        "diferente",
        "maiorQue",
        "menorQue",
        "maiorIgualQue",
        "menorIgualQue"
    };

    const std::vector<std::pair<int, int>> inputs = {
        {10, 5},
        {5, 10},
        {5, 5},
        {-5, -2},
        {-2, -5},
        {32767, -32768},
        {-32768, 32767}
    };

    for (const auto &operation : operations)
    {
        const std::string source =
            "programa relacional\n"
            "inicio\n"
            "inteiro a, b;\n"
            "leia(a);\n"
            "leia(b);\n"
            "se a " + operation + " b entao\n"
                          "    mostre(1);\n"
                          "senao\n"
                          "    mostre(0);\n"
                          "fimse\n"
                          "fim";

        for (const auto &values : inputs)
        {
            const int expected =
                expectedComparison(
                    operation,
                    values.first,
                    values.second
                    )
                    ? 1
                    : 0;

            requireOutput(
                source,
                {values.first, values.second},
                {expected},
                "Comparacao " + operation
                );
        }
    }
}

void testBooleanConditions()
{
    const std::string source = R"(programa logicos
inicio
se verdadeiro entao
    mostre(1);
fimse

se falso entao
    mostre(9);
senao
    mostre(2);
fimse

se (verdadeiro ou falso) e nao falso entao
    mostre(3);
fimse

se nao (verdadeiro e falso) entao
    mostre(4);
fimse

se falso ou verdadeiro e falso entao
    mostre(9);
senao
    mostre(5);
fimse
fim)";

    requireOutput(
        source,
        {},
        {1, 2, 3, 4, 5},
        "Condicoes logicas compostas"
        );
}

void testVectorsAndExpressions()
{
    const std::string source = R"(programa vetoresCondicionais
inicio
inteiro i, a, b, v[4];

i recebe 1;
a recebe 3;
b recebe 1;

v[1] recebe 8;
v[2] recebe 2;

se (v[i] mais b) maiorQue (v[i mais 1] mais a) entao
    mostre(4);
senao
    mostre(9);
fimse

se (v[2] mais 1) igual (a) entao
    mostre(5);
fimse
fim)";

    requireOutput(
        source,
        {},
        {4, 5},
        "Comparacoes com vetores e expressoes"
        );
}

void testNestedConditionsAndScopes()
{
    const std::string source = R"(programa escoposCondicionais
inicio
inteiro a;

a recebe 1;

se verdadeiro entao
    inteiro a;
    a recebe 2;
    mostre(a);

    se a maiorQue 1 entao
        inteiro a;
        a recebe 3;
        mostre(a);
    fimse

    mostre(a);
senao
    inteiro a;
    a recebe 4;
    mostre(a);
fimse

mostre(a);
fim)";

    requireOutput(
        source,
        {},
        {2, 3, 2, 1},
        "Escopos internos e aninhamentos"
        );
}

void testNameCollisions()
{
    const std::string source = R"(programa conflitosM31
inicio
inteiro __bip_label_1, temp1;

__bip_label_1 recebe 1;
temp1 recebe 3;

se __bip_label_1 igual 1 entao
    inteiro temp1;
    temp1 recebe 7;
    mostre(temp1);
senao
    mostre(0);
fimse

mostre(temp1);
fim)";

    requireOutput(
        source,
        {},
        {7, 3},
        "Rotulos e identificadores exclusivos"
        );
}

void testBitwiseAndLogicalSeparation()
{
    const std::string source = R"(programa logicaBits
inicio
inteiro a;

a recebe 6 eBitBit 3;

se (a igual 2) e verdadeiro entao
    mostre(8);
fimse

se (6 ouBitBit 1) igual 7 entao
    mostre(9);
fimse
fim)";

    requireOutput(
        source,
        {},
        {8, 9},
        "Operacoes logicas e bit a bit distintas"
        );
}

void testOutOfScopeRejected()
{
    const std::string source = R"(programa escopoInvalido
inicio
se verdadeiro entao
    inteiro interno;
    interno recebe 1;
fimse

mostre(interno);
fim)";

    const CompilerService compiler;
    const AnalysisResult result =
        compiler.compile(source);

    require(
        !result.success,
        "Uso fora do escopo deveria falhar."
        );

    require(
        result.assembly.empty(),
        "Erro semantico gerou Assembly."
        );

    require(
        !result.errors.empty(),
        "Diagnostico de escopo ausente."
        );

    require(
        result.errors.front().phase ==
            AnalysisPhase::Semantic,
        "Escopo invalido deveria ser erro semantico."
        );
}

void testDeterministicGeneration()
{
    const std::string source = R"(programa determinismo
inicio
inteiro a;
a recebe 1;

se a igual 1 entao
    inteiro b;
    b recebe 2;
    mostre(b);
senao
    inteiro b;
    b recebe 3;
    mostre(b);
fimse
fim)";

    const bip::BipCodeGenerator generator;

    const auto first = generator.generate(source);
    const auto second = generator.generate(source);

    require(
        first.success && second.success,
        "Geracao deterministica falhou."
        );

    require(
        first.assembly == second.assembly,
        "Assembly alterado entre compilacoes iguais."
        );

    AssemblyMachine machine(first.assembly);

    require(
        machine.execute() == std::vector<int>{2},
        "Execucao deterministica incorreta."
        );
}

void testInitialLoops()
{
    const std::string source = R"(programa testeLacos
inicio
inteiro i;

i recebe 1;

enquanto i menorIgualQue 3 faca
    mostre(i);
    i recebe i mais 1;
fimenquanto

i recebe 1;

faca
    mostre(i);
    i recebe i mais 1;
ate i maiorQue 3;

fim)";

    requireOutput(
        source,
        {},
        {1, 2, 3, 1, 2, 3},
        "Teste inicial de while e faca ate"
        );
}

void testInitiallyFalseAndSingleIteration()
{
    const std::string source = R"(programa limitesLacos
inicio
inteiro i;

i recebe 5;

enquanto i menorQue 1 faca
    mostre(99);
fimenquanto

enquanto falso faca
    mostre(98);
fimenquanto

faca
    mostre(i);
    i recebe i mais 1;
ate i maiorQue 0;

fim)";

    requireOutput(
        source,
        {},
        {5},
        "While com zero iteracoes e faca com uma"
        );
}

void testVectorsAndInputInLoops()
{
    const std::string source = R"(programa vetoresLacos
inicio
inteiro i, v[4], x[4], total;

i recebe 0;

enquanto i menorQue 3 faca
    leia(v[i]);
    x[i] recebe v[i] mais i;
    i recebe i mais 1;
fimenquanto

i recebe 0;

faca
    total recebe x[i] mais v[i];
    mostre(total);
    i recebe i mais 1;
ate i maiorIgualQue 3;

fim)";

    requireOutput(
        source,
        {5, 7, 9},
        {10, 15, 20},
        "Vetores, leitura e temporarios nos lacos"
        );
}

void testComposedConditionsAndNestedLoops()
{
    const std::string source = R"(programa aninhamentoLacos
inicio
inteiro i, j, resultado;

i recebe 0;

enquanto (i menorQue 2) e (verdadeiro ou falso) faca
    j recebe 0;

    faca
        resultado recebe i mais j;

        se (resultado igual 1) ou falso entao
            mostre(7);
        senao
            mostre(resultado);
        fimse

        j recebe j mais 1;
    ate j maiorIgualQue 2;

    i recebe i mais 1;
fimenquanto

fim)";

    requireOutput(
        source,
        {},
        {0, 7, 7, 2},
        "Condicoes compostas e lacos aninhados"
        );
}

void testLoopScopesAndLabelCollisions()
{
    const std::string source = R"(programa escoposLacos
inicio
inteiro i, temp1, __bip_label_1;

i recebe 0;
temp1 recebe 50;
__bip_label_1 recebe 8;

enquanto i menorQue 2 faca
    inteiro temp1;
    temp1 recebe i mais 10;
    mostre(temp1);
    i recebe i mais 1;
fimenquanto

mostre(temp1);

faca
    inteiro limite;
    limite recebe i mais 2;
    mostre(limite);
    i recebe i mais 1;
ate limite maiorIgualQue 5;

mostre(__bip_label_1);
fim)";

    const bip::BipCodeGenerator generator;

    const auto first = generator.generate(source);
    const auto second = generator.generate(source);

    require(
        first.success && second.success,
        "Geracao dos lacos com escopos falhou."
        );

    require(
        first.assembly == second.assembly,
        "Rotulos dos lacos nao sao deterministicos."
        );

    AssemblyMachine machine(first.assembly);

    require(
        machine.execute() ==
            std::vector<int>{10, 11, 50, 4, 5, 8},
        "Escopos, condicao ate ou rotulos incorretos."
        );
}

// ============================================================
// LACO PARA - M3.1
// ============================================================

void testInitialForWithVectors()
{
    const std::string source = R"(programa testePara
inicio
inteiro i, v[5];

para i recebe 0 ate i menorQue 5 passo i recebe i mais 1 faca
    v[i] recebe i mais 10;
fimpara

para i recebe 0 ate i menorQue 5 passo i recebe i mais 1 faca
    mostre(v[i]);
fimpara

fim)";

    requireOutput(
        source,
        {},
        {10, 11, 12, 13, 14},
        "Para com escrita e leitura de vetores"
        );
}

void testForIncrementAndDecrement()
{
    const std::string source = R"(programa passosPara
inicio
inteiro i;

para i recebe 1 ate i menorIgualQue 7 passo i recebe i mais 2 faca
    mostre(i);
fimpara

para i recebe 6 ate i maiorIgualQue 0 passo i recebe i menos 3 faca
    mostre(i);
fimpara

fim)";

    requireOutput(
        source,
        {},
        {1, 3, 5, 7, 6, 3, 0},
        "Para com incrementos e decrementos diferentes de um"
        );
}

void testForZeroIterations()
{
    const std::string source = R"(programa paraVazio
inicio
inteiro i;

para i recebe 10 ate i menorQue 4 passo i recebe i mais 2 faca
    mostre(99);
fimpara

mostre(i);
fim)";

    requireOutput(
        source,
        {},
        {10},
        "Para com zero iteracoes"
        );
}

void testForCompoundConditions()
{
    const std::string source = R"(programa paraComposto
inicio
inteiro i, limite;

limite recebe 6;

para i recebe 0 ate
    (i menorQue limite) e (i diferente 4)
    passo i recebe i mais 1 faca
    mostre(i);
fimpara

fim)";

    requireOutput(
        source,
        {},
        {0, 1, 2, 3},
        "Para com condicoes logicas compostas"
        );
}

void testForIndexExpressions()
{
    const std::string source = R"(programa paraIndices
inicio
inteiro i, v[6];

para i recebe 0 ate i menorQue 3 passo i recebe i mais 1 faca
    v[i mais 1] recebe i mais 20;
fimpara

para i recebe 1 ate i menorIgualQue 3 passo i recebe i mais 1 faca
    mostre(v[i]);
fimpara

fim)";

    requireOutput(
        source,
        {},
        {20, 21, 22},
        "Para com expressoes nos indices de vetores"
        );
}

void testForExpressionAsStep()
{
    const std::string source = R"(programa paraPassoExpressao
inicio
inteiro i, incremento;

incremento recebe 1;

para i recebe 0 ate i menorQue 7 passo i recebe i mais (incremento mais 1) faca
    mostre(i);
fimpara

fim)";

    requireOutput(
        source,
        {},
        {0, 2, 4, 6},
        "Para com expressao composta no passo"
        );
}

void testNestedForAndScopes()
{
    const std::string source = R"(programa paraAninhado
inicio
inteiro i, j, resultado;

para i recebe 0 ate i menorQue 2 passo i recebe i mais 1 faca
    inteiro local;
    local recebe i mais 10;

    para j recebe 0 ate j menorQue 3 passo j recebe j mais 1 faca
        resultado recebe local mais j;
        mostre(resultado);
    fimpara
fimpara

fim)";

    requireOutput(
        source,
        {},
        {10, 11, 12, 11, 12, 13},
        "Para aninhado e escopos internos"
        );
}

void testForWithConditionalAndWhile()
{
    const std::string source = R"(programa paraComDesvios
inicio
inteiro i, j;

para i recebe 0 ate i menorQue 3 passo i recebe i mais 1 faca
    se i igual 1 entao
        mostre(9);
    senao
        mostre(i);
    fimse

    j recebe 0;
    enquanto j menorQue 2 faca
        j recebe j mais 1;
    fimenquanto

    mostre(j);
fimpara

fim)";

    requireOutput(
        source,
        {},
        {0, 2, 9, 2, 2, 2},
        "Para com condicional e enquanto aninhados"
        );
}

void testForInvalidConditionRejected()
{
    const std::string source = R"(programa paraCondicaoInvalida
inicio
inteiro i;

para i recebe 0 ate i mais 1 passo i recebe i mais 1 faca
    mostre(i);
fimpara

fim)";

    const bip::BipCodeGenerator generator;
    const auto result = generator.generate(source);

    require(
        !result.success,
        "Condicao nao logica do para deveria falhar."
        );

    require(
        result.assembly.empty(),
        "Condicao invalida gerou Assembly parcial."
        );

    require(
        !result.message.empty(),
        "Diagnostico de condicao invalida ausente."
        );
}

void testForDeterminism()
{
    const std::string source = R"(programa paraDeterministico
inicio
inteiro i;

para i recebe 0 ate i menorQue 3 passo i recebe i mais 1 faca
    mostre(i);
fimpara

fim)";

    const bip::BipCodeGenerator generator;
    const auto first = generator.generate(source);
    const auto second = generator.generate(source);

    require(
        first.success && second.success,
        "Geracao do para falhou."
        );

    require(
        first.assembly == second.assembly,
        "Geracao do para nao e deterministica."
        );

    AssemblyMachine machine(first.assembly);

    require(
        machine.execute() == std::vector<int>{0, 1, 2},
        "Execucao do para deterministico incorreta."
        );
}

}

int main()
{
    try
    {
        testInitialProgram();
        testAllRelationalOperators();
        testBooleanConditions();
        testVectorsAndExpressions();
        testNestedConditionsAndScopes();
        testNameCollisions();
        testBitwiseAndLogicalSeparation();
        testOutOfScopeRejected();
        testDeterministicGeneration();

        testInitialLoops();
        testInitiallyFalseAndSingleIteration();
        testVectorsAndInputInLoops();
        testComposedConditionsAndNestedLoops();
        testLoopScopesAndLabelCollisions();

        testInitialForWithVectors();
        testForIncrementAndDecrement();
        testForZeroIterations();
        testForCompoundConditions();
        testForIndexExpressions();
        testForExpressionAsStep();
        testNestedForAndScopes();
        testForWithConditionalAndWhile();
        testForInvalidConditionRejected();
        testForDeterminism();

        std::cout
            << "Todos os testes M3.1 passaram.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha nos testes M3.1: "
            << error.what()
            << '\n';

        return 1;
    }
}