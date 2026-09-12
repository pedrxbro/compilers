#include "CompilerService.h"

#include "Lexico.h"
#include "Sintatico.h"
#include "Semantico.h"
#include "LexicalError.h"
#include "SyntacticError.h"

AnalysisResult CompilerService::analyze(const std::string &source) const
{
    try
    {
        Lexico lexico(source.c_str());
        Sintatico sintatico;
        Semantico semantico;

        sintatico.parse(&lexico, &semantico);

        return {
            true,
            "Analise sintatica concluida com sucesso.",
            -1
        };
    }
    catch (const LexicalError &error)
    {
        return {
            false,
            std::string("Erro lexico: ") + error.getMessage(),
            error.getPosition()
        };
    }
    catch (const SyntacticError &error)
    {
        return {
            false,
            std::string("Erro sintatico: ") + error.getMessage(),
            error.getPosition()
        };
    }
}