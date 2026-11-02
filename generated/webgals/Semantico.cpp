
#include "Semantico.h"

#include <algorithm>
#include <string>
#include <vector>

// Executa a ação semântica associada a uma regra da gramática.
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

    const std::string &lexeme = token->getLexeme();
    const int position = token->getPosition();

    switch (action)
    {
    // ========================================================
    // TIPOS DAS DECLARACOES - M2.1
    // ========================================================
    case 1:
        declarationProcessor_.captureType(lexeme, position);
        break;

    // ========================================================
    // TIPOS DOS LITERAIS - M2.1
    // ========================================================
    case 2:
        pushLiteralType(DataType::Integer);
        break;

    case 3:
        pushLiteralType(DataType::Float);
        break;

    case 4:
        pushLiteralType(DataType::Character);
        break;

    case 5:
        pushLiteralType(DataType::Text);
        break;

    case 6:
        pushLiteralType(DataType::Logical);
        break;

    case 7:
        pushLiteralType(DataType::Unknown);
        break;

    // ========================================================
    // OPERADORES BINARIOS - M2.1
    // ========================================================
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        pushBinaryOperator(lexeme, position);
        break;

    // ========================================================
    // OPERADORES UNARIOS - M2.1
    // ========================================================
    case 14:
    case 15:
        pushUnaryOperator(lexeme, position);
        break;

    // ========================================================
    // REDUCOES BINARIAS - M2.1
    // ========================================================
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
        reduceBinaryExpression(position);
        break;

    case 22:
        reduceUnaryExpression(position);
        break;

    // ========================================================
    // DECLARACOES - M2.1
    // ========================================================
    case 23:
        declarationProcessor_.beginVariableDeclaration();
        break;

    case 24:
        declarationProcessor_.captureIdentifier(lexeme);
        break;

    case 25:
        declarationProcessor_.declareSimple(position);
        break;

    case 26:
        declarationProcessor_.captureVectorSize(
            lexeme,
            position
            );
        break;

    case 27:
        declarationProcessor_.declareVector(position);
        break;

    case 28:
        declarationProcessor_.beginParameterDeclaration();
        break;

    // ========================================================
    // SUB-ROTINAS - M2.1
    // ========================================================
    case 29:
        declarationProcessor_.beginProcedure(
            lexeme,
            position
            );
        captureCurrentSubroutine(lexeme, position);
        break;

    case 30:
        declarationProcessor_.beginFunction(
            lexeme,
            position
            );
        captureCurrentSubroutine(lexeme, position);
        break;

    case 31:
        completeReturn(position);
        break;

    case 32:
        declarationProcessor_.endSubroutine(position);
        currentSubroutine_ = nullptr;
        break;

    // ========================================================
    // CHAMADAS DE SUB-ROTINAS - M2.1
    // ========================================================
    case 33:
        beginSubroutineCall(lexeme, position);
        break;

    case 34:
        discardExpressionResult(position);
        break;

    case 35:
        // Marca o fim da lista de argumentos.
        break;

    case 36:
        completeCallAsStatement(position);
        break;

    case 37:
        completeCallAsExpression(position);
        break;

    // ========================================================
    // IDENTIFICADORES E VETORES - M2.1
    // ========================================================
    case 38:
        captureIdentifierReference(lexeme, position);
        break;

    case 39:
        validateVectorIndex(position);
        break;

    // ========================================================
    // ATRIBUICOES - M2.1
    // ========================================================
    case 40:
        captureAssignmentTarget(position);
        break;

    case 41:
        completeAssignment(position);
        break;

    // ========================================================
    // ENTRADA E SAIDA - M2.1
    // ========================================================
    case 42:
        initializeInputTarget(position);
        break;

    case 43:
        usePendingIdentifier(position);
        break;

    // ========================================================
    // OPERANDOS - M2.1
    // ========================================================
    case 44:
        pushPendingIdentifierValue(position);
        break;

    // ========================================================
    // CONDICOES - M2.1
    // ========================================================
    case 45:
        validateCondition(position);
        break;

    // ========================================================
    // ESCOPOS - M2.1
    // ========================================================
    case 46:
        declarationProcessor_.enterInternalScope(lexeme);
        break;

    case 47:
        declarationProcessor_.leaveInternalScope(position);
        break;

    // ========================================================
    // FINALIZACAO - M2.1
    // ========================================================
    case 48:
        finalizeProgram(position);
        break;

        // ========================================================
        // OPERADORES BIT A BIT - M2.2
        // ========================================================

    // Captura os operadores binários bit a bit.
    case 49: // ouBitBit
    case 51: // ouExclusivoBitBit
    case 53: // eBitBit
    case 55: // deslocaDireita ou deslocaEsquerda
        pushBinaryOperator(lexeme, position);
        break;

    // Reduz as expressões binárias bit a bit.
    case 50: // ouBitBit
    case 52: // ouExclusivoBitBit
    case 54: // eBitBit
    case 56: // deslocamentos
        reduceBinaryExpression(position);
        break;

    // Captura o complemento bit a bit.
    // A redução continua utilizando a ação #22.
    case 57:
        pushUnaryOperator(lexeme, position);
        break;

    default:
        break;
    }
}

// ============================================================
// CONSULTA DE ESCOPOS E DIAGNOSTICOS
// ============================================================

const ScopeManager &Semantico::scopeManager() const
{
    return declarationProcessor_.scopeManager();
}

std::vector<SemanticDiagnostic> Semantico::warnings() const
{
    std::vector<SemanticDiagnostic> result = warnings_;

    std::vector<const Symbol *> symbols =
        declarationProcessor_
            .scopeManager()
            .allSymbols();

    std::sort(
        symbols.begin(),
        symbols.end(),
        [](const Symbol *left, const Symbol *right)
        {
            if (left->scope() != right->scope())
            {
                return left->scope() < right->scope();
            }

            return left->name() < right->name();
        }
        );

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

// ============================================================
// IDENTIFICADORES
// ============================================================

Symbol *Semantico::resolveIdentifier(
    const std::string &name,
    int position
    )
{
    ScopeManager &scopeManager =
        declarationProcessor_.scopeManager();

    Symbol *symbol = scopeManager.findVisible(name);

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

void Semantico::captureIdentifierReference(
    const std::string &name,
    int position
    )
{
    Symbol *symbol = resolveIdentifier(name, position);

    pendingIdentifierReferences_.push_back(
        {
            symbol,
            name,
            position,
            false
        }
        );
}

Semantico::IdentifierReference
Semantico::popIdentifierReference(
    int position
    )
{
    if (pendingIdentifierReferences_.empty())
    {
        throw SemanticError(
            "Referencia de identificador esperada, mas nenhuma referencia esta pendente.",
            position
            );
    }

    IdentifierReference reference =
        pendingIdentifierReferences_.back();

    pendingIdentifierReferences_.pop_back();

    return reference;
}

Semantico::IdentifierReference &
Semantico::pendingIdentifierReference(
    int position
    )
{
    if (pendingIdentifierReferences_.empty())
    {
        throw SemanticError(
            "Referencia de identificador esperada para acesso ao vetor.",
            position
            );
    }

    return pendingIdentifierReferences_.back();
}

void Semantico::validateValueReference(
    const IdentifierReference &reference,
    int position
    ) const
{
    if (reference.symbol == nullptr)
    {
        throw SemanticError(
            "Referencia de identificador invalida.",
            position
            );
    }

    if (reference.symbol->kind() == SymbolKind::Function)
    {
        throw SemanticError(
            "Sub-rotina '" +
                reference.name +
                "' deve ser chamada para produzir um valor.",
            position
            );
    }

    if (reference.symbol->vectorSize().has_value() &&
        !reference.indexed)
    {
        throw SemanticError(
            "Vetor '" +
                reference.name +
                "' deve possuir um indice para ser usado como valor.",
            position
            );
    }
}

void Semantico::validateAssignmentReference(
    const IdentifierReference &reference,
    int position
    ) const
{
    if (reference.symbol == nullptr)
    {
        throw SemanticError(
            "Destino de atribuicao invalido.",
            position
            );
    }

    if (reference.symbol->kind() == SymbolKind::Function)
    {
        throw SemanticError(
            "Sub-rotina '" +
                reference.name +
                "' nao pode receber atribuicao.",
            position
            );
    }

    if (reference.symbol->vectorSize().has_value() &&
        !reference.indexed)
    {
        throw SemanticError(
            "Vetor '" +
                reference.name +
                "' deve possuir um indice para receber atribuicao.",
            position
            );
    }
}

void Semantico::markReferenceUsed(
    const IdentifierReference &reference
    )
{
    if (reference.symbol == nullptr)
    {
        throw SemanticError(
            "Referencia de identificador invalida.",
            reference.position
            );
    }

    if (reference.symbol->kind() != SymbolKind::Function &&
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

    reference.symbol->markUsed();
}

void Semantico::usePendingIdentifier(
    int position
    )
{
    const IdentifierReference reference =
        popIdentifierReference(position);

    validateValueReference(reference, position);
    markReferenceUsed(reference);
}

void Semantico::pushPendingIdentifierValue(
    int position
    )
{
    const IdentifierReference reference =
        popIdentifierReference(position);

    validateValueReference(reference, position);
    markReferenceUsed(reference);

    expressionTypes_.push_back(
        reference.symbol->type()
        );
}

// ============================================================
// ATRIBUICOES
// ============================================================

void Semantico::captureAssignmentTarget(
    int position
    )
{
    if (assignmentTarget_.has_value())
    {
        throw SemanticError(
            "Ja existe um destino de atribuicao pendente.",
            position
            );
    }

    IdentifierReference reference =
        popIdentifierReference(position);

    validateAssignmentReference(reference, position);

    assignmentTarget_ = reference;
}

void Semantico::completeAssignment(
    int position
    )
{
    if (!assignmentTarget_.has_value())
    {
        throw SemanticError(
            "Destino da atribuicao nao foi identificado.",
            position
            );
    }

    const DataType expressionType =
        popExpressionType(position);

    const DataType destinationType =
        assignmentTarget_->symbol->type();

    const CompatibilityResult result =
        SemanticTable::atribType(
            destinationType,
            expressionType
            );

    if (result == CompatibilityResult::Error)
    {
        throw SemanticError(
            "Atribuicao incompativel: identificador '" +
                assignmentTarget_->name +
                "' e do tipo " +
                SemanticTable::typeName(destinationType) +
                ", mas a expressao resulta em " +
                SemanticTable::typeName(expressionType) +
                ".",
            position
            );
    }

    if (result == CompatibilityResult::Warning)
    {
        warnings_.push_back(
            {
                "Atribuicao com conversao de " +
                    SemanticTable::typeName(expressionType) +
                    " para " +
                    SemanticTable::typeName(destinationType) +
                    " no identificador '" +
                    assignmentTarget_->name +
                    "' pode causar perda de dados.",
                position
            }
            );
    }

    assignmentTarget_->symbol->markInitialized();
    assignmentTarget_.reset();
}

void Semantico::initializeInputTarget(
    int position
    )
{
    IdentifierReference reference =
        popIdentifierReference(position);

    validateAssignmentReference(reference, position);

    reference.symbol->markInitialized();
}

// ============================================================
// VETORES
// ============================================================

void Semantico::validateVectorIndex(
    int position
    )
{
    const DataType indexType =
        popExpressionType(position);

    IdentifierReference &reference =
        pendingIdentifierReference(position);

    if (reference.symbol == nullptr)
    {
        throw SemanticError(
            "Referencia de vetor invalida.",
            position
            );
    }

    if (!reference.symbol->vectorSize().has_value())
    {
        throw SemanticError(
            "Identificador '" +
                reference.name +
                "' nao e um vetor.",
            position
            );
    }

    if (indexType != DataType::Integer)
    {
        throw SemanticError(
            "Indice do vetor '" +
                reference.name +
                "' deve ser INTEIRO, mas a expressao resulta em " +
                SemanticTable::typeName(indexType) +
                ".",
            position
            );
    }

    reference.indexed = true;
}

// ============================================================
// PILHA DE TIPOS
// ============================================================

void Semantico::pushLiteralType(
    DataType type
    )
{
    expressionTypes_.push_back(type);
}

DataType Semantico::popExpressionType(
    int position
    )
{
    if (expressionTypes_.empty())
    {
        throw SemanticError(
            "Tipo de expressao esperado, mas a pilha de tipos esta vazia.",
            position
            );
    }

    const DataType type = expressionTypes_.back();

    expressionTypes_.pop_back();

    return type;
}

// ============================================================
// RECONHECIMENTO DOS OPERADORES
// M2.1 + M2.2
// ============================================================

BinaryOperator Semantico::binaryOperatorFromLexeme(
    const std::string &lexeme,
    int position
    ) const
{
    // Operadores aritméticos da M2.1.
    if (lexeme == "mais")
    {
        return BinaryOperator::Add;
    }

    if (lexeme == "menos")
    {
        return BinaryOperator::Subtract;
    }

    if (lexeme == "vezes")
    {
        return BinaryOperator::Multiply;
    }

    if (lexeme == "divido")
    {
        return BinaryOperator::Divide;
    }

    if (lexeme == "resto")
    {
        return BinaryOperator::Modulo;
    }

    // Operadores relacionais da M2.1.
    if (lexeme == "maiorQue" ||
        lexeme == "menorQue" ||
        lexeme == "maiorIgualQue" ||
        lexeme == "menorIgualQue")
    {
        return BinaryOperator::Relational;
    }

    if (lexeme == "igual" ||
        lexeme == "diferente")
    {
        return BinaryOperator::Equality;
    }

    // Operadores lógicos da M2.1.
    if (lexeme == "e")
    {
        return BinaryOperator::LogicalAnd;
    }

    if (lexeme == "ou")
    {
        return BinaryOperator::LogicalOr;
    }

    // Operadores bit a bit da M2.2.
    if (lexeme == "eBitBit")
    {
        return BinaryOperator::BitwiseAnd;
    }

    if (lexeme == "ouBitBit")
    {
        return BinaryOperator::BitwiseOr;
    }

    if (lexeme == "ouExclusivoBitBit")
    {
        return BinaryOperator::BitwiseXor;
    }

    if (lexeme == "deslocaEsquerda")
    {
        return BinaryOperator::ShiftLeft;
    }

    if (lexeme == "deslocaDireita")
    {
        return BinaryOperator::ShiftRight;
    }

    throw SemanticError(
        "Operador binario desconhecido: '" +
            lexeme +
            "'.",
        position
        );
}

UnaryOperator Semantico::unaryOperatorFromLexeme(
    const std::string &lexeme,
    int position
    ) const
{
    if (lexeme == "menos")
    {
        return UnaryOperator::Negate;
    }

    if (lexeme == "nao")
    {
        return UnaryOperator::LogicalNot;
    }

    if (lexeme == "naoBitBit")
    {
        return UnaryOperator::BitwiseNot;
    }

    throw SemanticError(
        "Operador unario desconhecido: '" +
            lexeme +
            "'.",
        position
        );
}

// ============================================================
// EMPILHAMENTO DE OPERADORES
// ============================================================

void Semantico::pushBinaryOperator(
    const std::string &lexeme,
    int position
    )
{
    binaryOperators_.push_back(
        binaryOperatorFromLexeme(
            lexeme,
            position
            )
        );
}

void Semantico::pushUnaryOperator(
    const std::string &lexeme,
    int position
    )
{
    unaryOperators_.push_back(
        unaryOperatorFromLexeme(
            lexeme,
            position
            )
        );
}

// ============================================================
// REDUCOES SEMANTICAS
// ============================================================

void Semantico::reduceBinaryExpression(
    int position
    )
{
    if (binaryOperators_.empty())
    {
        throw SemanticError(
            "Operador binario esperado, mas nenhum operador esta pendente.",
            position
            );
    }

    const DataType right =
        popExpressionType(position);

    const DataType left =
        popExpressionType(position);

    const BinaryOperator operation =
        binaryOperators_.back();

    binaryOperators_.pop_back();

    const DataType result =
        SemanticTable::resultType(
            left,
            right,
            operation
            );

    if (result == DataType::Unknown)
    {
        throw SemanticError(
            "Operacao '" +
                SemanticTable::operatorName(operation) +
                "' incompativel entre " +
                SemanticTable::typeName(left) +
                " e " +
                SemanticTable::typeName(right) +
                ".",
            position
            );
    }

    expressionTypes_.push_back(result);
}

void Semantico::reduceUnaryExpression(
    int position
    )
{
    if (unaryOperators_.empty())
    {
        throw SemanticError(
            "Operador unario esperado, mas nenhum operador esta pendente.",
            position
            );
    }

    const DataType operand =
        popExpressionType(position);

    const UnaryOperator operation =
        unaryOperators_.back();

    unaryOperators_.pop_back();

    const DataType result =
        SemanticTable::resultType(
            operand,
            operation
            );

    if (result == DataType::Unknown)
    {
        throw SemanticError(
            "Operacao unaria '" +
                SemanticTable::operatorName(operation) +
                "' incompativel com o tipo " +
                SemanticTable::typeName(operand) +
                ".",
            position
            );
    }

    expressionTypes_.push_back(result);
}

// ============================================================
// CONDICOES
// ============================================================

void Semantico::validateCondition(
    int position
    )
{
    const DataType conditionType =
        popExpressionType(position);

    if (conditionType != DataType::Logical)
    {
        throw SemanticError(
            "Condicao deve resultar em LOGICO, mas a expressao resulta em " +
                SemanticTable::typeName(conditionType) +
                ".",
            position
            );
    }
}

void Semantico::discardExpressionResult(
    int position
    )
{
    popExpressionType(position);
}

// ============================================================
// CHAMADAS DE SUB-ROTINAS
// ============================================================

void Semantico::beginSubroutineCall(
    const std::string &name,
    int position
    )
{
    Symbol *symbol =
        resolveIdentifier(name, position);

    if (symbol->kind() != SymbolKind::Function)
    {
        throw SemanticError(
            "Identificador '" +
                name +
                "' nao e uma sub-rotina.",
            position
            );
    }

    symbol->markUsed();

    subroutineCalls_.push_back(symbol);
}

Symbol *Semantico::popSubroutineCall(
    int position
    )
{
    if (subroutineCalls_.empty())
    {
        throw SemanticError(
            "Chamada de sub-rotina esperada, mas nenhuma chamada esta pendente.",
            position
            );
    }

    Symbol *symbol = subroutineCalls_.back();

    subroutineCalls_.pop_back();

    return symbol;
}

void Semantico::completeCallAsStatement(
    int position
    )
{
    Symbol *symbol =
        popSubroutineCall(position);

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Chamada de sub-rotina invalida.",
            position
            );
    }

    // O valor retornado pela função não é utilizado.
}

void Semantico::completeCallAsExpression(
    int position
    )
{
    Symbol *symbol =
        popSubroutineCall(position);

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Chamada de funcao invalida.",
            position
            );
    }

    if (symbol->type() == DataType::Void)
    {
        throw SemanticError(
            "Procedimento '" +
                symbol->name() +
                "' nao pode participar de uma expressao.",
            position
            );
    }

    expressionTypes_.push_back(
        symbol->type()
        );
}

void Semantico::captureCurrentSubroutine(
    const std::string &name,
    int position
    )
{
    Symbol *symbol =
        declarationProcessor_
            .scopeManager()
            .globalScope()
            .findLocal(name);

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Nao foi possivel localizar a sub-rotina '" +
                name +
                "' apos sua declaracao.",
            position
            );
    }

    currentSubroutine_ = symbol;
}

// ============================================================
// RETORNO DE FUNCOES
// ============================================================

void Semantico::completeReturn(
    int position
    )
{
    const DataType expressionType =
        popExpressionType(position);

    if (currentSubroutine_ == nullptr)
    {
        throw SemanticError(
            "Comando retorne utilizado fora de uma funcao.",
            position
            );
    }

    if (currentSubroutine_->type() == DataType::Void)
    {
        throw SemanticError(
            "Procedimento nao pode retornar uma expressao.",
            position
            );
    }

    const CompatibilityResult result =
        SemanticTable::atribType(
            currentSubroutine_->type(),
            expressionType
            );

    if (result == CompatibilityResult::Error)
    {
        throw SemanticError(
            "Retorno incompativel na funcao '" +
                currentSubroutine_->name() +
                "': esperado " +
                SemanticTable::typeName(
                    currentSubroutine_->type()
                    ) +
                ", recebido " +
                SemanticTable::typeName(
                    expressionType
                    ) +
                ".",
            position
            );
    }

    if (result == CompatibilityResult::Warning)
    {
        warnings_.push_back(
            {
                "Retorno da funcao '" +
                    currentSubroutine_->name() +
                    "' realiza conversao de " +
                    SemanticTable::typeName(expressionType) +
                    " para " +
                    SemanticTable::typeName(
                        currentSubroutine_->type()
                        ) +
                    " e pode causar perda de dados.",
                position
            }
            );
    }
}

// ============================================================
// FINALIZACAO DO PROGRAMA
// ============================================================

void Semantico::finalizeProgram(
    int position
    )
{
    if (assignmentTarget_.has_value())
    {
        throw SemanticError(
            "Existe uma atribuicao sem finalizacao.",
            position
            );
    }

    if (!binaryOperators_.empty())
    {
        throw SemanticError(
            "Existem operadores binarios sem reducao.",
            position
            );
    }

    if (!unaryOperators_.empty())
    {
        throw SemanticError(
            "Existem operadores unarios sem reducao.",
            position
            );
    }

    if (!subroutineCalls_.empty())
    {
        throw SemanticError(
            "Existe uma chamada de sub-rotina sem finalizacao.",
            position
            );
    }

    if (!pendingIdentifierReferences_.empty())
    {
        throw SemanticError(
            "Existem referencias de identificadores pendentes ao final do programa.",
            position
            );
    }

    expressionTypes_.clear();
}
