#include "Semantico.h"

void Semantico::executeAction(
    int action,
    const Token *token
    )
{
    if (token == nullptr)
    {
        throw SemanticError(
            "Acao semantica recebeu um token invalido."
            );
    }

    const std::string &lexeme =
        token->getLexeme();

    const int position =
        token->getPosition();

    switch (action)
    {
    case 1:
        declarationProcessor_.captureType(
            lexeme,
            position
            );
        break;

    case 23:
        declarationProcessor_
            .beginVariableDeclaration();
        break;

    case 24:
        declarationProcessor_
            .captureIdentifier(
                lexeme
                );
        break;

    case 25:
        declarationProcessor_
            .declareSimple(
                position
                );
        break;

    case 26:
        declarationProcessor_
            .captureVectorSize(
                lexeme,
                position
                );
        break;

    case 27:
        declarationProcessor_
            .declareVector(
                position
                );
        break;

    case 28:
        declarationProcessor_
            .beginParameterDeclaration();
        break;

    case 29:
        declarationProcessor_
            .beginProcedure(
                lexeme,
                position
                );
        break;

    case 30:
        declarationProcessor_
            .beginFunction(
                lexeme,
                position
                );
        break;

    case 32:
        declarationProcessor_
            .endSubroutine(
                position
                );
        break;

    case 46:
        declarationProcessor_
            .enterInternalScope(
                lexeme
                );
        break;

    case 47:
        declarationProcessor_
            .leaveInternalScope(
                position
                );
        break;

    default:
        break;
    }
}

const ScopeManager &Semantico::scopeManager() const
{
    return declarationProcessor_
        .scopeManager();
}