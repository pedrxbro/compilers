#include "CompilerService.h"

#include "Lexico.h"
#include "Sintatico.h"
#include "Semantico.h"

#include "LexicalError.h"
#include "SyntacticError.h"
#include "SemanticError.h"

AnalysisResult CompilerService::analyze(
    const std::string &source
    ) const
{
    try
    {
        Lexico lexico(
            source.c_str()
            );

        Sintatico sintatico;
        Semantico semantico;

        sintatico.parse(
            &lexico,
            &semantico
            );

        return {
            true,
            "Analise sintatica e semantica concluida com sucesso.",
            -1
        };
    }
    catch (const LexicalError &error)
    {
        return {
            false,
            std::string("Erro lexico: ") +
                error.getMessage(),
            error.getPosition()
        };
    }
    catch (const SyntacticError &error)
    {
        return {
            false,
            std::string("Erro sintatico: ") +
                error.getMessage(),
            error.getPosition()
        };
    }
    catch (const SemanticError &error)
    {
        return {
            false,
            std::string("Erro semantico: ") +
                error.getMessage(),
            error.getPosition()
        };
    }
}