#include "CompilerService.h"
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
        testName + ": conteudo esperado ausente: " + expected
        );
}

void requireSuccess(
    const AnalysisResult &result,
    const std::string &testName
    )
{
    require(
        result.success,
        testName + ": compilacao falhou: " + result.message
        );

    require(
        result.errors.empty(),
        testName + ": compilacao possui erros."
        );

    require(
        !result.assembly.empty(),
        testName + ": Assembly vazio."
        );

    requireContains(
        result.assembly,
        ".data",
        testName
        );

    requireContains(
        result.assembly,
        ".text",
        testName
        );

    requireContains(
        result.assembly,
        "HLT 0",
        testName
        );
}

void requireFailure(
    const AnalysisResult &result,
    AnalysisPhase expectedPhase,
    const std::string &testName
    )
{
    require(
        !result.success,
        testName + ": compilacao deveria falhar."
        );

    require(
        result.assembly.empty(),
        testName + ": Assembly parcial foi retornado."
        );

    require(
        !result.errors.empty(),
        testName + ": diagnostico ausente."
        );

    require(
        result.errors.front().phase == expectedPhase,
        testName + ": fase incorreta do diagnostico."
        );

    require(
        !result.message.empty(),
        testName + ": mensagem de erro vazia."
        );
}

const std::string basicSource = R"(programa calculo
inicio
inteiro a, b, resultado;

a recebe 10;
b recebe 5;
resultado recebe a mais b;
resultado recebe resultado menos 2;
fim)";

const std::string invalidSemanticSource = R"(programa invalido
inicio
inteiro a;
a recebe b mais 1;
fim)";

void testOriginalAnalysisPreserved()
{
    const CompilerService compiler;

    const AnalysisResult result =
        compiler.analyze(basicSource);

    require(
        result.success,
        "Analise original deixou de funcionar."
        );

    require(
        result.errors.empty(),
        "Analise original retornou erros."
        );

    require(
        result.assembly.empty(),
        "analyze nao deve executar geracao BIP."
        );

    require(
        result.symbolTable.size() == 3,
        "Tabela de simbolos incompleta."
        );

    std::cout
        << "[OK] Analise original preservada\n";
}

void testBasicAssemblyIntegration()
{
    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(basicSource);

    requireSuccess(
        result,
        "Integracao basica"
        );

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

    require(
        result.assembly == expected,
        "Assembly basico diferente do esperado."
        );

    require(
        result.symbolTable.size() == 3,
        "Tabela de simbolos nao foi preservada."
        );

    requireContains(
        result.message,
        "Assembly BIP gerado com sucesso.",
        "Mensagem de sucesso"
        );

    std::cout
        << "[OK] Geracao integrada de Assembly\n";
}

void testVectorBitwiseAndIo()
{
    const std::string source = R"(programa vetores
inicio
inteiro a, resultado, valores[3];

a recebe 1;
valores[0] recebe 6;
valores[a] recebe valores[0] mais 2;

resultado recebe valores[a] eBitBit 3;
resultado recebe resultado ouExclusivoBitBit 1;

leia(valores[2]);
mostre(resultado);
mostre(valores[2]);
fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireSuccess(
        result,
        "Vetores, bits e entrada/saida"
        );

    requireContains(
        result.assembly,
        "valores: 0",
        "Declaracao do vetor"
        );

    requireContains(
        result.assembly,
        "LDV valores",
        "Leitura do vetor"
        );

    requireContains(
        result.assembly,
        "STOV valores",
        "Escrita no vetor"
        );

    requireContains(
        result.assembly,
        "ANDI 3",
        "Operacao AND"
        );

    requireContains(
        result.assembly,
        "XORI 1",
        "Operacao XOR"
        );

    requireContains(
        result.assembly,
        "$in_port",
        "Entrada de dados"
        );

    requireContains(
        result.assembly,
        "$out_port",
        "Saida de dados"
        );

    require(
        result.symbolTable.size() == 3,
        "Tabela de simbolos de vetores incompleta."
        );

    std::cout
        << "[OK] Vetores, bitwise e entrada/saida\n";
}

void testLexicalFailure()
{
    const std::string source = R"(programa lexical
inicio
inteiro a;
a recebe 1;
@
fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireFailure(
        result,
        AnalysisPhase::Lexical,
        "Erro lexico"
        );

    std::cout
        << "[OK] Erro lexico impede geracao\n";
}

void testSyntacticFailure()
{
    const std::string source = R"(programa sintaxe
inicio
inteiro a;
a recebe ;
fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireFailure(
        result,
        AnalysisPhase::Syntactic,
        "Erro sintatico"
        );

    std::cout
        << "[OK] Erro sintatico impede geracao\n";
}

void testSemanticFailure()
{
    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(invalidSemanticSource);

    requireFailure(
        result,
        AnalysisPhase::Semantic,
        "Erro semantico"
        );

    requireContains(
        result.message,
        "b",
        "Diagnostico semantico"
        );

    std::cout
        << "[OK] Erro semantico impede geracao\n";
}

void testUnsupportedOperation()
{
    const std::string source = R"(programa operacao
inicio
inteiro a, b, resultado;

a recebe 2;
b recebe 3;
resultado recebe a vezes b;
fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireFailure(
        result,
        AnalysisPhase::CodeGeneration,
        "Operacao nao suportada"
        );

    requireContains(
        result.message,
        "geracao BIP",
        "Diagnostico do gerador"
        );

    std::cout
        << "[OK] Operacao nao suportada recusada\n";
}

void testUnsupportedDataType()
{
    const std::string source = R"(programa tipos
inicio
flutuante valor;
fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireFailure(
        result,
        AnalysisPhase::CodeGeneration,
        "Tipo nao suportado"
        );

    std::cout
        << "[OK] Tipo nao suportado recusado\n";
}

void testUnsupportedControlFlow()
{
    const std::string source = R"(programa controle
inicio
inteiro a;

se verdadeiro entao
a recebe 1;
fimse

fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireFailure(
        result,
        AnalysisPhase::CodeGeneration,
        "Controle de fluxo nao suportado"
        );

    std::cout
        << "[OK] Controle de fluxo sem suporte recusado\n";
}

void testWarningsNotDuplicated()
{
    const std::string source = R"(programa avisos
inicio
inteiro a, b;
a recebe 1;
fim)";

    const CompilerService compiler;

    const AnalysisResult analysis =
        compiler.analyze(source);

    const AnalysisResult compilation =
        compiler.compile(source);

    requireSuccess(
        compilation,
        "Compilacao com avisos"
        );

    require(
        !analysis.warnings.empty(),
        "O programa deveria produzir avisos semanticos."
        );

    require(
        compilation.warnings.size() ==
            analysis.warnings.size(),
        "Avisos semanticos foram duplicados."
        );

    std::cout
        << "[OK] Avisos preservados sem duplicacao\n";
}

void testPreviousAssemblyNotReused()
{
    const CompilerService compiler;

    const AnalysisResult first =
        compiler.compile(basicSource);

    requireSuccess(
        first,
        "Primeira compilacao"
        );

    const AnalysisResult second =
        compiler.compile(invalidSemanticSource);

    requireFailure(
        second,
        AnalysisPhase::Semantic,
        "Segunda compilacao"
        );

    require(
        second.assembly.empty(),
        "Assembly anterior foi reutilizado."
        );

    const AnalysisResult third =
        compiler.compile(basicSource);

    requireSuccess(
        third,
        "Terceira compilacao"
        );

    require(
        third.assembly == first.assembly,
        "Compilacao posterior nao reproduziu o resultado."
        );

    std::cout
        << "[OK] Assembly anterior nao reutilizado\n";
}

void testGeneratorOriginalApi()
{
    const bip::BipCodeGenerator generator;

    const bip::CodegenResult result =
        generator.generate(basicSource);

    require(
        result.success,
        "API original do gerador deixou de funcionar."
        );

    require(
        !result.assembly.empty(),
        "API original retornou Assembly vazio."
        );

    std::cout
        << "[OK] API original do gerador preservada\n";
}

}

int main()
{
    try
    {
        testOriginalAnalysisPreserved();
        testBasicAssemblyIntegration();
        testVectorBitwiseAndIo();

        testLexicalFailure();
        testSyntacticFailure();
        testSemanticFailure();

        testUnsupportedOperation();
        testUnsupportedDataType();
        testUnsupportedControlFlow();

        testWarningsNotDuplicated();
        testPreviousAssemblyNotReused();
        testGeneratorOriginalApi();

        std::cout
            << "Todos os testes de integracao da IDE BIP passaram.\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Falha na integracao da IDE BIP: "
            << error.what()
            << '\n';

        return 1;
    }
}