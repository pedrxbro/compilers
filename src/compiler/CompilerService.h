#ifndef COMPILER_SERVICE_H
#define COMPILER_SERVICE_H

#include "AnalysisResult.h"

#include <string>

class CompilerService
{
public:
    // Mantem o comportamento original das analises.
    AnalysisResult analyze(
        const std::string &source
        ) const;

    // Executa as analises e a geracao de Assembly BIP.
    AnalysisResult compile(
        const std::string &source
        ) const;
};

#endif