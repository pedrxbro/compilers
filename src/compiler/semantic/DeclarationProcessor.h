#ifndef DECLARATION_PROCESSOR_H
#define DECLARATION_PROCESSOR_H

#include "ScopeManager.h"

#include <cstddef>
#include <optional>
#include <string>

using namespace std;

class DeclarationProcessor
{
public:
    DeclarationProcessor();

    void captureType(
        const string &lexeme,
        int position
        );

    void beginVariableDeclaration();

    void beginParameterDeclaration();

    void captureIdentifier(
        const string &lexeme
        );

    void captureVectorSize(
        const string &lexeme,
        int position
        );

    void declareSimple(
        int position
        );

    void declareVector(
        int position
        );

    void beginProcedure(
        const string &name,
        int position
        );

    void beginFunction(
        const string &name,
        int position
        );

    void endSubroutine(
        int position
        );

    void enterInternalScope(
        const string &label
        );

    void leaveInternalScope(
        int position
        );

    ScopeManager &scopeManager();

    const ScopeManager &scopeManager() const;

private:
    enum class DeclarationMode
    {
        None,
        Variable,
        Parameter
    };

    static DataType dataTypeFromLexeme(
        const string &lexeme,
        int position
        );

    static size_t vectorSizeFromLexeme(
        const string &lexeme,
        int position
        );

    void requireCurrentType(
        int position
        ) const;

    void requireIdentifier(
        int position
        ) const;

    void requireDeclarationMode(
        int position
        ) const;

    void declareCurrentSymbol(
        SymbolKind kind,
        optional<size_t> vectorSize,
        optional<size_t> parameterPosition,
        int position
        );

    void declareSubroutine(
        const string &name,
        DataType type,
        int position
        );

    void clearCurrentItem();

    ScopeManager scopeManager_;

    DataType currentType_;
    DeclarationMode declarationMode_;

    string currentIdentifier_;
    optional<size_t> currentVectorSize_;

    size_t nextParameterPosition_;
};

#endif