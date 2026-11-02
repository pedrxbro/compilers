
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
    CodegenResult generate(
        const std::string &source
        ) const;
};

}

#endif
