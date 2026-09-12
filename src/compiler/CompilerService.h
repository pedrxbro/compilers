#ifndef COMPILER_SERVICE_H
#define COMPILER_SERVICE_H

#include "AnalysisResult.h"

#include <string>

class CompilerService
{
public:
    AnalysisResult analyze(const std::string &source) const;
};

#endif