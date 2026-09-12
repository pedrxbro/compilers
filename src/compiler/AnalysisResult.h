#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include <string>

struct AnalysisResult
{
    bool success = false;
    std::string message;
    int position = -1;
};

#endif