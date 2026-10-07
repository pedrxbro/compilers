#include "Scope.h"

#include <stdexcept>
#include <utility>

using namespace std;

Scope::Scope(
    size_t id,
    string name,
    ScopeKind kind,
    string qualifiedName,
    Scope *parent
    )
    : id_(id),
    name_(std::move(name)),
    kind_(kind),
    qualifiedName_(std::move(qualifiedName)),
    parent_(parent)
{
    if (name_.empty())
    {
        throw invalid_argument(
            "Nome do escopo não pode ser vazio"
            );
    }

    if (qualifiedName_.empty())
    {
        throw invalid_argument(
            "Nome qualificado do escopo não pode ser vazio"
            );
    }

    if (kind_ == ScopeKind::Global && parent_ != nullptr)
    {
        throw invalid_argument(
            "Escopo global não pode possuir escopo pai"
            );
    }

    if (kind_ != ScopeKind::Global && parent_ == nullptr)
    {
        throw invalid_argument(
            "Escopo não global deve possuir um escopo pai"
            );
    }
}

size_t Scope::id() const
{
    return id_;
}

const string &Scope::name() const
{
    return name_;
}

ScopeKind Scope::kind() const
{
    return kind_;
}

const string &Scope::qualifiedName() const
{
    return qualifiedName_;
}

Scope *Scope::parent()
{
    return parent_;
}

const Scope *Scope::parent() const
{
    return parent_;
}

bool Scope::isGlobal() const
{
    return kind_ == ScopeKind::Global;
}

Symbol *Scope::declareSymbol(
    const string &name,
    DataType type,
    SymbolKind kind,
    optional<size_t> vectorSize,
    optional<size_t> parameterPosition
    )
{
    if (symbols_.find(name) != symbols_.end())
    {
        return nullptr;
    }

    Symbol symbol(
        name,
        type,
        kind,
        qualifiedName_,
        vectorSize,
        parameterPosition
        );

    auto result = symbols_.emplace(
        name,
        std::move(symbol)
        );

    if (!result.second)
    {
        return nullptr;
    }

    return &result.first->second;
}

Symbol *Scope::findLocal(const string &name)
{
    const auto iterator = symbols_.find(name);

    if (iterator == symbols_.end())
    {
        return nullptr;
    }

    return &iterator->second;
}

const Symbol *Scope::findLocal(const string &name) const
{
    const auto iterator = symbols_.find(name);

    if (iterator == symbols_.end())
    {
        return nullptr;
    }

    return &iterator->second;
}

size_t Scope::symbolCount() const
{
    return symbols_.size();
}

size_t Scope::childCount() const
{
    return children_.size();
}

const Scope &Scope::childAt(size_t index) const
{
    return *children_.at(index);
}

Scope &Scope::addChild(unique_ptr<Scope> child)
{
    if (!child)
    {
        throw invalid_argument(
            "Escopo filho inválido"
            );
    }

    if (child->parent() != this)
    {
        throw invalid_argument(
            "O escopo filho deve apontar para o escopo pai correto"
            );
    }

    children_.push_back(std::move(child));

    return *children_.back();
}