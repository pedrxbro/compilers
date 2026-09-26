#ifndef SYMBOL_H
#define SYMBOL_H

#include "SemanticTypes.h"

#include <cstddef>
#include <optional>
#include <string>

class Symbol
{
public:
    Symbol(
        std::string name,
        DataType type,
        SymbolKind kind,
        std::string scope,
        std::optional<std::size_t> vectorSize = std::nullopt,
        std::optional<std::size_t> parameterPosition = std::nullopt
        );

    const std::string &name() const;
    DataType type() const;
    SymbolKind kind() const;
    const std::string &scope() const;

    bool isInitialized() const;
    bool isUsed() const;

    std::optional<std::size_t> vectorSize() const;
    std::optional<std::size_t> parameterPosition() const;

    std::optional<DataType> returnType() const;

    void markInitialized();
    void markUsed();

private:
    std::string name_;
    DataType type_;
    SymbolKind kind_;
    std::string scope_;

    bool initialized_;
    bool used_;

    std::optional<std::size_t> vectorSize_;
    std::optional<std::size_t> parameterPosition_;
};

#endif