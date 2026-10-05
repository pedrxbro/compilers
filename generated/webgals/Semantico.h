#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"

#include "DeclarationProcessor.h"

class Semantico
{
public:
    void executeAction(
        int action,
        const Token *token
        );

    const ScopeManager &scopeManager() const;

private:
    DeclarationProcessor declarationProcessor_;
};

#endif