#include "CompilerService.h"
#include "BipCodeGenerator.h"

#include <iostream>
#include <stdexcept>
#include <string>
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

    requireContains(result.assembly, ".data", testName);
    requireContains(result.assembly, ".text", testName);
    requireContains(result.assembly, "HLT 0", testName);
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

// ============================================================
// PROGRAMAS DE TESTE
// ============================================================

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

const std::string vectorRegressionSource = R"(programa testeVetores
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

// ============================================================
// TESTES ORIGINAIS
// ============================================================

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
        "valores: 0, 0, 0\n",
        "Declaracao contigua do vetor"
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

void testConditionalControlFlow()
{
    const std::string source = R"(programa controle
inicio
inteiro a;

a recebe 1;

se verdadeiro entao
    a recebe 2;
fimse

se a igual 2 entao
    mostre(a);
senao
    mostre(0);
fimse

fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireSuccess(
        result,
        "Desvio condicional"
        );

    requireContains(
        result.assembly,
        "BEQ",
        "Comparacao por igualdade"
        );

    requireContains(
        result.assembly,
        "JMP",
        "Desvio incondicional"
        );

    requireContains(
        result.assembly,
        "$out_port",
        "Saida condicional"
        );

    std::cout
        << "[OK] Controle condicional suportado\n";
}

void testUnsupportedLoop()
{
    const std::string source = R"(programa repeticao
inicio
inteiro a;
a recebe 0;

enquanto a menorQue 3 faca
    a recebe a mais 1;
fimenquanto

fim)";

    const CompilerService compiler;

    const AnalysisResult result =
        compiler.compile(source);

    requireFailure(
        result,
        AnalysisPhase::CodeGeneration,
        "Laco ainda nao implementado"
        );

    std::cout
        << "[OK] Laco posterior ainda recusado\n";
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

// ============================================================
// NOVOS TESTES DE INTEGRACAO DA M2.2
// ============================================================

void testVectorIndexRegression()
{
    const CompilerService compiler;

    const AnalysisResult analysis =
        compiler.analyze(vectorRegressionSource);

    require(
        analysis.success,
        "Analise com indices calculados falhou: " +
            analysis.message
        );

    require(
        analysis.errors.empty(),
        "Analise de vetores retornou erros."
        );

    require(
        analysis.assembly.empty(),
        "Analise semantica nao deve gerar Assembly."
        );

    const AnalysisResult result =
        compiler.compile(vectorRegressionSource);

    requireSuccess(
        result,
        "Regressao de indices calculados"
        );

    require(
        result.symbolTable.size() == 4,
        "Tabela de simbolos incompleta."
        );

    requireContains(
        result.assembly,
        "v: 0, 0, 0, 0, 0\n"
        "x: 0, 0, 0, 0, 0\n",
        "Declaracoes contiguas dos vetores"
        );

    requireContains(
        result.assembly,
        "    LD i\n"
        "    ADDI 1\n"
        "    STO $indr\n"
        "    LD $in_port\n"
        "    STOV v\n",
        "Entrada em vetor com indice calculado"
        );

    requireContains(
        result.assembly,
        "    LD i\n"
        "    ADDI 1\n"
        "    STO $indr\n"
        "    LDV v\n"
        "    STO $out_port\n",
        "Saida de vetor com indice calculado"
        );

    std::size_t vectorCount = 0;

    for (const auto &symbol : result.symbolTable)
    {
        if (symbol.name == "v" || symbol.name == "x")
        {
            require(
                symbol.kind == "vetor",
                "Modalidade incorreta no vetor."
                );

            require(
                symbol.type == "inteiro",
                "Tipo incorreto no vetor."
                );

            require(
                symbol.vectorSize.has_value() &&
                    symbol.vectorSize.value() == 5,
                "Tamanho do vetor incorreto."
                );

            require(
                symbol.initialized,
                "Vetor deveria estar inicializado."
                );

            require(
                symbol.used,
                "Vetor deveria estar marcado como usado."
                );

            ++vectorCount;
        }
    }

    require(
        vectorCount == 2,
        "Vetores ausentes na tabela de simbolos."
        );

    std::cout
        << "[OK] Regressao de indices calculados na IDE\n";
}

void testValidNestedVectorAnalysis()
{
    const std::string source = R"(programa semanticoVetores
inicio
inteiro v[5], x[5], i, resultado;

i recebe 1;
v[0] recebe 1;
v[1] recebe 10;
x[1] recebe 2;

v[x[v[0]]] recebe v[i] mais 3;

resultado recebe v[x[v[0]]];

leia(v[x[v[0]]]);
mostre(v[x[v[0]]]);
mostre(resultado);
fim)";

    const CompilerService compiler;

    const AnalysisResult analysis =
        compiler.analyze(source);

    require(
        analysis.success,
        "Analise de vetores aninhados falhou: " +
            analysis.message
        );

    require(
        analysis.errors.empty(),
        "Vetores aninhados produziram erro semantico."
        );

    require(
        analysis.assembly.empty(),
        "Analise semantica gerou Assembly indevidamente."
        );

    const AnalysisResult compilation =
        compiler.compile(source);

    requireSuccess(
        compilation,
        "Compilacao de vetores aninhados"
        );

    require(
        compilation.symbolTable.size() == 4,
        "Tabela semantica de vetores aninhados incompleta."
        );

    require(
        compilation.warnings.size() ==
            analysis.warnings.size(),
        "Avisos duplicados em acessos aninhados."
        );

    std::cout
        << "[OK] Analise semantica de vetores aninhados\n";
}

void testSemanticIndexFailures()
{
    const std::vector<std::string> commands = {
        "v[1.5] recebe 10;",
        "v[f] recebe 10;",
        "v[i mais f] recebe 10;",
        "v[verdadeiro] recebe 10;",
        "v[l] recebe 10;",
        "v[i igual 1] recebe 10;",
        "v['a'] recebe 10;",
        "v[\"texto\"] recebe 10;",
        "leia(v[1.5]);",
        "leia(v[l]);",
        "mostre(v[f]);",
        "mostre(v[i igual 1]);",
        "i recebe v[f];"
    };

    const CompilerService compiler;

    for (const auto &command : commands)
    {
        const std::string source =
            "programa indiceInvalido\n"
            "inicio\n"
            "inteiro v[5], i;\n"
            "flutuante f;\n"
            "logico l;\n"
            "i recebe 1;\n"
            "f recebe 1.5;\n"
            "l recebe verdadeiro;\n"
            "v[0] recebe 10;\n" +
            command +
            "\nfim";

        const AnalysisResult analysis =
            compiler.analyze(source);

        requireFailure(
            analysis,
            AnalysisPhase::Semantic,
            "Validacao semantica: " + command
            );

        requireContains(
            analysis.errors.front().message,
            "deve ser INTEIRO",
            "Diagnostico de tipo do indice"
            );

        const AnalysisResult compilation =
            compiler.compile(source);

        requireFailure(
            compilation,
            AnalysisPhase::Semantic,
            "Compilacao de indice invalido: " + command
            );

        requireContains(
            compilation.errors.front().message,
            "deve ser INTEIRO",
            "Indice invalido bloqueia Assembly"
            );
    }

    std::cout
        << "[OK] Tipos invalidos nos indices recusados\n";
}

void testNestedInvalidIndexType()
{
    const std::string source = R"(programa indiceAninhadoInvalido
inicio
inteiro v[5];
flutuante x[5];

x[0] recebe 1.5;
v[x[0]] recebe 20;
fim)";

    const CompilerService compiler;

    const AnalysisResult analysis =
        compiler.analyze(source);

    requireFailure(
        analysis,
        AnalysisPhase::Semantic,
        "Indice aninhado nao inteiro"
        );

    requireContains(
        analysis.errors.front().message,
        "Indice do vetor 'v' deve ser INTEIRO",
        "Validacao do vetor externo"
        );

    const AnalysisResult compilation =
        compiler.compile(source);

    requireFailure(
        compilation,
        AnalysisPhase::Semantic,
        "Indice aninhado invalido bloqueia Assembly"
        );

    std::cout
        << "[OK] Tipo do indice aninhado validado\n";
}

void testInvalidVectorReferences()
{
    const std::vector<std::string> commands = {
        "i[0] recebe 10;",
        "leia(i[0]);",
        "mostre(i[0]);",
        "v recebe 10;",
        "leia(v);",
        "mostre(v);",
        "i recebe v;"
    };

    const CompilerService compiler;

    for (const auto &command : commands)
    {
        const std::string source =
            "programa referenciaInvalida\n"
            "inicio\n"
            "inteiro i, v[3];\n"
            "i recebe 1;\n"
            "v[0] recebe 10;\n" +
            command +
            "\nfim";

        const AnalysisResult result =
            compiler.compile(source);

        requireFailure(
            result,
            AnalysisPhase::Semantic,
            "Referencia invalida: " + command
            );
    }

    std::cout
        << "[OK] Referencias invalidas de vetores recusadas\n";
}

void testLiteralIndexBoundsIntegration()
{
    const std::vector<std::string> commands = {
        "v[3] recebe 10;",
        "v[menos 1] recebe 10;",
        "leia(v[3]);",
        "leia(v[menos 1]);",
        "mostre(v[3]);",
        "mostre(v[menos 1]);",
        "i recebe v[3];",
        "i recebe v[menos 1];"
    };

    const CompilerService compiler;

    for (const auto &command : commands)
    {
        const std::string source =
            "programa limites\n"
            "inicio\n"
            "inteiro i, v[3];\n"
            "i recebe 0;\n"
            "v[0] recebe 10;\n" +
            command +
            "\nfim";

        const AnalysisResult analysis =
            compiler.analyze(source);

        require(
            analysis.success,
            "Indice inteiro deveria passar pela "
            "validacao de tipos: " + command
            );

        const AnalysisResult compilation =
            compiler.compile(source);

        requireFailure(
            compilation,
            AnalysisPhase::CodeGeneration,
            "Limite de vetor: " + command
            );

        requireContains(
            compilation.errors.front().message,
            "Indice literal fora dos limites",
            "Diagnostico de limite do vetor"
            );
    }

    std::cout
        << "[OK] Limites literais validados no gerador\n";
}

void testVectorAssemblyNotReused()
{
    const CompilerService compiler;

    const AnalysisResult first =
        compiler.compile(vectorRegressionSource);

    requireSuccess(
        first,
        "Primeira compilacao de vetores"
        );

    const std::string invalidSource = R"(programa erroVetor
inicio
inteiro v[3];
v[menos 1] recebe 10;
fim)";

    const AnalysisResult second =
        compiler.compile(invalidSource);

    requireFailure(
        second,
        AnalysisPhase::CodeGeneration,
        "Compilacao com indice invalido"
        );

    require(
        second.assembly.empty(),
        "Assembly anterior foi mantido indevidamente."
        );

    const AnalysisResult third =
        compiler.compile(vectorRegressionSource);

    requireSuccess(
        third,
        "Nova compilacao valida de vetores"
        );

    require(
        third.assembly == first.assembly,
        "Compilacao de vetores nao deterministica."
        );

    std::cout
        << "[OK] Assembly de vetores nao reutilizado\n";
}

}

int main()
{
    try
    {
        // Testes originais.
        testOriginalAnalysisPreserved();
        testBasicAssemblyIntegration();
        testVectorBitwiseAndIo();

        testLexicalFailure();
        testSyntacticFailure();
        testSemanticFailure();

        testUnsupportedOperation();
        testUnsupportedDataType();
        testConditionalControlFlow();
        testUnsupportedLoop();

        testWarningsNotDuplicated();
        testPreviousAssemblyNotReused();
        testGeneratorOriginalApi();

        // Regressoes da M2.2.
        testVectorIndexRegression();
        testValidNestedVectorAnalysis();
        testSemanticIndexFailures();
        testNestedInvalidIndexType();
        testInvalidVectorReferences();
        testLiteralIndexBoundsIntegration();
        testVectorAssemblyNotReused();

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