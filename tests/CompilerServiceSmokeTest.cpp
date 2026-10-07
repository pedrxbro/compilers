#include "CompilerService.h"

#include <iostream>
#include <string>

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

void printDiagnostics(
    const std::string &label,
    const std::vector<AnalysisDiagnostic> &diagnostics
    )
{
    std::cout
        << label
        << ": "
        << diagnostics.size()
        << '\n';

    for (const AnalysisDiagnostic &diagnostic : diagnostics)
    {
        std::cout
            << "  ["
            << phaseToString(diagnostic.phase)
            << "] "
            << diagnostic.message;

        if (diagnostic.position >= 0)
        {
            std::cout
                << " (posicao "
                << diagnostic.position
                << ")";
        }

        std::cout << '\n';
    }
}

void printSymbolTable(
    const AnalysisResult &result
    )
{
    std::cout
        << "symbolTable: "
        << result.symbolTable.size()
        << '\n';

    for (const SymbolTableEntry &symbol : result.symbolTable)
    {
        std::cout
            << "  nome="
            << symbol.name
            << ", tipo="
            << symbol.type
            << ", modalidade="
            << symbol.kind
            << ", escopo="
            << symbol.scope
            << ", inicializado="
            << std::boolalpha
            << symbol.initialized
            << ", usado="
            << std::boolalpha
            << symbol.used;

        if (symbol.vectorSize.has_value())
        {
            std::cout
                << ", tamanhoVetor="
                << symbol.vectorSize.value();
        }

        if (symbol.parameterPosition.has_value())
        {
            std::cout
                << ", posicaoParametro="
                << symbol.parameterPosition.value();
        }

        std::cout << '\n';
    }
}

void runTest(
    const std::string &name,
    const std::string &source,
    const CompilerService &compiler
    )
{
    const AnalysisResult result =
        compiler.analyze(
            source
            );

    std::cout
        << "========================================\n";

    std::cout
        << name
        << '\n';

    std::cout
        << "success: "
        << std::boolalpha
        << result.success
        << '\n';

    std::cout
        << "message:\n"
        << result.message
        << '\n';

    std::cout
        << "legacy position: "
        << result.position
        << '\n';

    printDiagnostics(
        "errors",
        result.errors
        );

    printDiagnostics(
        "warnings",
        result.warnings
        );

    printSymbolTable(
        result
        );
}

int main()
{
    CompilerService compiler;

    const std::string validProgram = R"(programa teste
inicio
inteiro x;
x recebe 10;
mostre(x);
fim)";

    const std::string warningProgram = R"(programa teste
inicio
inteiro x, y;
mostre(y);
fim)";

    const std::string semanticErrorProgram = R"(programa teste
inicio
inteiro x;
inteiro x;
fim)";

    const std::string lexicalErrorProgram = R"(programa teste
inicio
inteiro x;
x recebe @;
fim)";

    const std::string syntacticErrorProgram = R"(programa teste
inicio
inteiro x
fim)";

    runTest(
        "1. Programa valido",
        validProgram,
        compiler
        );

    runTest(
        "2. Programa com multiplos avisos semanticos",
        warningProgram,
        compiler
        );

    runTest(
        "3. Programa com erro semantico",
        semanticErrorProgram,
        compiler
        );

    runTest(
        "4. Programa com erro lexico",
        lexicalErrorProgram,
        compiler
        );

    runTest(
        "5. Programa com erro sintatico",
        syntacticErrorProgram,
        compiler
        );

    return 0;
}