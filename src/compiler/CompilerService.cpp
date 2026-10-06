#include "CompilerService.h"

#include "Lexico.h"
#include "Sintatico.h"
#include "Semantico.h"

#include "LexicalError.h"
#include "SyntacticError.h"
#include "SemanticError.h"

#include "Symbol.h"
#include "SemanticTypes.h"

#include <string>
#include <vector>

namespace
{

std::string dataTypeToString(
    DataType type
    )
{
    switch (type)
    {
    case DataType::Integer:
        return "inteiro";

    case DataType::Float:
        return "flutuante";

    case DataType::Text:
        return "texto";

    case DataType::Logical:
        return "logico";

    case DataType::Character:
        return "caractere";

    case DataType::Void:
        return "void";

    case DataType::Unknown:
        return "desconhecido";
    }

    return "desconhecido";
}

std::string symbolKindToString(
    SymbolKind kind
    )
{
    switch (kind)
    {
    case SymbolKind::Variable:
        return "variavel";

    case SymbolKind::Vector:
        return "vetor";

    case SymbolKind::Parameter:
        return "parametro";

    case SymbolKind::Function:
        return "funcao";
    }

    return "desconhecido";
}

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

void appendSemanticWarnings(
    const Semantico &semantico,
    AnalysisResult &result
    )
{
    const std::vector<SemanticDiagnostic> warnings =
        semantico.warnings();

    for (const SemanticDiagnostic &warning : warnings)
    {
        result.warnings.push_back(
            {
                AnalysisPhase::Semantic,
                warning.message,
                warning.position
            }
            );
    }
}

void appendSymbolTable(
    const Semantico &semantico,
    AnalysisResult &result
    )
{
    const std::vector<const Symbol *> symbols =
        semantico
            .scopeManager()
            .allSymbols();

    for (const Symbol *symbol : symbols)
    {
        if (symbol == nullptr)
        {
            continue;
        }

        SymbolTableEntry entry;

        entry.name =
            symbol->name();

        entry.type =
            dataTypeToString(
                symbol->type()
                );

        entry.kind =
            symbolKindToString(
                symbol->kind()
                );

        entry.scope =
            symbol->scope();

        entry.initialized =
            symbol->isInitialized();

        entry.used =
            symbol->isUsed();

        entry.vectorSize =
            symbol->vectorSize();

        entry.parameterPosition =
            symbol->parameterPosition();

        result.symbolTable.push_back(
            entry
            );
    }
}

std::string buildMessage(
    const AnalysisResult &result
    )
{
    std::string message;

    if (result.errors.empty())
    {
        message =
            "Analise lexica, sintatica e semantica concluida com sucesso.";
    }
    else
    {
        for (const AnalysisDiagnostic &error : result.errors)
        {
            if (!message.empty())
            {
                message += "\n";
            }

            message +=
                "Erro " +
                phaseToString(
                    error.phase
                    ) +
                ": " +
                error.message;
        }
    }

    for (const AnalysisDiagnostic &warning : result.warnings)
    {
        if (!message.empty())
        {
            message += "\n";
        }

        message +=
            "Aviso " +
            phaseToString(
                warning.phase
                ) +
            ": " +
            warning.message;
    }

    return message;
}

}

AnalysisResult CompilerService::analyze(
    const std::string &source
    ) const
{
    AnalysisResult result;

    Semantico semantico;

    try
    {
        Lexico lexico(
            source.c_str()
            );

        Sintatico sintatico;

        sintatico.parse(
            &lexico,
            &semantico
            );
    }
    catch (const LexicalError &error)
    {
        result.errors.push_back(
            {
                AnalysisPhase::Lexical,
                error.getMessage(),
                error.getPosition()
            }
            );
    }
    catch (const SyntacticError &error)
    {
        result.errors.push_back(
            {
                AnalysisPhase::Syntactic,
                error.getMessage(),
                error.getPosition()
            }
            );
    }
    catch (const SemanticError &error)
    {
        result.errors.push_back(
            {
                AnalysisPhase::Semantic,
                error.getMessage(),
                error.getPosition()
            }
            );
    }

    appendSemanticWarnings(
        semantico,
        result
        );

    appendSymbolTable(
        semantico,
        result
        );

    result.success =
        result.errors.empty();

    if (!result.errors.empty())
    {
        result.position =
            result.errors.front().position;
    }

    result.message =
        buildMessage(
            result
            );

    return result;
}