#include "Symbol.h"

#include <stdexcept>
#include <utility>

using namespace std;

Symbol::Symbol(
    string name,
    DataType type,
    SymbolKind kind,
    string scope,
    optional<size_t> vectorSize,
    optional<size_t> parameterPosition
    )
    : name_(std::move(name)),
    type_(type),
    kind_(kind),
    scope_(std::move(scope)),
    initialized_(kind == SymbolKind::Parameter),
    used_(false),
    vectorSize_(vectorSize),
    parameterPosition_(parameterPosition)
{
    if (name_.empty())
    {
        throw invalid_argument(
            "Nome do símbolo não pode ser vazio"
            );
    }

    if (scope_.empty())
    {
        throw invalid_argument(
            "Escopo do símbolo não pode ser vazio"
            );
    }

    if (kind_ == SymbolKind::Vector)
    {
        if (!vectorSize_.has_value() ||
            vectorSize_.value() == 0)
        {
            throw invalid_argument(
                "Símbolo do vetor deve ter um tamanho válido"
                );
        }

        if (parameterPosition_.has_value())
        {
            throw invalid_argument(
                "Símbolo do vetor não pode ter uma posição de parâmetro"
                );
        }
    }

    if (kind_ == SymbolKind::Parameter)
    {
        if (!parameterPosition_.has_value() ||
            parameterPosition_.value() == 0)
        {
            throw invalid_argument(
                "Símbolo do parâmetro deve ter uma posição válida"
                );
        }

        if (vectorSize_.has_value() &&
            vectorSize_.value() == 0)
        {
            throw invalid_argument(
                "Parâmetro vetor deve possuir tamanho válido"
                );
        }
    }

    if (kind_ == SymbolKind::Variable ||
        kind_ == SymbolKind::Function)
    {
        if (vectorSize_.has_value())
        {
            throw invalid_argument(
                "Esse tipo de símbolo não pode ter um tamanho de vetor"
                );
        }

        if (parameterPosition_.has_value())
        {
            throw invalid_argument(
                "Esse tipo de símbolo não pode ter uma posição de parâmetro"
                );
        }
    }
}

const string &Symbol::name() const
{
    return name_;
}

DataType Symbol::type() const
{
    return type_;
}

SymbolKind Symbol::kind() const
{
    return kind_;
}

const string &Symbol::scope() const
{
    return scope_;
}

bool Symbol::isInitialized() const
{
    return initialized_;
}

bool Symbol::isUsed() const
{
    return used_;
}

optional<size_t> Symbol::vectorSize() const
{
    return vectorSize_;
}

optional<size_t> Symbol::parameterPosition() const
{
    return parameterPosition_;
}

optional<DataType> Symbol::returnType() const
{
    if (kind_ != SymbolKind::Function)
    {
        return nullopt;
    }

    return type_;
}

void Symbol::markInitialized()
{
    initialized_ = true;
}

void Symbol::markUsed()
{
    used_ = true;
}