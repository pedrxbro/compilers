#ifndef SCOPE_MANAGER_H
#define SCOPE_MANAGER_H

#include "Scope.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

using namespace std;

class ScopeManager
{
public:
    ScopeManager();

    Scope &globalScope();
    const Scope &globalScope() const;

    Scope &currentScope();
    const Scope &currentScope() const;

    bool atGlobalScope() const;

    Scope &enterFunctionScope(
        const string &functionName
        );

    Scope &enterInternalScope(
        const string &label = "bloco"
        );

    Scope &leaveScope();

    Symbol *declareSymbol(
        const string &name,
        DataType type,
        SymbolKind kind,
        optional<size_t> vectorSize = nullopt,
        optional<size_t> parameterPosition = nullopt
        );

    Symbol *findInCurrentScope(
        const string &name
        );

    const Symbol *findInCurrentScope(
        const string &name
        ) const;

    Symbol *findVisible(
        const string &name
        );

    const Symbol *findVisible(
        const string &name
        ) const;

    bool isVisible(
        const string &name
        ) const;

private:
    Scope &createChildScope(
        const string &name,
        ScopeKind kind
        );

    unique_ptr<Scope> globalScope_;
    Scope *currentScope_;

    size_t nextScopeId_;
    size_t nextInternalScopeIndex_;
};

#endif // SCOPE_MANAGER_H