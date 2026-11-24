#include "BipCodeGenerator.h"
#include "BipIr.h"
#include "CompilerService.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <memory>
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

void requireContains(
    const std::string &text,
    const std::string &expected
    )
{
    require(
        text.find(expected) != std::string::npos,
        "Trecho ausente no Assembly: " + expected
        );
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

    return text.substr(
        first,
        last - first + 1
        );
}

std::string normalizedName(std::string name)
{
    std::transform(
        name.begin(),
        name.end(),
        name.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::toupper(character)
                );
        }
        );

    return name;
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

std::string formatValues(
    const std::vector<int> &values
    )
{
    std::ostringstream output;

    output << "[";

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i != 0)
        {
            output << ", ";
        }

        output << values[i];
    }

    output << "]";

    return output.str();
}

struct Instruction
{
    std::string opcode;
    std::string operand;
};

struct SimulationResult
{
    std::vector<int> output;

    std::unordered_map<
        std::string,
        std::vector<int>
        > memory;

    std::size_t calls = 0;
    std::size_t returns = 0;
};

// ============================================================
// SIMULADOR DO SUBCONJUNTO BIP UTILIZADO NOS TESTES M3.2
// ============================================================

SimulationResult simulate(
    const std::string &assembly
    )
{
    SimulationResult result;

    std::vector<Instruction> instructions;

    std::unordered_map<
        std::string,
        std::size_t
        > labels;

    std::unordered_set<std::string> allNames;

    std::istringstream input(assembly);
    std::string line;

    enum class Section
    {
        None,
        Data,
        Text
    };

    Section section = Section::None;

    while (std::getline(input, line))
    {
        line = trim(line);

        if (line.empty())
        {
            continue;
        }

        if (line == ".data")
        {
            require(
                section == Section::None,
                "Secao .data fora de ordem."
                );

            section = Section::Data;
            continue;
        }

        if (line == ".text")
        {
            require(
                section == Section::Data,
                "Secao .text fora de ordem."
                );

            section = Section::Text;
            continue;
        }

        if (section == Section::Data)
        {
            const auto separator =
                line.find(':');

            require(
                separator != std::string::npos,
                "Declaracao de memoria invalida: " + line
                );

            const std::string name =
                trim(line.substr(0, separator));

            require(
                !name.empty(),
                "Nome de memoria vazio."
                );

            require(
                allNames.insert(
                            normalizedName(name)
                            ).second,
                "Nome duplicado no Assembly: " + name
                );

            std::istringstream values(
                line.substr(separator + 1)
                );

            std::vector<int> cells;
            std::string value;

            while (std::getline(values, value, ','))
            {
                cells.push_back(
                    word16(std::stoi(trim(value)))
                    );
            }

            require(
                !cells.empty(),
                "Declaracao sem celulas: " + name
                );

            result.memory.emplace(
                name,
                std::move(cells)
                );

            continue;
        }

        require(
            section == Section::Text,
            "Instrucao fora da secao .text."
            );

        if (line.back() == ':')
        {
            const std::string name =
                trim(
                    line.substr(0, line.size() - 1)
                    );

            require(
                allNames.insert(
                            normalizedName(name)
                            ).second,
                "Rotulo duplicado: " + name
                );

            labels.emplace(
                name,
                instructions.size()
                );

            continue;
        }

        Instruction instruction;

        std::istringstream parts(line);

        parts >> instruction.opcode;
        parts >> instruction.operand;

        require(
            !instruction.opcode.empty() &&
                !instruction.operand.empty(),
            "Instrucao invalida: " + line
            );

        std::string extra;

        require(
            !(parts >> extra),
            "Operandos excedentes: " + line
            );

        instructions.push_back(
            std::move(instruction)
            );
    }

    require(
        labels.find("_PRINCIPAL") != labels.end(),
        "Rotulo principal ausente."
        );

    require(
        !instructions.empty(),
        "Assembly sem instrucoes."
        );

    require(
        instructions.size() <= 2048,
        "Limite de instrucoes excedido."
        );

    std::size_t dataCells = 0;

    for (const auto &entry : result.memory)
    {
        dataCells += entry.second.size();
    }

    require(
        dataCells <= 2048,
        "Limite de memoria de dados excedido."
        );

    int acc = 0;
    int indexRegister = 0;

    // A execucao inicia na primeira instrucao da
    // secao .text, antes do rotulo _PRINCIPAL.
    std::size_t pc = 0;

    std::vector<std::size_t> returnStack;

    const auto jumpTarget =
        [&](const std::string &name) -> std::size_t
    {
        const auto found =
            labels.find(name);

        require(
            found != labels.end(),
            "Destino de desvio inexistente: " + name
            );

        require(
            found->second < instructions.size(),
            "Destino de desvio fora da memoria: " + name
            );

        return found->second;
    };

    const auto scalar =
        [&](const std::string &name) -> int &
    {
        const auto found =
            result.memory.find(name);

        require(
            found != result.memory.end(),
            "Memoria inexistente: " + name
            );

        require(
            found->second.size() == 1,
            "Operacao escalar sobre vetor: " + name
            );

        return found->second.front();
    };

    const auto vectorCell =
        [&](const std::string &name) -> int &
    {
        const auto found =
            result.memory.find(name);

        require(
            found != result.memory.end(),
            "Vetor inexistente: " + name
            );

        require(
            indexRegister >= 0 &&
                static_cast<std::size_t>(indexRegister) <
                    found->second.size(),
            "Indice fora dos limites: " + name
            );

        return found->second.at(
            static_cast<std::size_t>(indexRegister)
            );
    };

    const auto immediate =
        [](const std::string &operand) -> int
    {
        const int value = std::stoi(operand);

        require(
            value >= -1024 &&
                value <= 1023,
            "Imediato fora dos limites BIP."
            );

        return value;
    };

    constexpr std::size_t MAX_STEPS = 100000;

    for (std::size_t step = 0;
         step < MAX_STEPS;
         ++step)
    {
        require(
            pc < instructions.size(),
            "Contador de programa fora das instrucoes."
            );

        const Instruction &instruction =
            instructions[pc++];

        const std::string &op =
            instruction.opcode;

        const std::string &arg =
            instruction.operand;

        if (op == "LDI")
        {
            acc = word16(immediate(arg));
        }
        else if (op == "LD")
        {
            acc = scalar(arg);
        }
        else if (op == "STO")
        {
            if (arg == "$out_port")
            {
                result.output.push_back(acc);
            }
            else if (arg == "$indr")
            {
                indexRegister = acc;
            }
            else
            {
                scalar(arg) = acc;
            }
        }
        else if (op == "LDV")
        {
            acc = vectorCell(arg);
        }
        else if (op == "STOV")
        {
            vectorCell(arg) = acc;
        }
        else if (op == "ADD")
        {
            acc = word16(acc + scalar(arg));
        }
        else if (op == "ADDI")
        {
            acc = word16(acc + immediate(arg));
        }
        else if (op == "SUB")
        {
            acc = word16(acc - scalar(arg));
        }
        else if (op == "SUBI")
        {
            acc = word16(acc - immediate(arg));
        }
        else if (op == "AND")
        {
            acc = word16(acc & scalar(arg));
        }
        else if (op == "ANDI")
        {
            acc = word16(acc & immediate(arg));
        }
        else if (op == "OR")
        {
            acc = word16(acc | scalar(arg));
        }
        else if (op == "ORI")
        {
            acc = word16(acc | immediate(arg));
        }
        else if (op == "XOR")
        {
            acc = word16(acc ^ scalar(arg));
        }
        else if (op == "XORI")
        {
            acc = word16(acc ^ immediate(arg));
        }
        else if (op == "NOT")
        {
            acc = word16(~acc);
        }
        else if (op == "JMP")
        {
            pc = jumpTarget(arg);
        }
        else if (op == "BEQ")
        {
            if (acc == 0)
            {
                pc = jumpTarget(arg);
            }
        }
        else if (op == "BNE")
        {
            if (acc != 0)
            {
                pc = jumpTarget(arg);
            }
        }
        else if (op == "BGT")
        {
            if (acc > 0)
            {
                pc = jumpTarget(arg);
            }
        }
        else if (op == "BGE")
        {
            if (acc >= 0)
            {
                pc = jumpTarget(arg);
            }
        }
        else if (op == "BLT")
        {
            if (acc < 0)
            {
                pc = jumpTarget(arg);
            }
        }
        else if (op == "BLE")
        {
            if (acc <= 0)
            {
                pc = jumpTarget(arg);
            }
        }
        else if (op == "CALL")
        {
            // Limite conservador adotado nesta etapa.
            require(
                returnStack.empty(),
                "Chamada aninhada nao suportada pelo teste."
                );

            returnStack.push_back(pc);

            ++result.calls;

            pc = jumpTarget(arg);
        }
        else if (op == "RETURN")
        {
            require(
                arg == "0",
                "Operando de RETURN invalido."
                );

            require(
                !returnStack.empty(),
                "RETURN sem CALL correspondente."
                );

            pc = returnStack.back();

            returnStack.pop_back();

            ++result.returns;
        }
        else if (op == "HLT")
        {
            require(
                arg == "0",
                "Operando de HLT invalido."
                );

            require(
                returnStack.empty(),
                "Programa encerrou dentro de uma rotina."
                );

            require(
                result.calls == result.returns,
                "Quantidade de CALL e RETURN divergente."
                );

            return result;
        }
        else
        {
            throw std::runtime_error(
                "Instrucao nao suportada pelo simulador "
                "de testes M3.2: " + op
                );
        }
    }

    throw std::runtime_error(
        "Limite de passos excedido. Possivel laco infinito."
        );
}

AnalysisResult compileSuccessfully(
    const std::string &source,
    const std::string &testName
    )
{
    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    require(
        result.success,
        testName + ": compilacao falhou: " +
            result.message
        );

    require(
        result.errors.empty(),
        testName + ": erros em resultado de sucesso."
        );

    require(
        !result.assembly.empty(),
        testName + ": Assembly vazio."
        );

    return result;
}

void expectOutput(
    const std::string &source,
    const std::vector<int> &expected,
    std::size_t expectedCalls,
    const std::string &testName
    )
{
    const AnalysisResult result =
        compileSuccessfully(source, testName);

    const SimulationResult simulation =
        simulate(result.assembly);

    require(
        simulation.output == expected,
        testName +
            ": esperado " + formatValues(expected) +
            ", recebido " + formatValues(simulation.output)
        );

    require(
        simulation.calls == expectedCalls,
        testName + ": quantidade de CALL incorreta."
        );

    require(
        simulation.returns == expectedCalls,
        testName + ": quantidade de RETURN incorreta."
        );
}

void expectFailure(
    const std::string &source,
    const std::string &testName,
    const std::string &expectedMessage = ""
    )
{
    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    require(
        !result.success,
        testName + ": deveria falhar."
        );

    require(
        !result.errors.empty(),
        testName + ": falha sem diagnostico."
        );

    require(
        result.assembly.empty(),
        testName + ": Assembly parcial foi publicado."
        );

    if (!expectedMessage.empty())
    {
        require(
            result.message.find(expectedMessage) !=
                std::string::npos,
            testName + ": diagnostico inesperado: " +
                result.message
            );
    }
}

// ============================================================
// TESTES FUNCIONAIS M3.2
// ============================================================

void testProcedureWithoutArguments()
{
    expectOutput(
        R"(programa semParametros

procedimento exibir()
inicio
mostre(7);
fimprocedimento

inicio
mostre(1);
exibir();
mostre(2);
fim)",
        {1, 7, 2},
        1,
        "Procedimento sem parametros"
        );
}

void testProcedureWithOneArgument()
{
    expectOutput(
        R"(programa exemplo

procedimento exibir(inteiro x)
inicio
mostre(x);
fimprocedimento

inicio
inteiro numero;
numero recebe 7;
exibir(numero);
fim)",
        {7},
        1,
        "Procedimento com um argumento"
        );
}

void testMultipleArguments()
{
    // A ordem alfabetica dos nomes difere da assinatura.
    expectOutput(
        R"(programa multiplosParametros

procedimento exibir(inteiro z, inteiro a)
inicio
mostre(z);
mostre(a);
fimprocedimento

inicio
exibir(17, 29);
fim)",
        {17, 29},
        1,
        "Ordem dos parametros formais"
        );
}

void testIndependentCopy()
{
    const std::string source =
        R"(programa copiaIndependente

procedimento alterar(inteiro valor)
inicio
valor recebe 20;
mostre(valor);
fimprocedimento

inicio
inteiro numero;
numero recebe 7;
alterar(numero);
mostre(numero);
fim)";

    const AnalysisResult result =
        compileSuccessfully(
            source,
            "Passagem por copia"
            );

    const SimulationResult simulation =
        simulate(result.assembly);

    require(
        simulation.output == std::vector<int>{20, 7},
        "Alteracao do parametro modificou o argumento."
        );

    require(
        simulation.memory.at("numero").front() == 7,
        "Valor original nao foi preservado na memoria."
        );

    require(
        simulation.calls == 1 &&
            simulation.returns == 1,
        "Fluxo de chamada invalido."
        );
}

void testRepeatedArgument()
{
    expectOutput(
        R"(programa argumentoRepetido

procedimento alterar(inteiro primeiro, inteiro segundo)
inicio
primeiro recebe 30;
mostre(primeiro);
mostre(segundo);
fimprocedimento

inicio
inteiro numero;
numero recebe 5;
alterar(numero, numero);
mostre(numero);
fim)",
        {30, 5, 5},
        1,
        "Parametros independentes com o mesmo argumento"
        );
}

void testLiteralsAndVariables()
{
    expectOutput(
        R"(programa argumentos

procedimento exibir(inteiro x, inteiro y)
inicio
mostre(x);
mostre(y);
fimprocedimento

inicio
inteiro numero;
numero recebe 9;
exibir(numero, 15);
exibir(3, numero);
fim)",
        {9, 15, 3, 9},
        2,
        "Argumentos literais e variaveis"
        );
}

void testComplexExpressions()
{
    expectOutput(
        R"(programa expressoesArgumentos

procedimento exibir(inteiro primeiro, inteiro segundo)
inicio
mostre(primeiro);
mostre(segundo);
fimprocedimento

inicio
inteiro valor;
valor recebe 7;
exibir(valor mais (3 menos 2),
       (10 mais 5) menos (valor mais 1));
mostre(valor);
fim)",
        {8, 7, 7},
        1,
        "Protecao dos temporarios dos argumentos"
        );
}

void testVectorArguments()
{
    expectOutput(
        R"(programa argumentosVetoriais

procedimento exibir(inteiro primeiro, inteiro segundo)
inicio
primeiro recebe primeiro mais 10;
mostre(primeiro);
mostre(segundo);
fimprocedimento

inicio
inteiro valores[3], indice;
valores[0] recebe 4;
valores[1] recebe 12;
valores[2] recebe 20;
indice recebe 1;

exibir(valores[indice],
       valores[indice mais 1] menos valores[0]);

mostre(valores[1]);
mostre(valores[2]);
fim)",
        {22, 16, 12, 20},
        1,
        "Elementos de vetor por copia"
        );
}

void testRepeatedCalls()
{
    expectOutput(
        R"(programa chamadasRepetidas

procedimento exibir(inteiro valor)
inicio
valor recebe valor mais 1;
mostre(valor);
fimprocedimento

inicio
exibir(10);
exibir(20);
exibir(10);
fim)",
        {11, 21, 11},
        3,
        "Reutilizacao das celulas de parametros"
        );
}

void testLocalVariables()
{
    expectOutput(
        R"(programa variaveisLocais

procedimento primeiro(inteiro x)
inicio
inteiro resultado;
resultado recebe x mais 1;
mostre(resultado);
fimprocedimento

procedimento segundo(inteiro x)
inicio
inteiro resultado;
resultado recebe x mais 10;
mostre(resultado);
fimprocedimento

inicio
inteiro resultado;
resultado recebe 100;
primeiro(5);
segundo(5);
mostre(resultado);
fim)",
        {6, 15, 100},
        2,
        "Isolamento das variaveis locais"
        );
}

void testUnusedRoutine()
{
    expectOutput(
        R"(programa rotinaNaoChamada

procedimento exibir()
inicio
mostre(99);
fimprocedimento

inicio
mostre(1);
fim)",
        {1},
        0,
        "Rotina nao executada acidentalmente"
        );
}

void testNameCollisions()
{
    expectOutput(
        R"(programa colisoes

procedimento primeira(inteiro temp1)
inicio
inteiro R1;
R1 recebe temp1 mais 1;
mostre(R1);
fimprocedimento

procedimento segunda(inteiro temp1)
inicio
mostre(temp1);
fimprocedimento

inicio
inteiro _ROTINA_1, temp1, R1;
_ROTINA_1 recebe 10;
temp1 recebe 20;
R1 recebe 30;

primeira(_ROTINA_1);
segunda(temp1);

mostre(_ROTINA_1);
mostre(temp1);
mostre(R1);
fim)",
        {11, 20, 10, 20, 30},
        2,
        "Colisoes de nomes, parametros e rotulos"
        );
}

void testCallsInsideControlStructures()
{
    expectOutput(
        R"(programa chamadaEmLaco

procedimento exibir(inteiro valor)
inicio
se valor maiorQue 0 entao
    inteiro local;
    local recebe valor mais 10;
    mostre(local);
senao
    mostre(0);
fimse
fimprocedimento

inicio
inteiro contador;
contador recebe 0;

enquanto contador menorQue 3 faca
    exibir(contador);
    contador recebe contador mais 1;
fimenquanto

mostre(contador);
fim)",
        {0, 11, 12, 3},
        3,
        "Integracao com condicionais, lacos e escopos"
        );
}

void testAssemblyOrder()
{
    const std::string source =
        R"(programa ordemAssembly

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir(7);
fim)";

    const AnalysisResult result =
        compileSuccessfully(
            source,
            "Ordem do Assembly"
            );

    const std::string &assembly =
        result.assembly;

    const auto textPosition =
        assembly.find(".text\n");

    const auto jumpPosition =
        assembly.find("JMP _PRINCIPAL");

    const auto routinePosition =
        assembly.find("_EXIBIR:");

    const auto returnPosition =
        assembly.find("RETURN 0");

    const auto principalPosition =
        assembly.find("_PRINCIPAL:");

    const auto callPosition =
        assembly.find("CALL _EXIBIR");

    const auto haltPosition =
        assembly.find("HLT 0");

    require(
        textPosition != std::string::npos &&
            jumpPosition != std::string::npos &&
            routinePosition != std::string::npos &&
            returnPosition != std::string::npos &&
            principalPosition != std::string::npos &&
            callPosition != std::string::npos &&
            haltPosition != std::string::npos,
        "Assembly de chamada incompleto."
        );

    require(
        textPosition < jumpPosition &&
            jumpPosition < routinePosition &&
            routinePosition < returnPosition &&
            returnPosition < principalPosition &&
            principalPosition < callPosition &&
            callPosition < haltPosition,
        "Ordem incorreta das instrucoes de controle."
        );

    const SimulationResult simulation =
        simulate(assembly);

    require(
        simulation.output == std::vector<int>{7},
        "Fluxo de execucao incorreto."
        );
}

void testFormalParameterMetadata()
{
    const std::string source =
        R"(programa metadados

procedimento exibir(inteiro z, inteiro a)
inicio
mostre(z);
mostre(a);
fimprocedimento

inicio
exibir(1, 2);
fim)";

    const CompilerService compiler;

    const AnalysisResult analysis =
        compiler.analyze(source);

    require(
        analysis.success,
        "Falha na analise dos parametros: " +
            analysis.message
        );

    const bip::BipIrBuilder builder;

    const bip::Program program =
        builder.build(source, analysis);

    require(
        program.routines.size() == 1,
        "Quantidade incorreta de rotinas no IR."
        );

    const bip::Routine &routine =
        program.routines.front();

    require(
        routine.scope == "global::exibir",
        "Escopo incorreto da rotina."
        );

    require(
        routine.label == "_EXIBIR",
        "Rotulo nao segue o padrao dos slides."
        );

    require(
        routine.parameters.size() == 2,
        "Quantidade incorreta de parametros no IR."
        );

    require(
        routine.parameters[0].name == "z" &&
            routine.parameters[0].position == 1 &&
            routine.parameters[0].type == "inteiro" &&
            routine.parameters[0].assemblyName == "EXIBIR_z",
        "Primeiro parametro incorreto."
        );

    require(
        routine.parameters[1].name == "a" &&
            routine.parameters[1].position == 2 &&
            routine.parameters[1].type == "inteiro" &&
            routine.parameters[1].assemblyName == "EXIBIR_a",
        "Segundo parametro incorreto."
        );

    require(
        routine.parameters[0].assemblyName !=
            routine.parameters[1].assemblyName,
        "Parametros possuem a mesma celula de memoria."
        );

    for (const bip::FormalParameter &parameter :
         routine.parameters)
    {
        const auto found =
            std::find_if(
                program.declarations.begin(),
                program.declarations.end(),
                [&](const bip::Declaration &declaration)
                {
                    return declaration.name ==
                           parameter.assemblyName;
                }
                );

        require(
            found != program.declarations.end(),
            "Parametro ausente das declaracoes de memoria."
            );

        require(
            found->size == 1 &&
                !found->isVector,
            "Parametro escalar com declaracao invalida."
            );
    }
}

void testDeterministicGeneration()
{
    const std::string source =
        R"(programa determinismo

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir(10);
exibir(20);
fim)";

    const bip::BipCodeGenerator generator;

    const bip::CodegenResult first =
        generator.generate(source);

    const bip::CodegenResult second =
        generator.generate(source);

    require(
        first.success && second.success,
        "Falha na geracao deterministica."
        );

    require(
        first.assembly == second.assembly,
        "Assembly varia entre compilacoes identicas."
        );
}

// ============================================================
// TESTES DE NOMENCLATURA CONFORME OS SLIDES
// ============================================================

void testReadableRoutineNames()
{
    const std::string source =
        R"(programa nomesLegiveis

procedimento exibir(inteiro x)
inicio
inteiro resultado;
resultado recebe x mais 1;
mostre(resultado);
fimprocedimento

procedimento dobro(inteiro x)
inicio
inteiro resultado;
resultado recebe x mais x;
mostre(resultado);
fimprocedimento

inicio
exibir(5);
dobro(7);
fim)";

    const AnalysisResult result =
        compileSuccessfully(
            source,
            "Nomes legiveis de rotinas"
            );

    requireContains(
        result.assembly,
        "EXIBIR_x: 0"
        );

    requireContains(
        result.assembly,
        "EXIBIR_resultado: 0"
        );

    requireContains(
        result.assembly,
        "DOBRO_x: 0"
        );

    requireContains(
        result.assembly,
        "DOBRO_resultado: 0"
        );

    requireContains(
        result.assembly,
        "_EXIBIR:"
        );

    requireContains(
        result.assembly,
        "_DOBRO:"
        );

    requireContains(
        result.assembly,
        "CALL _EXIBIR"
        );

    requireContains(
        result.assembly,
        "CALL _DOBRO"
        );

    requireContains(
        result.assembly,
        "STO EXIBIR_x"
        );

    requireContains(
        result.assembly,
        "LD EXIBIR_x"
        );

    require(
        result.assembly.find("__bip_local_") ==
            std::string::npos,
        "Rotina utiliza nome local generico."
        );

    const SimulationResult simulation =
        simulate(result.assembly);

    require(
        simulation.output == std::vector<int>{6, 14},
        "Novos nomes alteraram o resultado da execucao."
        );
}

void testReadableInternalScopes()
{
    const std::string source =
        R"(programa escoposLegiveis

procedimento exibir(inteiro x)
inicio
inteiro resultado;
resultado recebe x;

se x maiorQue 0 entao
    inteiro resultado;
    resultado recebe 20;
    mostre(resultado);
senao
    inteiro resultado;
    resultado recebe 30;
    mostre(resultado);
fimse

mostre(resultado);
fimprocedimento

inicio
exibir(5);
exibir(0);
fim)";

    const AnalysisResult result =
        compileSuccessfully(
            source,
            "Nomes de escopos internos"
            );

    requireContains(
        result.assembly,
        "EXIBIR_x: 0"
        );

    requireContains(
        result.assembly,
        "EXIBIR_resultado: 0"
        );

    requireContains(
        result.assembly,
        "EXIBIR_ENTAO_1_resultado: 0"
        );

    requireContains(
        result.assembly,
        "EXIBIR_SENAO_2_resultado: 0"
        );

    const SimulationResult simulation =
        simulate(result.assembly);

    require(
        simulation.output ==
            std::vector<int>{20, 5, 30, 0},
        "Escopos internos nao preservaram seus valores."
        );
}

void testReadableNameCollisions()
{
    const std::string source =
        R"(programa colisaoNomesLegiveis

procedimento exibir(inteiro x)
inicio
mostre(x);
fimprocedimento

inicio
inteiro exibir_x, _exibir;

exibir_x recebe 100;
_exibir recebe 200;

exibir(7);

mostre(exibir_x);
mostre(_exibir);
fim)";

    const AnalysisResult result =
        compileSuccessfully(
            source,
            "Colisao de nomes legiveis"
            );

    // Os nomes globais originais devem permanecer.
    requireContains(
        result.assembly,
        "exibir_x: 0"
        );

    requireContains(
        result.assembly,
        "_exibir: 0"
        );

    // O parametro e o rotulo recebem sufixos.
    requireContains(
        result.assembly,
        "EXIBIR_x_2: 0"
        );

    requireContains(
        result.assembly,
        "_EXIBIR_2:"
        );

    requireContains(
        result.assembly,
        "CALL _EXIBIR_2"
        );

    requireContains(
        result.assembly,
        "STO EXIBIR_x_2"
        );

    requireContains(
        result.assembly,
        "LD EXIBIR_x_2"
        );

    const SimulationResult simulation =
        simulate(result.assembly);

    require(
        simulation.output ==
            std::vector<int>{7, 100, 200},
        "Colisao alterou os valores originais."
        );
}

void testReservedPrincipalLabel()
{
    const std::string source =
        R"(programa rotuloReservado

procedimento principal()
inicio
mostre(7);
fimprocedimento

inicio
principal();
mostre(9);
fim)";

    const AnalysisResult result =
        compileSuccessfully(
            source,
            "Protecao do rotulo principal"
            );

    requireContains(
        result.assembly,
        "JMP _PRINCIPAL"
        );

    requireContains(
        result.assembly,
        "_PRINCIPAL_2:"
        );

    requireContains(
        result.assembly,
        "CALL _PRINCIPAL_2"
        );

    const SimulationResult simulation =
        simulate(result.assembly);

    require(
        simulation.output ==
            std::vector<int>{7, 9},
        "Colisao com o rotulo principal."
        );

    require(
        simulation.calls == 1 &&
            simulation.returns == 1,
        "Fluxo de chamada invalido."
        );
}

// ============================================================
// OPERACOES NAO SUPORTADAS E DIAGNOSTICOS
// ============================================================

void testUnsupportedParameterType()
{
    expectFailure(
        R"(programa parametroFlutuante

procedimento exibir(flutuante valor)
inicio
mostre(1);
fimprocedimento

inicio
exibir(1.5);
fim)",
        "Parametro flutuante",
        "parametros escalares inteiros"
        );
}

void testUnsupportedVectorParameter()
{
    expectFailure(
        R"(programa parametroVetorial

procedimento exibir(inteiro valores[3])
inicio
mostre(valores[0]);
fimprocedimento

inicio
mostre(1);
fim)",
        "Parametro vetorial",
        "Passagem integral de vetores"
        );
}

void testUnsupportedOperationInArgument()
{
    expectFailure(
        R"(programa argumentoInvalido

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir(2 vezes 3);
fim)",
        "Multiplicacao no argumento",
        "sem suporte"
        );
}

void testUnsupportedFunction()
{
    expectFailure(
        R"(programa funcaoComRetorno

funcao inteiro dobro(inteiro valor)
inicio
retorne valor mais valor;
fimfuncao

inicio
inteiro numero;
numero recebe dobro(7);
mostre(numero);
fim)",
        "Funcao utilizada em expressao",
        "Funcoes com retorno"
        );
}

void testUnsupportedRecursion()
{
    expectFailure(
        R"(programa recursao

procedimento repetir(inteiro valor)
inicio
repetir(valor);
fimprocedimento

inicio
repetir(1);
fim)",
        "Recursao direta",
        "Recursao"
        );
}

void testUnsupportedNestedCall()
{
    expectFailure(
        R"(programa chamadaAninhada

procedimento primeira()
inicio
mostre(1);
fimprocedimento

procedimento segunda()
inicio
primeira();
fimprocedimento

inicio
segunda();
fim)",
        "Chamada entre rotinas",
        "Chamadas entre sub-rotinas"
        );
}

void testWrongArgumentCount()
{
    expectFailure(
        R"(programa quantidadeIncorreta

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir();
fim)",
        "Quantidade incorreta de argumentos",
        "esperava 1 parametros"
        );
}

void testUnknownRoutine()
{
    expectFailure(
        R"(programa rotinaInexistente

inicio
inexistente();
fim)",
        "Rotina inexistente",
        "nao foi declarada"
        );
}

void testMemoryLimit()
{
    bip::Program program;

    program.declarations.push_back(
        {"vetor", 2048, true}
        );

    program.declarations.push_back(
        {"parametro", 1, false}
        );

    const bip::BipEmitter emitter;

    bool rejected = false;

    try
    {
        emitter.emit(program);
    }
    catch (const bip::CodegenError &error)
    {
        rejected =
            std::string(error.what()).find(
                "memoria"
                ) != std::string::npos;
    }

    require(
        rejected,
        "Limite da memoria de dados nao respeitado."
        );
}

void testInstructionLimit()
{
    bip::Program program;

    auto literal =
        std::make_shared<bip::Expression>();

    literal->operation = bip::Operation::Literal;
    literal->value = 1;

    bip::Statement statement;

    statement.kind = bip::StatementKind::Output;
    statement.expression = literal;

    // Cada saida produz LDI e STO.
    // 1024 saidas + HLT excedem 2048 instrucoes.
    program.statements.assign(
        1024,
        statement
        );

    const bip::BipEmitter emitter;

    bool rejected = false;

    try
    {
        emitter.emit(program);
    }
    catch (const bip::CodegenError &error)
    {
        rejected =
            std::string(error.what()).find(
                "memoria de instrucoes"
                ) != std::string::npos;
    }

    require(
        rejected,
        "Limite da memoria de instrucoes nao respeitado."
        );
}

void testPreviousProgramRegression()
{
    const std::string source =
        R"(programa regressao

inicio
inteiro numero;
numero recebe 7;
mostre(numero);
fim)";

    const AnalysisResult result =
        compileSuccessfully(
            source,
            "Regressao de programa sem rotinas"
            );

    const std::string expected =
        ".data\n"
        "numero: 0\n"
        ".text\n"
        "_PRINCIPAL:\n"
        "    LDI 7\n"
        "    STO numero\n"
        "    LD numero\n"
        "    STO $out_port\n"
        "    HLT 0\n";

    require(
        result.assembly == expected,
        "Formato anterior do Assembly foi alterado."
        );

    const SimulationResult simulation =
        simulate(result.assembly);

    require(
        simulation.output == std::vector<int>{7},
        "Regressao na execucao de programa simples."
        );

    require(
        simulation.calls == 0,
        "CALL indevido em programa sem rotinas."
        );
}

}

int main()
{
    try
    {
        // Chamadas e passagem de parametros.
        testProcedureWithoutArguments();
        testProcedureWithOneArgument();
        testMultipleArguments();
        testIndependentCopy();
        testRepeatedArgument();
        testLiteralsAndVariables();
        testComplexExpressions();
        testVectorArguments();
        testRepeatedCalls();
        testLocalVariables();
        testUnusedRoutine();
        testNameCollisions();
        testCallsInsideControlStructures();
        testAssemblyOrder();
        testFormalParameterMetadata();
        testDeterministicGeneration();

        // Nomenclatura de acordo com os slides.
        testReadableRoutineNames();
        testReadableInternalScopes();
        testReadableNameCollisions();
        testReservedPrincipalLabel();

        // Rejeicao de recursos nao suportados.
        testUnsupportedParameterType();
        testUnsupportedVectorParameter();
        testUnsupportedOperationInArgument();
        testUnsupportedFunction();
        testUnsupportedRecursion();
        testUnsupportedNestedCall();
        testWrongArgumentCount();
        testUnknownRoutine();

        // Limites da arquitetura.
        testMemoryLimit();
        testInstructionLimit();

        // Regressao.
        testPreviousProgramRegression();

        std::cout
            << "Todos os testes de geracao BIP M3.2 passaram."
            << std::endl;

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha nos testes BIP M3.2: "
            << error.what()
            << std::endl;

        return 1;
    }
}