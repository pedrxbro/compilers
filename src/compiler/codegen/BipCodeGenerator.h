#ifndef BIP_CODE_GENERATOR_H
#define BIP_CODE_GENERATOR_H

#include "AnalysisResult.h"

#include <string>
#include <vector>

namespace bip
{

struct CodegenResult
{
    bool success = false;

    std::string assembly;
    std::string message;

    int position = -1;

    std::vector<AnalysisDiagnostic> warnings;
};

class BipCodeGenerator
{
public:
    // API original: realiza a analise antes de gerar.
    CodegenResult generate(
        const std::string &source
        ) const;

    // Nova API: utiliza a analise ja realizada.
    CodegenResult generate(
        const std::string &source,
        const AnalysisResult &analysis
        ) const;
};

}

#endif