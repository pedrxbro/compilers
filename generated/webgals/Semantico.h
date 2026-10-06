#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"

#include "DeclarationProcessor.h"

#include <optional>
#include <string>
#include <vector>

struct SemanticDiagnostic
{
    std::string message;
    int position = -1;
};

class Semantico
{
public:
    void executeAction(
        int action,
        const Token *token
        );

    const ScopeManager &scopeManager() const;

    std::vector<SemanticDiagnostic> warnings() const;

private:
    struct IdentifierReference
    {
        Symbol *symbol;
        std::string name;
        int position;
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

    void markReferenceUsed(
        const IdentifierReference &reference
        );

    void usePendingIdentifier(
        int position
        );

    void useIdentifierImmediately(
        const std::string &name,
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

    DeclarationProcessor declarationProcessor_;

    std::vector<IdentifierReference>
        pendingIdentifierReferences_;

    std::optional<IdentifierReference>
        assignmentTarget_;

    std::vector<SemanticDiagnostic>
        warnings_;
};

#endif