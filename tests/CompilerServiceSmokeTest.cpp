#include "CompilerService.h"

#include <iostream>
#include <string>

void runTest(
    const std::string &name,
    const std::string &source,
    const CompilerService &compiler)
{
    const AnalysisResult result = compiler.analyze(source);

    std::cout << "========================================\n";
    std::cout << name << '\n';
    std::cout << "success: "
              << std::boolalpha
              << result.success
              << '\n';

    std::cout << "message: "
              << result.message
              << '\n';

    std::cout << "position: "
              << result.position
              << '\n';
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
        compiler);

    runTest(
        "2. Programa com erro lexico",
        lexicalErrorProgram,
        compiler);

    runTest(
        "3. Programa com erro sintatico",
        syntacticErrorProgram,
        compiler);

    return 0;
}