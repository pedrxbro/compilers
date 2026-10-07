#ifndef SCOPE_H
#define SCOPE_H

#include "Symbol.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


using namespace std;

enum class ScopeKind
{
    Global,
    Function,
    Internal
};

class Scope
{
public:
    Scope(
        size_t id,
        string name,
        ScopeKind kind,
        string qualifiedName,
        Scope *parent
        );

    size_t id() const;

    const string &name() const;

    ScopeKind kind() const;

    const string &qualifiedName() const;

    Scope *parent();
    const Scope *parent() const;

    bool isGlobal() const;

    Symbol *declareSymbol(
        const string &name,
        DataType type,
        SymbolKind kind,
        optional<size_t> vectorSize = nullopt,
        optional<size_t> parameterPosition = nullopt
        );

    Symbol *findLocal(const string &name);
    const Symbol *findLocal(const string &name) const;

    size_t symbolCount() const;

    size_t childCount() const;

    const Scope &childAt(size_t index) const;

private:
    friend class ScopeManager;

    Scope &addChild(unique_ptr<Scope> child);

    size_t id_;
    string name_;
    ScopeKind kind_;
    string qualifiedName_;

    Scope *parent_;

    unordered_map<string, Symbol> symbols_;
    vector<unique_ptr<Scope>> children_;
};

#endif // SCOPE_H