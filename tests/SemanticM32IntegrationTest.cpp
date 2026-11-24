#include "CompilerService.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

struct TestCase
{
    std::string name;
    std::string source;

    bool expectedSuccess;

    std::string expectedError;
    std::string expectedWarning;

    std::optional<std::size_t> exactWarningCount;
};

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

bool containsSemanticDiagnostic(
    const std::vector<AnalysisDiagnostic> &diagnostics,
    const std::string &expected
    )
{
    return std::any_of(
        diagnostics.begin(),
        diagnostics.end(),
        [&expected](const AnalysisDiagnostic &diagnostic)
        {
            return
                diagnostic.phase == AnalysisPhase::Semantic &&
                diagnostic.message.find(expected) !=
                    std::string::npos;
        }
        );
}

void runTest(
    const CompilerService &compiler,
    const TestCase &test
    )
{
    std::cout
        << "========================================\n"
        << test.name
        << "\n----------------------------------------\n"
        << "Codigo-fonte:\n"
        << test.source
        << "\n----------------------------------------\n";

    const AnalysisResult result =
        compiler.analyze(test.source);

    require(
        result.success == test.expectedSuccess,
        test.name +
            ": resultado diferente do esperado.\n" +
            result.message
        );

    const std::size_t expectedErrorCount =
        test.expectedSuccess ? 0 : 1;

    require(
        result.errors.size() == expectedErrorCount,
        test.name +
            ": quantidade de erros diferente do esperado.\n" +
            result.message
        );

    if (!test.expectedError.empty())
    {
        require(
            containsSemanticDiagnostic(
                result.errors,
                test.expectedError
                ),
            test.name +
                ": erro semantico esperado nao encontrado: " +
                test.expectedError +
                "\n" +
                result.message
            );

        require(
            result.errors.front().position >= 0,
            test.name +
                ": erro semantico sem posicao valida."
            );
    }

    if (!test.expectedWarning.empty())
    {
        require(
            containsSemanticDiagnostic(
                result.warnings,
                test.expectedWarning
                ),
            test.name +
                ": aviso semantico esperado nao encontrado: " +
                test.expectedWarning +
                "\n" +
                result.message
            );
    }

    if (test.exactWarningCount.has_value())
    {
        require(
            result.warnings.size() ==
                test.exactWarningCount.value(),
            test.name +
                ": quantidade de avisos diferente do esperado.\n" +
                result.message
            );
    }

    // A etapa utiliza exclusivamente a analise semantica.
    require(
        result.assembly.empty(),
        test.name +
            ": a analise semantica nao deveria gerar Assembly."
        );

    std::cout
        << "Resultado obtido:\n"
        << result.message
        << "\n[OK] "
        << test.name
        << '\n';
}

std::vector<TestCase> buildTests()
{
    return {

        // ====================================================
        // 01 - PROCEDIMENTO SEM PARAMETROS
        // ====================================================

        {
            "01 - Procedimento sem parametros",

            R"(programa procedimentoSemParametros

procedimento exibir()
inicio
mostre(10);
fimprocedimento

inicio
exibir();
fim)",

            true,
            "",
            "",
            0
        },

            // ====================================================
            // 02 - PROCEDIMENTO COM UM PARAMETRO
            // ====================================================

            {
                "02 - Procedimento com um parametro",

                R"(programa procedimentoUmParametro

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir(10);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 03 - MULTIPLOS PARAMETROS E ORDEM FORMAL
            // ====================================================

            {
                "03 - Procedimento com multiplos parametros",

                R"(programa multiplosParametros

procedimento exibir(inteiro z, texto a, logico m)
inicio
mostre(z);
mostre(a);
mostre(m);
fimprocedimento

inicio
exibir(10, "teste", verdadeiro);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 04 - FUNCAO COM PARAMETROS
            // ====================================================

            {
                "04 - Funcao com parametros",

                R"(programa funcaoComParametros

funcao inteiro soma(inteiro z, inteiro a)
inicio
retorne z mais a;
fimfuncao

inicio
inteiro resultado;
resultado recebe soma(10, 20);
mostre(resultado);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 05 - QUANTIDADE INSUFICIENTE
            // ====================================================

            {
                "05 - Quantidade insuficiente de argumentos",

                R"(programa argumentosInsuficientes

funcao inteiro soma(inteiro a, inteiro b)
inicio
retorne a mais b;
fimfuncao

inicio
inteiro resultado;
resultado recebe soma(10);
mostre(resultado);
fim)",

                false,
                "A funcao 'soma' esperava 2 parametros, mas recebeu 1.",
                "",
                std::nullopt
            },

            // ====================================================
            // 06 - QUANTIDADE EXCESSIVA
            // ====================================================

            {
                "06 - Quantidade excessiva de argumentos",

                R"(programa argumentosExcessivos

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir(10, 20);
fim)",

                false,
                "O procedimento 'exibir' esperava 1 parametros, mas recebeu 2.",
                "",
                std::nullopt
            },

            // ====================================================
            // 07 - TIPOS INCOMPATIVEIS
            // ====================================================

            {
                "07 - Argumento de tipo incompativel",

                R"(programa argumentosIncompativeis

funcao inteiro soma(inteiro a, inteiro b)
inicio
retorne a mais b;
fimfuncao

inicio
inteiro resultado;
resultado recebe soma(10, "teste");
mostre(resultado);
fim)",

                false,
                "O argumento 2 da funcao 'soma' possui tipo incompativel: esperado INTEIRO, recebido TEXTO.",
                "",
                std::nullopt
            },

            // ====================================================
            // 08 - ARGUMENTOS EM ORDEM INCORRETA
            // ====================================================

            {
                "08 - Argumentos em ordem incorreta",

                R"(programa argumentosOrdemIncorreta

procedimento exibir(inteiro z, texto a)
inicio
mostre(z);
mostre(a);
fimprocedimento

inicio
exibir("teste", 10);
fim)",

                false,
                "O argumento 1 do procedimento 'exibir' possui tipo incompativel: esperado INTEIRO, recebido TEXTO.",
                "",
                std::nullopt
            },

            // ====================================================
            // 09 - CHAMADAS ANINHADAS
            // ====================================================

            {
                "09 - Chamadas aninhadas",

                R"(programa chamadasAninhadas

funcao inteiro dobro(inteiro valor)
inicio
retorne valor mais valor;
fimfuncao

funcao inteiro soma(inteiro a, inteiro b)
inicio
retorne a mais b;
fimfuncao

inicio
inteiro resultado;
resultado recebe soma(dobro(2), dobro(3));
mostre(resultado);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 10 - PROCEDIMENTO COMO EXPRESSAO
            // ====================================================

            {
                "10 - Procedimento utilizado como expressao",

                R"(programa procedimentoExpressao

procedimento exibir()
inicio
mostre(10);
fimprocedimento

inicio
inteiro resultado;
resultado recebe exibir();
mostre(resultado);
fim)",

                false,
                "Procedimento 'exibir' nao pode participar de uma expressao.",
                "",
                std::nullopt
            },

            // ====================================================
            // 11 - SUB-ROTINA INEXISTENTE
            // ====================================================

            {
                "11 - Sub-rotina inexistente",

                R"(programa rotinaInexistente
inicio
coisa();
fim)",

                false,
                "Sub-rotina 'coisa' nao foi declarada.",
                "",
                std::nullopt
            },

            // ====================================================
            // 12 - CONVERSAO DE INTEIRO PARA FLUTUANTE
            // ====================================================

            {
                "12 - Conversao permitida sem aviso",

                R"(programa conversaoPermitida

procedimento exibir(flutuante valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir(10);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 13 - CONVERSAO DE FLUTUANTE PARA INTEIRO
            // ====================================================

            {
                "13 - Conversao permitida com aviso",

                R"(programa conversaoComAviso

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
exibir(10.5);
fim)",

                true,
                "",
                "O argumento 1 do procedimento 'exibir' realiza conversao de FLUTUANTE para INTEIRO",
                1
            },

            // ====================================================
            // 14 - CONVERSAO DE CARACTERE PARA TEXTO
            // ====================================================

            {
                "14 - Parametro texto recebe caractere",

                R"(programa caractereParaTexto

procedimento exibir(texto mensagem)
inicio
mostre(mensagem);
fimprocedimento

inicio
exibir('A');
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 15 - VETOR INDEXADO COMO ARGUMENTO ESCALAR
            // ====================================================

            {
                "15 - Elemento de vetor como argumento",

                R"(programa elementoVetor

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
inteiro valores[3];
valores[0] recebe 10;
exibir(valores[0]);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 16 - PARAMETRO VETORIAL RECEBE ESCALAR
            // ====================================================

            {
                "16 - Parametro vetorial recebe escalar",

                R"(programa parametroVetorial

procedimento exibir(inteiro valores[3])
inicio
mostre(valores[0]);
fimprocedimento

inicio
exibir(10);
fim)",

                false,
                "O argumento 1 do procedimento 'exibir' corresponde ao parametro vetorial 'valores'. A passagem integral de vetores por parametro nao e suportada.",
                "",
                std::nullopt
            },

            // ====================================================
            // 17 - VETOR INTEGRAL COMO ARGUMENTO
            // ====================================================

            {
                "17 - Vetor sem indice como argumento",

                R"(programa vetorSemIndice

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
inteiro valores[3];
valores[0] recebe 10;
exibir(valores);
fim)",

                false,
                "Vetor 'valores' deve possuir um indice para ser usado como valor.",
                "",
                std::nullopt
            },

            // ====================================================
            // 18 - IDENTIFICADOR NAO E SUB-ROTINA
            // ====================================================

            {
                "18 - Variavel utilizada como sub-rotina",

                R"(programa variavelComoRotina
inicio
inteiro valor;
valor recebe 10;
mostre(valor);
valor();
fim)",

                false,
                "Identificador 'valor' nao e uma sub-rotina.",
                "",
                std::nullopt
            },

            // ====================================================
            // 19 - CHAMADAS CONSECUTIVAS E ANINHADAS
            // ====================================================

            {
                "19 - Isolamento entre contextos de chamada",

                R"(programa isolamentoChamadas

funcao inteiro soma(inteiro a, inteiro b)
inicio
retorne a mais b;
fimfuncao

inicio
inteiro resultado;
resultado recebe soma(1, 2);
resultado recebe resultado mais soma(3, soma(4, 5));
mostre(resultado);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 20 - ERRO NA CHAMADA MAIS INTERNA
            // ====================================================

            {
                "20 - Erro em chamada aninhada",

                R"(programa erroChamadaAninhada

funcao inteiro dobro(inteiro valor)
inicio
retorne valor mais valor;
fimfuncao

funcao inteiro soma(inteiro a, inteiro b)
inicio
retorne a mais b;
fimfuncao

inicio
inteiro resultado;
resultado recebe soma(1, dobro("teste"));
mostre(resultado);
fim)",

                false,
                "O argumento 1 da funcao 'dobro' possui tipo incompativel: esperado INTEIRO, recebido TEXTO.",
                "",
                std::nullopt
            },

            // ====================================================
            // 21 - RETORNO INCOMPATIVEL COM ATRIBUICAO
            // ====================================================

            {
                "21 - Preservacao da compatibilidade do retorno",

                R"(programa retornoIncompativel

funcao inteiro dobro(inteiro valor)
inicio
retorne valor mais valor;
fimfuncao

inicio
texto mensagem;
mensagem recebe dobro(10);
mostre(mensagem);
fim)",

                false,
                "Atribuicao incompativel: identificador 'mensagem' e do tipo TEXTO, mas a expressao resulta em INTEIRO",
                "",
                std::nullopt
            },

            // ====================================================
            // 22 - FUNCAO UTILIZADA COMO COMANDO
            // ====================================================

            {
                "22 - Funcao chamada como comando",

                R"(programa funcaoComoComando

funcao inteiro dobro(inteiro valor)
inicio
retorne valor mais valor;
fimfuncao

inicio
dobro(10);
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 23 - PARAMETROS DE ESCOPOS DISTINTOS
            // ====================================================

            {
                "23 - Isolamento dos parametros formais",

                R"(programa escoposDosParametros

procedimento primeiro(inteiro valor)
inicio
mostre(valor);
fimprocedimento

procedimento segundo(texto valor)
inicio
mostre(valor);
fimprocedimento

inicio
primeiro(10);
segundo("teste");
fim)",

                true,
                "",
                "",
                0
            },

            // ====================================================
            // 24 - PROCEDIMENTO SEM PARAMETROS RECEBE ARGUMENTO
            // ====================================================

            {
                "24 - Argumento em rotina sem parametros",

                R"(programa rotinaSemParametros

procedimento exibir()
inicio
mostre(10);
fimprocedimento

inicio
exibir(20);
fim)",

                false,
                "O procedimento 'exibir' esperava 0 parametros, mas recebeu 1.",
                "",
                std::nullopt
            },

            // ====================================================
            // 25 - EXPRESSAO COMO ARGUMENTO
            // ====================================================

            {
                "25 - Tipo resultante da expressao como argumento",

                R"(programa expressaoComoArgumento

procedimento exibir(inteiro valor)
inicio
mostre(valor);
fimprocedimento

inicio
inteiro numero;
numero recebe 10;
exibir((numero mais 2) vezes 3);
fim)",

                true,
                "",
                "",
                0
            }
    };
}

}

int main()
{
    try
    {
        const CompilerService compiler;

        const std::vector<TestCase> tests =
            buildTests();

        for (const TestCase &test : tests)
        {
            runTest(
                compiler,
                test
                );
        }

        std::cout
            << "========================================\n"
            << "Todos os testes semanticos da M3.2 passaram.\n"
            << "Total de casos: "
            << tests.size()
            << '\n';

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha nos testes semanticos da M3.2: "
            << error.what()
            << std::endl;

        return 1;
    }
}