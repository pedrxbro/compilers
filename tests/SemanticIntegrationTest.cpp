#include "CompilerService.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

    namespace
{

    struct ExpectedDiagnostic
    {
        AnalysisPhase phase;
        std::string text;
    };

    struct TestCase
    {
        std::string name;
        std::string source;

        bool expectedSuccess;

        std::vector<ExpectedDiagnostic> expectedErrors;
        std::vector<ExpectedDiagnostic> expectedWarnings;

        std::optional<std::size_t> exactErrorCount;
        std::optional<std::size_t> exactWarningCount;

        std::function<void(const AnalysisResult &)> verifyResult;
    };

    std::string phaseToString(
        AnalysisPhase phase
        )
    {
        switch (phase)
        {
        case AnalysisPhase::Lexical:
            return "lexico";

        case AnalysisPhase::Syntactic:
            return "sintatico";

        case AnalysisPhase::Semantic:
            return "semantico";
        }

        return "desconhecido";
    }

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

    bool containsDiagnostic(
        const std::vector<AnalysisDiagnostic> &diagnostics,
        const ExpectedDiagnostic &expected
        )
    {
        return std::any_of(
            diagnostics.begin(),
            diagnostics.end(),
            [&expected](const AnalysisDiagnostic &diagnostic)
            {
                return
                    diagnostic.phase == expected.phase &&
                    diagnostic.message.find(expected.text) !=
                        std::string::npos;
            }
            );
    }

    const SymbolTableEntry &requireSymbol(
        const AnalysisResult &result,
        const std::string &name,
        const std::string &scope
        )
    {
        const auto iterator =
            std::find_if(
                result.symbolTable.begin(),
                result.symbolTable.end(),
                [&name, &scope](const SymbolTableEntry &entry)
                {
                    return
                        entry.name == name &&
                        entry.scope == scope;
                }
                );

        require(
            iterator != result.symbolTable.end(),
            "Simbolo '" +
                name +
                "' nao encontrado no escopo '" +
                scope +
                "'."
            );

        return *iterator;
    }

    std::vector<const SymbolTableEntry *> findSymbolsByName(
        const AnalysisResult &result,
        const std::string &name
        )
    {
        std::vector<const SymbolTableEntry *> matches;

        for (const SymbolTableEntry &entry : result.symbolTable)
        {
            if (entry.name == name)
            {
                matches.push_back(
                    &entry
                    );
            }
        }

        return matches;
    }

    void requireBasicSymbolState(
        const SymbolTableEntry &symbol,
        const std::string &expectedType,
        const std::string &expectedKind,
        bool expectedInitialized,
        bool expectedUsed
        )
    {
        require(
            symbol.type == expectedType,
            "Tipo inesperado para '" +
                symbol.name +
                "': esperado '" +
                expectedType +
                "', obtido '" +
                symbol.type +
                "'."
            );

        require(
            symbol.kind == expectedKind,
            "Modalidade inesperada para '" +
                symbol.name +
                "': esperado '" +
                expectedKind +
                "', obtido '" +
                symbol.kind +
                "'."
            );

        require(
            symbol.initialized == expectedInitialized,
            "Estado de inicializacao inesperado para '" +
                symbol.name +
                "'."
            );

        require(
            symbol.used == expectedUsed,
            "Estado de uso inesperado para '" +
                symbol.name +
                "'."
            );
    }

    void printExpectedDiagnostics(
        const std::string &label,
        const std::vector<ExpectedDiagnostic> &diagnostics
        )
    {
        std::cout
            << label
            << ": ";

        if (diagnostics.empty())
        {
            std::cout
                << "nenhum"
                << '\n';

            return;
        }

        std::cout << '\n';

        for (const ExpectedDiagnostic &diagnostic : diagnostics)
        {
            std::cout
                << "  ["
                << phaseToString(
                       diagnostic.phase
                       )
                << "] contem: "
                << diagnostic.text
                << '\n';
        }
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
            << "\n----------------------------------------\n"
            << "Resultado esperado: "
            << (
                   test.expectedSuccess
                       ? "SUCESSO"
                       : "FALHA"
                   )
            << '\n';

        printExpectedDiagnostics(
            "Erros esperados",
            test.expectedErrors
            );

        printExpectedDiagnostics(
            "Avisos esperados",
            test.expectedWarnings
            );

        const AnalysisResult result =
            compiler.analyze(
                test.source
                );

        require(
            result.success ==
                test.expectedSuccess,
            test.name +
                ": resultado de sucesso/falha diferente do esperado. Mensagem: " +
                result.message
            );

        if (test.exactErrorCount.has_value())
        {
            require(
                result.errors.size() ==
                    test.exactErrorCount.value(),
                test.name +
                    ": quantidade de erros diferente do esperado."
                );
        }

        if (test.exactWarningCount.has_value())
        {
            require(
                result.warnings.size() ==
                    test.exactWarningCount.value(),
                test.name +
                    ": quantidade de avisos diferente do esperado."
                );
        }

        for (const ExpectedDiagnostic &expected :
             test.expectedErrors)
        {
            require(
                containsDiagnostic(
                    result.errors,
                    expected
                    ),
                test.name +
                    ": erro esperado nao encontrado: " +
                    expected.text
                );
        }

        for (const ExpectedDiagnostic &expected :
             test.expectedWarnings)
        {
            require(
                containsDiagnostic(
                    result.warnings,
                    expected
                    ),
                test.name +
                    ": aviso esperado nao encontrado: " +
                    expected.text
                );
        }

        if (test.verifyResult)
        {
            test.verifyResult(
                result
                );
        }

        std::cout
            << "Resultado obtido: "
            << (
                   result.success
                       ? "SUCESSO"
                       : "FALHA"
                   )
            << "\n[OK] "
            << test.name
            << '\n';
    }

    std::vector<TestCase> buildTests()
    {
        return {

            // =====================================================
            // 01 - DECLARACAO VALIDA
            // =====================================================

            {
                "01 - Declaracao valida",

                R"(programa declaracaoValida
inicio
inteiro x;
x recebe 10;
mostre(x);
fim)",

                true,

                {},

                {},

                0,

                0,

                [](const AnalysisResult &result)
                {
                    const SymbolTableEntry &x =
                        requireSymbol(
                            result,
                            "x",
                            "global"
                            );

                    requireBasicSymbolState(
                        x,
                        "inteiro",
                        "variavel",
                        true,
                        true
                        );
                }
            },

                // =====================================================
                // 02 - VARIAVEL NAO DECLARADA
                // =====================================================

                {
                    "02 - Variavel nao declarada",

                    R"(programa variavelNaoDeclarada
inicio
x recebe 10;
fim)",

                    false,

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Identificador 'x' nao declarado"
                        }
                    },

                    {},

                    1,

                    0,

                    {}
                },

                // =====================================================
                // 03 - DUPLICIDADE
                // =====================================================

                {
                    "03 - Duplicidade no mesmo escopo",

                    R"(programa duplicidade
inicio
inteiro x;
x recebe 1;
mostre(x);
inteiro x;
fim)",

                    false,

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Identificador 'x' ja declarado no escopo 'global'"
                        }
                    },

                    {},

                    1,

                    0,

                    {}
                },

                // =====================================================
                // 04 - ESCOPOS
                // =====================================================

                {
                    "04 - Escopos com sombreamento valido",

                    R"(programa escopos
inicio
inteiro x;
x recebe 1;

se verdadeiro entao
    inteiro x;
    x recebe 2;
    mostre(x);
fimse

mostre(x);
fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    [](const AnalysisResult &result)
                    {
                        const std::vector<const SymbolTableEntry *> symbols =
                            findSymbolsByName(
                                result,
                                "x"
                                );

                        require(
                            symbols.size() == 2,
                            "O teste de escopos deveria produzir dois simbolos chamados 'x'."
                            );

                        bool foundGlobal = false;
                        bool foundInternal = false;

                        for (const SymbolTableEntry *symbol : symbols)
                        {
                            require(
                                symbol != nullptr,
                                "Referencia de simbolo nula no teste de escopos."
                                );

                            requireBasicSymbolState(
                                *symbol,
                                "inteiro",
                                "variavel",
                                true,
                                true
                                );

                            if (symbol->scope == "global")
                            {
                                foundGlobal = true;
                            }
                            else if (
                                symbol->scope.find(
                                    "global::"
                                    ) == 0
                                )
                            {
                                foundInternal = true;
                            }
                        }

                        require(
                            foundGlobal,
                            "Variavel global 'x' nao encontrada."
                            );

                        require(
                            foundInternal,
                            "Variavel 'x' do escopo interno nao encontrada."
                            );
                    }
                },

                // =====================================================
                // 05 - VAZAMENTO DE ESCOPO
                // =====================================================

                {
                    "05 - Vazamento de escopo proibido",

                    R"(programa vazamentoEscopo
inicio

se verdadeiro entao
    inteiro local;
    local recebe 1;
    mostre(local);
fimse

mostre(local);

fim)",

                    false,

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Identificador 'local' nao declarado em um escopo visivel a partir de 'global'"
                        }
                    },

                    {},

                    1,

                    0,

                    {}
                },

                // =====================================================
                // 06 - VARIAVEL INICIALIZADA
                // =====================================================

                {
                    "06 - Variavel inicializada",

                    R"(programa inicializada
inicio
inteiro x;
leia(x);
mostre(x);
fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    [](const AnalysisResult &result)
                    {
                        const SymbolTableEntry &x =
                            requireSymbol(
                                result,
                                "x",
                                "global"
                                );

                        requireBasicSymbolState(
                            x,
                            "inteiro",
                            "variavel",
                            true,
                            true
                            );
                    }
                },

                // =====================================================
                // 07 - VARIAVEL NAO INICIALIZADA
                // =====================================================

                {
                    "07 - Variavel usada sem inicializacao",

                    R"(programa naoInicializada
inicio
inteiro x;
mostre(x);
fim)",

                    true,

                    {},

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Identificador 'x' usado antes da inicializacao"
                        }
                    },

                    0,

                    1,

                    [](const AnalysisResult &result)
                    {
                        const SymbolTableEntry &x =
                            requireSymbol(
                                result,
                                "x",
                                "global"
                                );

                        requireBasicSymbolState(
                            x,
                            "inteiro",
                            "variavel",
                            false,
                            true
                            );
                    }
                },

                // =====================================================
                // 08 - VARIAVEL UTILIZADA
                // =====================================================

                {
                    "08 - Variavel utilizada",

                    R"(programa utilizada
inicio
inteiro origem, destino;

origem recebe 5;
destino recebe origem mais 1;

mostre(destino);

fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    [](const AnalysisResult &result)
                    {
                        const SymbolTableEntry &origem =
                            requireSymbol(
                                result,
                                "origem",
                                "global"
                                );

                        const SymbolTableEntry &destino =
                            requireSymbol(
                                result,
                                "destino",
                                "global"
                                );

                        requireBasicSymbolState(
                            origem,
                            "inteiro",
                            "variavel",
                            true,
                            true
                            );

                        requireBasicSymbolState(
                            destino,
                            "inteiro",
                            "variavel",
                            true,
                            true
                            );
                    }
                },

                // =====================================================
                // 09 - VARIAVEL NAO UTILIZADA
                // =====================================================

                {
                    "09 - Variavel declarada e nao utilizada",

                    R"(programa naoUtilizada
inicio
inteiro x;
x recebe 10;
fim)",

                    true,

                    {},

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Identificador 'x' declarado no escopo 'global' e nunca usado"
                        }
                    },

                    0,

                    1,

                    [](const AnalysisResult &result)
                    {
                        const SymbolTableEntry &x =
                            requireSymbol(
                                result,
                                "x",
                                "global"
                                );

                        requireBasicSymbolState(
                            x,
                            "inteiro",
                            "variavel",
                            true,
                            false
                            );
                    }
                },

                // =====================================================
                // 10 - VETORES
                // =====================================================

                {
                    "10 - Vetores",

                    R"(programa vetores
inicio
inteiro valores[3];

valores[0] recebe 10;

mostre(valores[0]);

fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    [](const AnalysisResult &result)
                    {
                        const SymbolTableEntry &valores =
                            requireSymbol(
                                result,
                                "valores",
                                "global"
                                );

                        requireBasicSymbolState(
                            valores,
                            "inteiro",
                            "vetor",
                            true,
                            true
                            );

                        require(
                            valores.vectorSize.has_value(),
                            "O vetor 'valores' deveria possuir tamanho registrado."
                            );

                        require(
                            valores.vectorSize.value() == 3,
                            "O vetor 'valores' deveria possuir tamanho 3."
                            );
                    }
                },

                // =====================================================
                // 11 - PARAMETROS
                // =====================================================

                {
                    "11 - Parametros",

                    R"(programa parametros

procedimento exibe(inteiro valor)
inicio
leia(valor);
mostre(valor);
fimprocedimento

inicio

exibe(10);

fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    [](const AnalysisResult &result)
                    {
                        const SymbolTableEntry &subrotina =
                            requireSymbol(
                                result,
                                "exibe",
                                "global"
                                );

                        requireBasicSymbolState(
                            subrotina,
                            "void",
                            "funcao",
                            false,
                            true
                            );

                        const SymbolTableEntry &valor =
                            requireSymbol(
                                result,
                                "valor",
                                "global::exibe"
                                );

                        requireBasicSymbolState(
                            valor,
                            "inteiro",
                            "parametro",
                            true,
                            true
                            );

                        require(
                            valor.parameterPosition.has_value(),
                            "O parametro 'valor' deveria possuir posicao registrada."
                            );

                        require(
                            valor.parameterPosition.value() == 1,
                            "O parametro 'valor' deveria ocupar a posicao 1."
                            );
                    }
                },

                // =====================================================
                // 12 - FUNCOES
                // =====================================================

                {
                    "12 - Funcoes",

                    R"(programa funcoes

funcao inteiro resposta()
inicio
retorne 42;
fimfuncao

inicio

inteiro x;

x recebe resposta();

mostre(x);

fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    [](const AnalysisResult &result)
                    {
                        const SymbolTableEntry &funcao =
                            requireSymbol(
                                result,
                                "resposta",
                                "global"
                                );

                        requireBasicSymbolState(
                            funcao,
                            "inteiro",
                            "funcao",
                            false,
                            true
                            );

                        const SymbolTableEntry &x =
                            requireSymbol(
                                result,
                                "x",
                                "global"
                                );

                        requireBasicSymbolState(
                            x,
                            "inteiro",
                            "variavel",
                            true,
                            true
                            );
                    }
                },

                // =====================================================
                // 13 - EXPRESSAO VALIDA
                // =====================================================

                {
                    "13 - Expressoes validas",

                    R"(programa expressaoValida
inicio

flutuante resultado;

resultado recebe
    (1 mais 2)
    vezes
    (3 divido 2);

mostre(resultado);

fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    {}
                },

                // =====================================================
                // 14 - EXPRESSAO INCOMPATIVEL
                // =====================================================

                {
                    "14 - Expressoes incompativeis",

                    R"(programa expressaoIncompativel
inicio

texto a, b;

a recebe "primeiro";
b recebe "segundo";

a recebe a vezes b;

fim)",

                    false,

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Operacao 'vezes' incompativel entre TEXTO e TEXTO"
                        }
                    },

                    {},

                    1,

                    0,

                    {}
                },

                // =====================================================
                // 15 - ATRIBUICAO VALIDA
                // =====================================================

                {
                    "15 - Atribuicoes validas",

                    R"(programa atribuicaoValida
inicio

inteiro i;
flutuante f;

i recebe 10;

f recebe i;

mostre(f);

fim)",

                    true,

                    {},

                    {},

                    0,

                    0,

                    {}
                },

                // =====================================================
                // 16 - ATRIBUICAO COM AVISO
                // =====================================================

                {
                    "16 - Atribuicao com aviso",

                    R"(programa atribuicaoAviso
inicio

inteiro i;
flutuante f;

f recebe 10.5;

i recebe f;

mostre(i);

fim)",

                    true,

                    {},

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Atribuicao com conversao de FLUTUANTE para INTEIRO"
                        }
                    },

                    0,

                    1,

                    {}
                },

            // =====================================================
            // 17 - ATRIBUICAO INVALIDA
            // =====================================================

            {
                "17 - Atribuicao invalida",

                    R"(programa atribuicaoInvalida
inicio

logico l;

l recebe verdadeiro;

mostre(l);

l recebe 10;

fim)",

                    false,

                    {
                        {
                            AnalysisPhase::Semantic,
                            "Atribuicao incompativel: identificador 'l' e do tipo LOGICO, mas a expressao resulta em INTEIRO"
                        }
                    },

                    {},

                    1,

                    0,

                {}
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
            << "Todos os testes semanticos da M2.1 passaram.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha nos testes semanticos da M2.1: "
            << error.what()
            << std::endl;

        return 1;
    }
}