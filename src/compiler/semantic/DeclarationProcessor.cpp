#include "DeclarationProcessor.h"

#include "SemanticError.h"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>

DeclarationProcessor::DeclarationProcessor()
    : currentType_(DataType::Unknown),
    declarationMode_(DeclarationMode::None),
    currentIdentifier_(),
    currentVectorSize_(nullopt),
    nextParameterPosition_(1)
{
}

// Converte e armazena o tipo atual
void DeclarationProcessor::captureType(
    const string &lexeme,
    int position
    )
{
    currentType_ =
        dataTypeFromLexeme(
            lexeme,
            position
            );
}

void DeclarationProcessor::beginVariableDeclaration()
{
    declarationMode_ =
        DeclarationMode::Variable;

    clearCurrentItem();
}

void DeclarationProcessor::beginParameterDeclaration()
{
    declarationMode_ =
        DeclarationMode::Parameter;

    clearCurrentItem();
}

void DeclarationProcessor::captureIdentifier(
    const string &lexeme
    )
{
    currentIdentifier_ = lexeme;
    currentVectorSize_.reset();
}

// Converte e armazena o tamanho do vetor
void DeclarationProcessor::captureVectorSize(
    const string &lexeme,
    int position
    )
{
    currentVectorSize_ =
        vectorSizeFromLexeme(
            lexeme,
            position
            );
}

// Declara uma variável ou parâmetro simples
void DeclarationProcessor::declareSimple(
    int position
    )
{
    requireCurrentType(position);
    requireIdentifier(position);
    requireDeclarationMode(position);

    // Parâmetros recebem uma posição
    if (declarationMode_ ==
        DeclarationMode::Parameter)
    {
        const size_t parameterPosition =
            nextParameterPosition_++;

        declareCurrentSymbol(
            SymbolKind::Parameter,
            nullopt,
            parameterPosition,
            position
            );

        return;
    }

    // Declara uma variável simples
    declareCurrentSymbol(
        SymbolKind::Variable,
        nullopt,
        nullopt,
        position
        );
}

void DeclarationProcessor::declareVector(
    int position
    )
{
    requireCurrentType(position);
    requireIdentifier(position);
    requireDeclarationMode(position);

    if (!currentVectorSize_.has_value())
    {
        throw SemanticError(
            "Tamanho do vetor nao foi informado.",
            position
            );
    }

    // Vetor dentro de um parâmetro
    if (declarationMode_ ==
        DeclarationMode::Parameter)
    {
        const size_t parameterPosition =
            nextParameterPosition_++;

        declareCurrentSymbol(
            SymbolKind::Parameter,
            currentVectorSize_,
            parameterPosition,
            position
            );

        return;
    }

    declareCurrentSymbol(
        SymbolKind::Vector,
        currentVectorSize_,
        nullopt,
        position
        );
}

void DeclarationProcessor::beginProcedure(
    const string &name,
    int position
    )
{
    declareSubroutine(
        name,
        DataType::Void,
        position
        );

    // Cria o escopo da procedure
    scopeManager_.enterFunctionScope(name);

    nextParameterPosition_ = 1;
    declarationMode_ = DeclarationMode::None;
    currentType_ = DataType::Unknown;

    clearCurrentItem();
}

void DeclarationProcessor::beginFunction(
    const string &name,
    int position
    )
{
    requireCurrentType(position);

    const DataType returnType =
        currentType_;

    declareSubroutine(
        name,
        returnType,
        position
        );

    // Cria o escopo da função
    scopeManager_.enterFunctionScope(name);

    nextParameterPosition_ = 1;
    declarationMode_ = DeclarationMode::None;
    currentType_ = DataType::Unknown;

    clearCurrentItem();
}

void DeclarationProcessor::endSubroutine(
    int position
    )
{
    if (scopeManager_.atGlobalScope())
    {
        throw SemanticError(
            "Tentativa de finalizar sub-rotina fora de um escopo de sub-rotina.",
            position
            );
    }

    if (scopeManager_.currentScope().kind() !=
        ScopeKind::Function)
    {
        throw SemanticError(
            "Existem escopos internos abertos ao finalizar a sub-rotina.",
            position
            );
    }

    scopeManager_.leaveScope();

    currentType_ =
        DataType::Unknown;

    declarationMode_ =
        DeclarationMode::None;

    nextParameterPosition_ = 1;

    clearCurrentItem();
}

void DeclarationProcessor::enterInternalScope(
    const string &label
    )
{
    if (label.empty())
    {
        scopeManager_.enterInternalScope(
            "bloco"
            );

        return;
    }

    scopeManager_.enterInternalScope(
        label
        );
}

void DeclarationProcessor::leaveInternalScope(
    int position
    )
{
    if (scopeManager_.atGlobalScope())
    {
        throw SemanticError(
            "Tentativa de finalizar um escopo interno inexistente.",
            position
            );
    }

    if (scopeManager_.currentScope().kind() !=
        ScopeKind::Internal)
    {
        throw SemanticError(
            "O escopo atual nao e um escopo interno.",
            position
            );
    }

    scopeManager_.leaveScope();
}

ScopeManager &DeclarationProcessor::scopeManager()
{
    return scopeManager_;
}

const ScopeManager &DeclarationProcessor::scopeManager() const
{
    return scopeManager_;
}

DataType DeclarationProcessor::dataTypeFromLexeme(
    const string &lexeme,
    int position
    )
{
    if (lexeme == "inteiro")
    {
        return DataType::Integer;
    }

    if (lexeme == "flutuante")
    {
        return DataType::Float;
    }

    if (lexeme == "texto")
    {
        return DataType::Text;
    }

    if (lexeme == "logico")
    {
        return DataType::Logical;
    }

    if (lexeme == "caractere")
    {
        return DataType::Character;
    }

    throw SemanticError(
        "Tipo desconhecido: '" +
            lexeme +
            "'.",
        position
        );
}

size_t DeclarationProcessor::vectorSizeFromLexeme(
    const string &lexeme,
    int position
    )
{
    if (lexeme.empty())
    {
        throw SemanticError(
            "Tamanho de vetor vazio.",
            position
            );
    }

    try
    {
        unsigned long long value = 0;

        if (lexeme.size() > 2 &&
            lexeme[0] == '0' &&
            (lexeme[1] == 'b' ||
             lexeme[1] == 'B'))
        {
            value =
                stoull(
                    lexeme.substr(2),
                    nullptr,
                    2
                    );
        }
        else if (
            lexeme.size() > 2 &&
            lexeme[0] == '0' &&
            (lexeme[1] == 'x' ||
             lexeme[1] == 'X'))
        {
            value =
                stoull(
                    lexeme.substr(2),
                    nullptr,
                    16
                    );
        }
        else
        {
            value =
                stoull(
                    lexeme,
                    nullptr,
                    10
                    );
        }

        if (value == 0)
        {
            throw SemanticError(
                "O tamanho de um vetor deve ser maior que zero.",
                position
                );
        }

        return static_cast<size_t>(
            value
            );
    }
    catch (const SemanticError &)
    {
        throw;
    }
    catch (const exception &)
    {
        throw SemanticError(
            "Tamanho de vetor invalido: '" +
                lexeme +
                "'.",
            position
            );
    }
}

void DeclarationProcessor::requireCurrentType(
    int position
    ) const
{
    if (currentType_ ==
        DataType::Unknown)
    {
        throw SemanticError(
            "Tipo da declaracao nao foi definido.",
            position
            );
    }
}

void DeclarationProcessor::requireIdentifier(
    int position
    ) const
{
    if (currentIdentifier_.empty())
    {
        throw SemanticError(
            "Identificador da declaracao nao foi definido.",
            position
            );
    }
}

void DeclarationProcessor::requireDeclarationMode(
    int position
    ) const
{
    if (declarationMode_ ==
        DeclarationMode::None)
    {
        throw SemanticError(
            "Modalidade da declaracao nao foi definida.",
            position
            );
    }
}

void DeclarationProcessor::declareCurrentSymbol(
    SymbolKind kind,
    optional<size_t> vectorSize,
    optional<size_t> parameterPosition,
    int position
    )
{
    Symbol *symbol =
        scopeManager_.declareSymbol(
            currentIdentifier_,
            currentType_,
            kind,
            vectorSize,
            parameterPosition
            );

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Identificador '" +
                currentIdentifier_ +
                "' ja declarado no escopo '" +
                scopeManager_
                    .currentScope()
                    .qualifiedName() +
                "'.",
            position
            );
    }

    clearCurrentItem();
}

void DeclarationProcessor::declareSubroutine(
    const string &name,
    DataType type,
    int position
    )
{
    if (!scopeManager_.atGlobalScope())
    {
        throw SemanticError(
            "Sub-rotinas devem ser declaradas no escopo global.",
            position
            );
    }

    Symbol *symbol =
        scopeManager_.declareSymbol(
            name,
            type,
            SymbolKind::Function
            );

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Identificador '" +
                name +
                "' ja declarado no escopo 'global'.",
            position
            );
    }
}

void DeclarationProcessor::clearCurrentItem()
{
    currentIdentifier_.clear();
    currentVectorSize_.reset();
}