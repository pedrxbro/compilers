#include "ScopeManager.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// Inicializa o escopo global e define o escopo atual como global
ScopeManager::ScopeManager()
    : globalScope_(
          make_unique<Scope>(
              0,
              "global",
              ScopeKind::Global,
              "global",
              nullptr
              )
          ),
    currentScope_(globalScope_.get()),
    nextScopeId_(1),
    nextInternalScopeIndex_(1)
{
}

// Retorna o escopo global
Scope &ScopeManager::globalScope()
{
    return *globalScope_;
}

// Retorna o escopo global somente para leitura
const Scope &ScopeManager::globalScope() const
{
    return *globalScope_;
}

// Retorna o escopo atualmente ativo.
Scope &ScopeManager::currentScope()
{
    return *currentScope_;
}

// Retorna o escopo atual somente para leitura
const Scope &ScopeManager::currentScope() const
{
    return *currentScope_;
}

// Verifica se o escopo atual é o global
bool ScopeManager::atGlobalScope() const
{
    return currentScope_ == globalScope_.get();
}

// Entra em um novo escopo de função
Scope &ScopeManager::enterFunctionScope(
    const string &functionName
    )
{
    // O nome da função é obrigatório
    if (functionName.empty())
    {
        throw invalid_argument(
            "Nome da função não pode ser vazio"
            );
    }

    // Funções só podem ser criadas a partir do escopo global
    if (!atGlobalScope())
    {
        throw logic_error(
            "Escopo de função deve ser criado a partir do escopo global"
            );
    }

    // Verifica se já existe uma função com esse nome
    for (size_t index = 0;
         index < globalScope_->childCount();
         ++index)
    {
        const Scope &child =
            globalScope_->childAt(index);

        if (child.kind() == ScopeKind::Function &&
            child.name() == functionName)
        {
            throw invalid_argument(
                "Já existe um escopo de função com esse nome"
                );
        }
    }

    // Cria a função como filha do escopo atual
    return createChildScope(
        functionName,
        ScopeKind::Function
        );
}

// Entra em um novo escopo interno (if, while ou bloco)
Scope &ScopeManager::enterInternalScope(
    const string &label
    )
{
    string normalizedLabel = label;

    // Usa "bloco" quando nenhum nome é informado.
    if (normalizedLabel.empty())
    {
        normalizedLabel = "bloco";
    }

    // Gera um nome único para o escopo interno.
    const string scopeName =
        normalizedLabel +
        "#" +
        to_string(nextInternalScopeIndex_++);

    // Cria o escopo interno como filho do escopo atual.
    return createChildScope(
        scopeName,
        ScopeKind::Internal
        );
}

// Sai do escopo atual e retorna para o escopo pai.
Scope &ScopeManager::leaveScope()
{
    // O escopo global não possui um pai.
    if (atGlobalScope())
    {
        throw logic_error(
            "Não é possível sair do escopo global"
            );
    }

    // Obtém o escopo pai.
    Scope *parent = currentScope_->parent();

    // Garante que o escopo atual possui um pai válido.
    if (parent == nullptr)
    {
        throw logic_error(
            "Escopo atual não possui escopo pai"
            );
    }

    // Volta para o escopo pai.
    currentScope_ = parent;

    return *currentScope_;
}

// Declara um símbolo no escopo atual.
Symbol *ScopeManager::declareSymbol(
    const string &name,
    DataType type,
    SymbolKind kind,
    optional<size_t> vectorSize,
    optional<size_t> parameterPosition
    )
{
    return currentScope_->declareSymbol(
        name,
        type,
        kind,
        vectorSize,
        parameterPosition
        );
}

// Procura um símbolo somente no escopo atual.
Symbol *ScopeManager::findInCurrentScope(
    const string &name
    )
{
    return currentScope_->findLocal(name);
}

// Versão const da busca no escopo atual.
const Symbol *ScopeManager::findInCurrentScope(
    const string &name
    ) const
{
    return currentScope_->findLocal(name);
}

// Procura um símbolo no escopo atual e em seus pais.
Symbol *ScopeManager::findVisible(
    const string &name
    )
{
    Scope *scope = currentScope_;

    // Sobe pela árvore de escopos até encontrar o símbolo.
    while (scope != nullptr)
    {
        Symbol *symbol =
            scope->findLocal(name);

        if (symbol != nullptr)
        {
            return symbol;
        }

        scope = scope->parent();
    }

    return nullptr;
}

// Versão const da busca por símbolo visível.
const Symbol *ScopeManager::findVisible(
    const string &name
    ) const
{
    const Scope *scope = currentScope_;

    // Sobe pela árvore de escopos até encontrar o símbolo.
    while (scope != nullptr)
    {
        const Symbol *symbol =
            scope->findLocal(name);

        if (symbol != nullptr)
        {
            return symbol;
        }

        scope = scope->parent();
    }

    return nullptr;
}

// Verifica se um símbolo está visível a partir do escopo atual.
bool ScopeManager::isVisible(
    const string &name
    ) const
{
    return findVisible(name) != nullptr;
}

// Retorna todos os símbolos de todos os escopos.
vector<const Symbol *> ScopeManager::allSymbols() const
{
    vector<const Symbol *> result;

    // Percorre a árvore começando pelo escopo global.
    collectSymbols(
        *globalScope_,
        result
        );

    return result;
}

// Percorre recursivamente um escopo e seus filhos.
void ScopeManager::collectSymbols(
    const Scope &scope,
    vector<const Symbol *> &result
    ) const
{
    // Adiciona os símbolos do escopo atual.
    for (const auto &entry : scope.symbols_)
    {
        result.push_back(
            &entry.second
            );
    }

    // Percorre todos os escopos filhos.
    for (const auto &child : scope.children_)
    {
        collectSymbols(
            *child,
            result
            );
    }
}

// Cria um novo escopo filho do escopo atual.
Scope &ScopeManager::createChildScope(
    const string &name,
    ScopeKind kind
    )
{
    // O escopo global é criado apenas pelo construtor.
    if (kind == ScopeKind::Global)
    {
        throw invalid_argument(
            "Escopo global não pode ser criado como filho"
            );
    }

    // Monta o nome completo do escopo.
    const string qualifiedName =
        currentScope_->qualifiedName() +
        "::" +
        name;

    // Cria o novo escopo apontando para o atual como parent.
    auto child = make_unique<Scope>(
        nextScopeId_++,
        name,
        kind,
        qualifiedName,
        currentScope_
        );

    // Adiciona o novo escopo como filho do atual.
    Scope &createdScope =
        currentScope_->addChild(
            std::move(child)
            );

    // O novo escopo passa a ser o escopo atual.
    currentScope_ = &createdScope;

    return createdScope;
}
