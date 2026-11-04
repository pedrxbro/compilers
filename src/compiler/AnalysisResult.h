#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

enum class AnalysisPhase
{
    Lexical,
    Syntactic,
    Semantic,
    CodeGeneration
};

struct AnalysisDiagnostic
{
    AnalysisPhase phase = AnalysisPhase::Semantic;
    std::string message;
    int position = -1;
};

struct SymbolTableEntry
{
    std::string name;
    std::string type;
    std::string kind;
    std::string scope;

    bool initialized = false;
    bool used = false;

    std::optional<std::size_t> vectorSize;
    std::optional<std::size_t> parameterPosition;
};

struct AnalysisResult
{
    bool success = false;

    std::string message;
    int position = -1;

    // Estrutura preparada para transportar varios diagnosticos.
    std::vector<AnalysisDiagnostic> errors;
    std::vector<AnalysisDiagnostic> warnings;

    // Snapshot da tabela de simbolos produzido pela analise.
    std::vector<SymbolTableEntry> symbolTable;

    // Assembly completo gerado para um programa valido.
    // Deve permanecer vazio quando a compilacao falhar.
    std::string assembly;
};

#endif