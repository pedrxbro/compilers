#include "ScopeManager.h"

#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

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

Scope &ScopeManager::globalScope()
{
    return *globalScope_;
}

const Scope &ScopeManager::globalScope() const
{
    return *globalScope_;
}

Scope &ScopeManager::currentScope()
{
    return *currentScope_;
}

const Scope &ScopeManager::currentScope() const
{
    return *currentScope_;
}

bool ScopeManager::atGlobalScope() const
{
    return currentScope_ == globalScope_.get();
}

Scope &ScopeManager::enterFunctionScope(
    const string &functionName
    )
{
    if (functionName.empty())
    {
        throw invalid_argument(
            "Nome da função não pode ser vazio"
            );
    }

    if (!atGlobalScope())
    {
        throw logic_error(
            "Escopo de função deve ser criado a partir do escopo global"
            );
    }

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

    return createChildScope(
        functionName,
        ScopeKind::Function
        );
}

Scope &ScopeManager::enterInternalScope(
    const string &label
    )
{
    string normalizedLabel = label;

    if (normalizedLabel.empty())
    {
        normalizedLabel = "bloco";
    }

    const string scopeName =
        normalizedLabel +
        "#" +
        to_string(nextInternalScopeIndex_++);

    return createChildScope(
        scopeName,
        ScopeKind::Internal
        );
}

Scope &ScopeManager::leaveScope()
{
    if (atGlobalScope())
    {
        throw logic_error(
            "Não é possível sair do escopo global"
            );
    }

    Scope *parent = currentScope_->parent();

    if (parent == nullptr)
    {
        throw logic_error(
            "Escopo atual não possui escopo pai"
            );
    }

    currentScope_ = parent;

    return *currentScope_;
}

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

Symbol *ScopeManager::findInCurrentScope(
    const string &name
    )
{
    return currentScope_->findLocal(name);
}

const Symbol *ScopeManager::findInCurrentScope(
    const string &name
    ) const
{
    return currentScope_->findLocal(name);
}

Symbol *ScopeManager::findVisible(
    const string &name
    )
{
    Scope *scope = currentScope_;

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

const Symbol *ScopeManager::findVisible(
    const string &name
    ) const
{
    const Scope *scope = currentScope_;

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

bool ScopeManager::isVisible(
    const string &name
    ) const
{
    return findVisible(name) != nullptr;
}

Scope &ScopeManager::createChildScope(
    const string &name,
    ScopeKind kind
    )
{
    if (kind == ScopeKind::Global)
    {
        throw invalid_argument(
            "Escopo global não pode ser criado como filho"
            );
    }

    const string qualifiedName =
        currentScope_->qualifiedName() +
        "::" +
        name;

    auto child = make_unique<Scope>(
        nextScopeId_++,
        name,
        kind,
        qualifiedName,
        currentScope_
        );

    Scope &createdScope =
        currentScope_->addChild(
            std::move(child)
            );

    currentScope_ = &createdScope;

    return createdScope;
}