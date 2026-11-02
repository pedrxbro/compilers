
#include "BipCodeGenerator.h"

#include "BipIr.h"
#include "CompilerService.h"
#include "LexicalError.h"

#include <exception>
#include <string>

namespace bip
{

CodegenResult BipCodeGenerator::generate(
    const std::string &source
    ) const
{
    CodegenResult result;

    const CompilerService compiler;

    const AnalysisResult analysis =
        compiler.analyze(source);

    result.warnings = analysis.warnings;

    if (!analysis.success)
    {
        result.success = false;

        result.message =
            "Geracao BIP cancelada: " +
            analysis.message;

        result.position = analysis.position;
        result.assembly.clear();

        return result;
    }

    try
    {
        const BipIrBuilder builder;

        const Program program =
            builder.build(source, analysis);

        const BipEmitter emitter;

        const std::string assembly =
            emitter.emit(program);

        result.success = true;
        result.assembly = assembly;

        result.message =
            "Assembly BIP gerado com sucesso.";

        return result;
    }
    catch (const CodegenError &error)
    {
        result.success = false;
        result.assembly.clear();

        result.message =
            std::string("Erro de geracao BIP: ") +
            error.what();

        result.position = error.position();

        return result;
    }
    catch (const LexicalError &error)
    {
        result.success = false;
        result.assembly.clear();

        result.message =
            std::string("Erro lexico durante a geracao: ") +
            error.getMessage();

        result.position = error.getPosition();

        return result;
    }
    catch (const std::exception &error)
    {
        result.success = false;
        result.assembly.clear();

        result.message =
            std::string("Falha interna na geracao BIP: ") +
            error.what();

        return result;
    }
}

}
