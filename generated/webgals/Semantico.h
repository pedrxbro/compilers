#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"

#include "DeclarationProcessor.h"
#include "SemanticTable.h"

#include <optional>
#include <string>
#include <vector>

class Semantico
{
public:
    void executeAction(
        int action,
        const Token *token
        );

    const ScopeManager &scopeManager() const;

    std::vector<std::string> warnings() const;

private:
    struct IdentifierReference
    {
        Symbol *symbol = nullptr;
        std::string name;
        int position = -1;
        bool indexed = false;
    };

    Symbol *resolveIdentifier(
        const std::string &name,
        int position
        );

    void captureIdentifierReference(
        const std::string &name,
        int position
        );

    IdentifierReference popIdentifierReference(
        int position
        );

    IdentifierReference &pendingIdentifierReference(
        int position
        );

    void validateValueReference(
        const IdentifierReference &reference,
        int position
        ) const;

    void validateAssignmentReference(
        const IdentifierReference &reference,
        int position
        ) const;

    void markReferenceUsed(
        const IdentifierReference &reference
        );

    void usePendingIdentifier(
        int position
        );

    void pushPendingIdentifierValue(
        int position
        );

    void captureAssignmentTarget(
        int position
        );

    void completeAssignment(
        int position
        );

    void initializeInputTarget(
        int position
        );

    void validateVectorIndex(
        int position
        );

    void pushLiteralType(
        DataType type
        );

    DataType popExpressionType(
        int position
        );

    BinaryOperator binaryOperatorFromLexeme(
        const std::string &lexeme,
        int position
        ) const;

    UnaryOperator unaryOperatorFromLexeme(
        const std::string &lexeme,
        int position
        ) const;

    void pushBinaryOperator(
        const std::string &lexeme,
        int position
        );

    void pushUnaryOperator(
        const std::string &lexeme,
        int position
        );

    void reduceBinaryExpression(
        int position
        );

    void reduceUnaryExpression(
        int position
        );

    void validateCondition(
        int position
        );

    void discardExpressionResult(
        int position
        );

    void beginSubroutineCall(
        const std::string &name,
        int position
        );

    Symbol *popSubroutineCall(
        int position
        );

    void completeCallAsStatement(
        int position
        );

    void completeCallAsExpression(
        int position
        );

    void captureCurrentSubroutine(
        const std::string &name,
        int position
        );

    void completeReturn(
        int position
        );

    void finalizeProgram(
        int position
        );

    DeclarationProcessor declarationProcessor_;

    std::vector<IdentifierReference>
        pendingIdentifierReferences_;

    std::optional<IdentifierReference>
        assignmentTarget_;

    std::vector<DataType>
        expressionTypes_;

    std::vector<BinaryOperator>
        binaryOperators_;

    std::vector<UnaryOperator>
        unaryOperators_;

    std::vector<Symbol *>
        subroutineCalls_;

    Symbol *currentSubroutine_ = nullptr;

    std::vector<std::string>
        warnings_;
};

#endif