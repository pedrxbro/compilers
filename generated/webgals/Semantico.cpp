#include "Semantico.h"

#include <algorithm>
#include <string>
#include <vector>

// Executa uma ação semântica associada a uma regra da gramática.
void Semantico::executeAction(
    int action,
    const Token *token
    )
{
    // Toda ação que depende de token exige um token válido.
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
    // Captura o tipo de uma declaração.
    case 1:
        declarationProcessor_.captureType(
            lexeme,
            position
            );
        break;

    // Inicia uma declaração de variável.
    case 23:
        declarationProcessor_
            .beginVariableDeclaration();
        break;

    // Captura o identificador da declaração.
    case 24:
        declarationProcessor_
            .captureIdentifier(
                lexeme
                );
        break;

    // Finaliza a declaração de uma variável simples.
    case 25:
        declarationProcessor_
            .declareSimple(
                position
                );
        break;

    // Captura o tamanho de um vetor.
    case 26:
        declarationProcessor_
            .captureVectorSize(
                lexeme,
                position
                );
        break;

    // Finaliza a declaração de um vetor.
    case 27:
        declarationProcessor_
            .declareVector(
                position
                );
        break;

    // Inicia uma declaração de parâmetro.
    case 28:
        declarationProcessor_
            .beginParameterDeclaration();
        break;

    // Inicia uma declaração de procedimento.
    case 29:
        declarationProcessor_
            .beginProcedure(
                lexeme,
                position
                );
        break;

    // Inicia uma declaração de função.
    case 30:
        declarationProcessor_
            .beginFunction(
                lexeme,
                position
                );
        break;

    // Finaliza a declaração da sub-rotina atual.
    case 32:
        declarationProcessor_
            .endSubroutine(
                position
                );
        break;

    // Resolve e marca imediatamente um identificador como usado.
    case 33:
        useIdentifierImmediately(
            lexeme,
            position
            );
        break;

    // Resolve e guarda uma referência para uso posterior.
    case 38:
        captureIdentifierReference(
            lexeme,
            position
            );
        break;

    // Define o identificador pendente como destino de atribuição.
    case 40:
        captureAssignmentTarget(
            position
            );
        break;

    // Finaliza a atribuição e inicializa seu destino.
    case 41:
        completeAssignment(
            position
            );
        break;

    // Marca o identificador de entrada como inicializado.
    case 42:
        initializeInputTarget(
            position
            );
        break;

    // Usa o identificador que estava pendente.
    case 43:
        usePendingIdentifier(
            position
            );
        break;

    // Usa o identificador que estava pendente.
    case 44:
        usePendingIdentifier(
            position
            );
        break;

    // Entra em um novo escopo interno.
    case 46:
        declarationProcessor_
            .enterInternalScope(
                lexeme
                );
        break;

    // Sai do escopo interno atual.
    case 47:
        declarationProcessor_
            .leaveInternalScope(
                position
                );
        break;

    // Ignora ações que não possuem tratamento semântico.
    default:
        break;
    }
}

// Retorna o gerenciador de escopos somente para leitura.
const ScopeManager &Semantico::scopeManager() const
{
    return declarationProcessor_
        .scopeManager();
}

// Retorna os avisos semânticos encontrados.
std::vector<SemanticDiagnostic> Semantico::warnings() const
{
    // Começa com os avisos já registrados durante a análise.
    std::vector<SemanticDiagnostic> result =
        warnings_;

    // Obtém todos os símbolos declarados.
    std::vector<const Symbol *> symbols =
        declarationProcessor_
            .scopeManager()
            .allSymbols();

    // Ordena os símbolos por escopo e depois por nome.
    std::sort(
        symbols.begin(),
        symbols.end(),
        [](const Symbol *left,
           const Symbol *right)
        {
            if (left->scope() !=
                right->scope())
            {
                return left->scope() <
                       right->scope();
            }

            return left->name() <
                   right->name();
        }
        );

    // Gera avisos para símbolos declarados mas nunca utilizados.
    for (const Symbol *symbol : symbols)
    {
        if (symbol == nullptr)
        {
            continue;
        }

        if (symbol->isUsed())
        {
            continue;
        }

        result.push_back(
            {
                "Identificador '" +
                    symbol->name() +
                    "' declarado no escopo '" +
                    symbol->scope() +
                    "' e nunca usado.",
                -1
            }
            );
    }

    return result;
}

// Procura um identificador nos escopos visíveis.
Symbol *Semantico::resolveIdentifier(
    const std::string &name,
    int position
    )
{
    ScopeManager &scopeManager =
        declarationProcessor_
            .scopeManager();

    // Procura o símbolo no escopo atual e nos escopos pais.
    Symbol *symbol =
        scopeManager.findVisible(
            name
            );

    // Identificador não declarado ou fora do escopo.
    if (symbol == nullptr)
    {
        throw SemanticError(
            "Identificador '" +
                name +
                "' nao declarado em um escopo visivel a partir de '" +
                scopeManager
                    .currentScope()
                    .qualifiedName() +
                "'.",
            position
            );
    }

    return symbol;
}

// Resolve e armazena uma referência para uso posterior.
void Semantico::captureIdentifierReference(
    const std::string &name,
    int position
    )
{
    Symbol *symbol =
        resolveIdentifier(
            name,
            position
            );

    pendingIdentifierReferences_
        .push_back(
            {
                symbol,
                name,
                position
            }
            );
}

// Remove e retorna a última referência pendente.
Semantico::IdentifierReference
Semantico::popIdentifierReference(
    int position
    )
{
    // Não existe referência disponível para ser utilizada.
    if (pendingIdentifierReferences_.empty())
    {
        throw SemanticError(
            "Referencia de identificador esperada, mas nenhuma referencia esta pendente.",
            position
            );
    }

    // Recupera a referência mais recente.
    IdentifierReference reference =
        pendingIdentifierReferences_.back();

    // Remove a referência da lista de pendentes.
    pendingIdentifierReferences_.pop_back();

    return reference;
}

// Marca uma referência como utilizada e verifica sua inicialização.
void Semantico::markReferenceUsed(
    const IdentifierReference &reference
    )
{
    // Garante que a referência possui um símbolo válido.
    if (reference.symbol == nullptr)
    {
        throw SemanticError(
            "Referencia de identificador invalida.",
            reference.position
            );
    }

    // Variáveis não inicializadas geram um aviso ao serem usadas.
    if (reference.symbol->kind() !=
            SymbolKind::Function &&
        !reference.symbol->isInitialized())
    {
        warnings_.push_back(
            {
                "Identificador '" +
                    reference.name +
                    "' usado antes da inicializacao no escopo '" +
                    reference.symbol->scope() +
                    "'.",
                reference.position
            }
            );
    }

    // Registra que o símbolo foi utilizado.
    reference.symbol->markUsed();
}

// Usa a referência pendente mais recente.
void Semantico::usePendingIdentifier(
    int position
    )
{
    const IdentifierReference reference =
        popIdentifierReference(
            position
            );

    markReferenceUsed(
        reference
        );
}

// Resolve e usa um identificador imediatamente.
void Semantico::useIdentifierImmediately(
    const std::string &name,
    int position
    )
{
    Symbol *symbol =
        resolveIdentifier(
            name,
            position
            );

    const IdentifierReference reference =
        {
            symbol,
            name,
            position
        };

    markReferenceUsed(
        reference
        );
}

// Define a referência pendente como destino da atribuição.
void Semantico::captureAssignmentTarget(
    int position
    )
{
    // Impede que exista mais de um destino pendente.
    if (assignmentTarget_.has_value())
    {
        throw SemanticError(
            "Ja existe um destino de atribuicao pendente.",
            position
            );
    }

    assignmentTarget_ =
        popIdentifierReference(
            position
            );
}

// Finaliza a atribuição e marca seu destino como inicializado.
void Semantico::completeAssignment(
    int position
    )
{
    // Uma atribuição precisa possuir um destino válido.
    if (!assignmentTarget_.has_value())
    {
        throw SemanticError(
            "Destino da atribuicao nao foi identificado.",
            position
            );
    }

    // Atribuição concluída significa que o símbolo foi inicializado.
    assignmentTarget_
        ->symbol
        ->markInitialized();

    // Remove o destino da atribuição pendente.
    assignmentTarget_.reset();
}

// Marca como inicializado o identificador usado como destino de entrada.
void Semantico::initializeInputTarget(
    int position
    )
{
    IdentifierReference reference =
        popIdentifierReference(
            position
            );

    reference.symbol
        ->markInitialized();
}