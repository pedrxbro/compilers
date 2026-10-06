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

    const std::string &lexeme =
        token->getLexeme();

    const int position =
        token->getPosition();

    switch (action)
    {
        // ========================================================
        // TIPOS DAS DECLARACOES
        // ========================================================

    case 1:
        declarationProcessor_.captureType(
            lexeme,
            position
            );
        break;

        // ========================================================
        // TIPOS DOS LITERAIS
        // ========================================================

    case 2:
        pushLiteralType(
            DataType::Integer
            );
        break;

    case 3:
        pushLiteralType(
            DataType::Float
            );
        break;

    case 4:
        pushLiteralType(
            DataType::Character
            );
        break;

    case 5:
        pushLiteralType(
            DataType::Text
            );
        break;

    case 6:
        pushLiteralType(
            DataType::Logical
            );
        break;

    case 7:
        // NULO ainda não possui um tipo concreto
        pushLiteralType(
            DataType::Unknown
            );
        break;

        // ========================================================
        // OPERADORES BINARIOS
        // ========================================================

    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        // Guarda o operador para a redução da expressão.
        pushBinaryOperator(
            lexeme,
            position
            );
        break;

        // ========================================================
        // OPERADORES UNARIOS
        // ========================================================

    case 14:
    case 15:
        // Guarda o operador para a redução da expressão.
        pushUnaryOperator(
            lexeme,
            position
            );
        break;

        // ========================================================
        // REDUCOES DAS EXPRESSOES
        // ========================================================

    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
        // Calcula o tipo resultante da operação binária.
        reduceBinaryExpression(
            position
            );
        break;

    case 22:
        // Calcula o tipo resultante da operação unária.
        reduceUnaryExpression(
            position
            );
        break;

        // ========================================================
        // DECLARACOES
        // ========================================================

    case 23:
        declarationProcessor_
            .beginVariableDeclaration();
        break;

    case 24:
        declarationProcessor_
            .captureIdentifier(
                lexeme
                );
        break;

    case 25:
        declarationProcessor_
            .declareSimple(
                position
                );
        break;

    case 26:
        declarationProcessor_
            .captureVectorSize(
                lexeme,
                position
                );
        break;

    case 27:
        declarationProcessor_
            .declareVector(
                position
                );
        break;

    case 28:
        declarationProcessor_
            .beginParameterDeclaration();
        break;

        // ========================================================
        // SUB-ROTINAS
        // ========================================================

    case 29:
        declarationProcessor_
            .beginProcedure(
                lexeme,
                position
                );

        // Guarda a sub-rotina atualmente sendo processada.
        captureCurrentSubroutine(
            lexeme,
            position
            );
        break;

    case 30:
        declarationProcessor_
            .beginFunction(
                lexeme,
                position
                );

        // Guarda a sub-rotina atualmente sendo processada.
        captureCurrentSubroutine(
            lexeme,
            position
            );
        break;

    case 31:
        // Verifica semanticamente o valor retornado.
        completeReturn(
            position
            );
        break;

    case 32:
        declarationProcessor_
            .endSubroutine(
                position
                );

        // Não há mais uma sub-rotina sendo processada.
        currentSubroutine_ = nullptr;
        break;

        // ========================================================
        // CHAMADAS DE SUB-ROTINAS
        // ========================================================

    case 33:
        // Resolve e guarda a sub-rotina chamada.
        beginSubroutineCall(
            lexeme,
            position
            );
        break;

    case 34:
        // O tipo do argumento já foi calculado.
        // A verificação dos parâmetros será feita posteriormente.
        discardExpressionResult(
            position
            );
        break;

    case 35:
        // Marca o fim da lista de argumentos.
        break;

    case 36:
        // Finaliza uma chamada usada como comando.
        completeCallAsStatement(
            position
            );
        break;

    case 37:
        // Finaliza uma chamada usada dentro de uma expressão.
        completeCallAsExpression(
            position
            );
        break;

        // ========================================================
        // IDENTIFICADORES E VETORES
        // ========================================================

    case 38:
        // Resolve e guarda uma referência ao identificador.
        captureIdentifierReference(
            lexeme,
            position
            );
        break;

    case 39:
        // Verifica o tipo do índice e marca o vetor como indexado.
        validateVectorIndex(
            position
            );
        break;

        // ========================================================
        // ATRIBUICOES
        // ========================================================

    case 40:
        // Guarda o identificador como destino da atribuição.
        captureAssignmentTarget(
            position
            );
        break;

    case 41:
        // Verifica o tipo da expressão e finaliza a atribuição.
        completeAssignment(
            position
            );
        break;

        // ========================================================
        // ENTRADA E SAIDA
        // ========================================================

    case 42:
        // Marca o destino da entrada como inicializado.
        initializeInputTarget(
            position
            );
        break;

    case 43:
        // Usa a referência de identificador pendente.
        usePendingIdentifier(
            position
            );
        break;

        // ========================================================
        // OPERANDOS
        // ========================================================

    case 44:
        // Obtém o tipo do identificador e adiciona à expressão.
        pushPendingIdentifierValue(
            position
            );
        break;

        // ========================================================
        // CONDICOES
        // ========================================================

    case 45:
        // Garante que a condição produz um valor lógico.
        validateCondition(
            position
            );
        break;

        // ========================================================
        // ESCOPOS
        // ========================================================

    case 46:
        // Entra em um novo escopo interno.
        declarationProcessor_
            .enterInternalScope(
                lexeme
                );
        break;

    case 47:
        // Retorna ao escopo pai.
        declarationProcessor_
            .leaveInternalScope(
                position
                );
        break;

        // ========================================================
        // FIM DO PROGRAMA
        // ========================================================

    case 48:
        // Verifica se não existem estruturas sem finalização.
        finalizeProgram(
            position
            );
        break;

    default:
        break;
    }
}

// Retorna o gerenciador de escopos para consulta.
const ScopeManager &Semantico::scopeManager() const
{
    return declarationProcessor_
        .scopeManager();
}

// Retorna os avisos semânticos acumulados.
std::vector<std::string> Semantico::warnings() const
{
    std::vector<std::string> result =
        warnings_;

    // Obtém todos os símbolos declarados na árvore de escopos.
    std::vector<const Symbol *> symbols =
        declarationProcessor_
            .scopeManager()
            .allSymbols();

    // Ordena primeiro pelo escopo e depois pelo nome.
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

    // Gera avisos para símbolos que nunca foram utilizados.
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
            "Identificador '" +
            symbol->name() +
            "' declarado no escopo '" +
            symbol->scope() +
            "' e nunca usado."
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

    // Procura no escopo atual e em seus escopos pais.
    Symbol *symbol =
        scopeManager.findVisible(
            name
            );

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

// Resolve um identificador e guarda sua referência para uso posterior.
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
                position,
                false
            }
            );
}

// Remove e retorna a referência pendente mais recente.
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

    pendingIdentifierReferences_
        .pop_back();

    return reference;
}

// Retorna a referência pendente sem removê-la.
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

// Verifica se um identificador pode ser utilizado como valor.
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

    // Sub-rotinas precisam ser chamadas para produzir valores.
    if (reference.symbol->kind() ==
        SymbolKind::Function)
    {
        throw SemanticError(
            "Sub-rotina '" +
                reference.name +
                "' deve ser chamada para produzir um valor.",
            position
            );
    }

    // Um vetor precisa ser indexado para produzir um valor.
    if (reference.symbol
            ->vectorSize()
            .has_value() &&
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

// Verifica se um identificador pode ser usado como destino.
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

    // Sub-rotinas não podem receber atribuições.
    if (reference.symbol->kind() ==
        SymbolKind::Function)
    {
        throw SemanticError(
            "Sub-rotina '" +
                reference.name +
                "' nao pode receber atribuicao.",
            position
            );
    }

    // Um vetor precisa ser indexado para receber atribuição.
    if (reference.symbol
            ->vectorSize()
            .has_value() &&
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

// Marca uma referência como utilizada e verifica sua inicialização.
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

    // Gera aviso quando uma variável é usada antes de ser inicializada.
    if (reference.symbol->kind() !=
            SymbolKind::Function &&
        !reference.symbol->isInitialized())
    {
        warnings_.push_back(
            "Identificador '" +
            reference.name +
            "' usado antes da inicializacao no escopo '" +
            reference.symbol->scope() +
            "' (posicao " +
            std::to_string(
                reference.position
                ) +
            ")."
            );
    }

    reference.symbol->markUsed();
}

// Usa a referência pendente como um valor.
void Semantico::usePendingIdentifier(
    int position
    )
{
    const IdentifierReference reference =
        popIdentifierReference(
            position
            );

    validateValueReference(
        reference,
        position
        );

    markReferenceUsed(
        reference
        );
}

// Obtém o tipo de uma referência e o coloca na expressão atual.
void Semantico::pushPendingIdentifierValue(
    int position
    )
{
    const IdentifierReference reference =
        popIdentifierReference(
            position
            );

    validateValueReference(
        reference,
        position
        );

    markReferenceUsed(
        reference
        );

    expressionTypes_.push_back(
        reference.symbol->type()
        );
}

// Define uma referência como destino da atribuição.
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
        popIdentifierReference(
            position
            );

    validateAssignmentReference(
        reference,
        position
        );

    assignmentTarget_ =
        reference;
}

// Verifica a compatibilidade e finaliza uma atribuição.
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

    // Obtém o tipo produzido pela expressão.
    const DataType expressionType =
        popExpressionType(
            position
            );

    const DataType destinationType =
        assignmentTarget_
            ->symbol
            ->type();

    // Verifica se os tipos podem participar da atribuição.
    const CompatibilityResult result =
        SemanticTable::atribType(
            destinationType,
            expressionType
            );

    if (result ==
        CompatibilityResult::Error)
    {
        throw SemanticError(
            "Atribuicao incompativel: identificador '" +
                assignmentTarget_->name +
                "' e do tipo " +
                SemanticTable::typeName(
                    destinationType
                    ) +
                ", mas a expressao resulta em " +
                SemanticTable::typeName(
                    expressionType
                    ) +
                ".",
            position
            );
    }

    if (result ==
        CompatibilityResult::Warning)
    {
        warnings_.push_back(
            "Atribuicao com conversao de " +
            SemanticTable::typeName(
                expressionType
                ) +
            " para " +
            SemanticTable::typeName(
                destinationType
                ) +
            " no identificador '" +
            assignmentTarget_->name +
            "' pode causar perda de dados."
            );
    }

    // Uma atribuição válida inicializa o destino.
    assignmentTarget_
        ->symbol
        ->markInitialized();

    // Libera o destino pendente.
    assignmentTarget_.reset();
}

// Marca como inicializado o identificador usado como entrada.
void Semantico::initializeInputTarget(
    int position
    )
{
    IdentifierReference reference =
        popIdentifierReference(
            position
            );

    validateAssignmentReference(
        reference,
        position
        );

    reference.symbol
        ->markInitialized();
}

// Verifica se o índice de um vetor é válido.
void Semantico::validateVectorIndex(
    int position
    )
{
    // O índice precisa resultar em inteiro.
    const DataType indexType =
        popExpressionType(
            position
            );

    // Obtém a referência do vetor que está sendo indexado.
    IdentifierReference &reference =
        pendingIdentifierReference(
            position
            );

    if (reference.symbol == nullptr)
    {
        throw SemanticError(
            "Referencia de vetor invalida.",
            position
            );
    }

    if (!reference.symbol
             ->vectorSize()
             .has_value())
    {
        throw SemanticError(
            "Identificador '" +
                reference.name +
                "' nao e um vetor.",
            position
            );
    }

    if (indexType !=
        DataType::Integer)
    {
        throw SemanticError(
            "Indice do vetor '" +
                reference.name +
                "' deve ser INTEIRO, mas a expressao resulta em " +
                SemanticTable::typeName(
                    indexType
                    ) +
                ".",
            position
            );
    }

    // A referência agora representa um elemento do vetor.
    reference.indexed = true;
}

// Adiciona o tipo de um literal à pilha de tipos.
void Semantico::pushLiteralType(
    DataType type
    )
{
    expressionTypes_
        .push_back(
            type
            );
}

// Remove e retorna o tipo da expressão mais recente.
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

    const DataType type =
        expressionTypes_.back();

    expressionTypes_.pop_back();

    return type;
}

// Converte o lexema em um operador binário.
BinaryOperator Semantico::binaryOperatorFromLexeme(
    const std::string &lexeme,
    int position
    ) const
{
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

    if (lexeme == "e")
    {
        return BinaryOperator::LogicalAnd;
    }

    if (lexeme == "ou")
    {
        return BinaryOperator::LogicalOr;
    }

    throw SemanticError(
        "Operador binario desconhecido: '" +
            lexeme +
            "'.",
        position
        );
}

// Converte o lexema em um operador unário.
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

    throw SemanticError(
        "Operador unario desconhecido: '" +
            lexeme +
            "'.",
        position
        );
}

// Adiciona um operador binário à pilha de operadores.
void Semantico::pushBinaryOperator(
    const std::string &lexeme,
    int position
    )
{
    binaryOperators_
        .push_back(
            binaryOperatorFromLexeme(
                lexeme,
                position
                )
            );
}

// Adiciona um operador unário à pilha de operadores.
void Semantico::pushUnaryOperator(
    const std::string &lexeme,
    int position
    )
{
    unaryOperators_
        .push_back(
            unaryOperatorFromLexeme(
                lexeme,
                position
                )
            );
}

// Reduz uma expressão binária para um único tipo.
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

    // A pilha fornece primeiro o operando da direita.
    const DataType right =
        popExpressionType(
            position
            );

    const DataType left =
        popExpressionType(
            position
            );

    const BinaryOperator operation =
        binaryOperators_.back();

    binaryOperators_.pop_back();

    // Consulta a tabela semântica para obter o tipo resultante.
    const DataType result =
        SemanticTable::resultType(
            left,
            right,
            operation
            );

    if (result ==
        DataType::Unknown)
    {
        throw SemanticError(
            "Operacao '" +
                SemanticTable::operatorName(
                    operation
                    ) +
                "' incompativel entre " +
                SemanticTable::typeName(
                    left
                    ) +
                " e " +
                SemanticTable::typeName(
                    right
                    ) +
                ".",
            position
            );
    }

    // Coloca o resultado da expressão de volta na pilha.
    expressionTypes_
        .push_back(
            result
            );
}

// Reduz uma expressão unária para um único tipo.
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
        popExpressionType(
            position
            );

    const UnaryOperator operation =
        unaryOperators_.back();

    unaryOperators_.pop_back();

    // Consulta a tabela semântica para obter o tipo resultante.
    const DataType result =
        SemanticTable::resultType(
            operand,
            operation
            );

    if (result ==
        DataType::Unknown)
    {
        throw SemanticError(
            "Operacao unaria '" +
                SemanticTable::operatorName(
                    operation
                    ) +
                "' incompativel com o tipo " +
                SemanticTable::typeName(
                    operand
                    ) +
                ".",
            position
            );
    }

    // Coloca o resultado da expressão de volta na pilha.
    expressionTypes_
        .push_back(
            result
            );
}

// Verifica se uma expressão pode ser usada como condição.
void Semantico::validateCondition(
    int position
    )
{
    const DataType conditionType =
        popExpressionType(
            position
            );

    if (conditionType !=
        DataType::Logical)
    {
        throw SemanticError(
            "Condicao deve resultar em LOGICO, mas a expressao resulta em " +
                SemanticTable::typeName(
                    conditionType
                    ) +
                ".",
            position
            );
    }
}

// Descarta o resultado de uma expressão que não será utilizado.
void Semantico::discardExpressionResult(
    int position
    )
{
    popExpressionType(
        position
        );
}

// Inicia o processamento de uma chamada de sub-rotina.
void Semantico::beginSubroutineCall(
    const std::string &name,
    int position
    )
{
    // Resolve a sub-rotina no escopo atual.
    Symbol *symbol =
        resolveIdentifier(
            name,
            position
            );

    if (symbol->kind() !=
        SymbolKind::Function)
    {
        throw SemanticError(
            "Identificador '" +
                name +
                "' nao e uma sub-rotina.",
            position
            );
    }

    symbol->markUsed();

    // Guarda a chamada até que seus argumentos sejam processados.
    subroutineCalls_
        .push_back(
            symbol
            );
}

// Remove e retorna a chamada de sub-rotina mais recente.
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

    Symbol *symbol =
        subroutineCalls_.back();

    subroutineCalls_.pop_back();

    return symbol;
}

// Finaliza uma chamada usada como comando.
void Semantico::completeCallAsStatement(
    int position
    )
{
    Symbol *symbol =
        popSubroutineCall(
            position
            );

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Chamada de sub-rotina invalida.",
            position
            );
    }

    // O valor retornado pela função não é utilizado.
}

// Finaliza uma chamada usada dentro de uma expressão.
void Semantico::completeCallAsExpression(
    int position
    )
{
    Symbol *symbol =
        popSubroutineCall(
            position
            );

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Chamada de funcao invalida.",
            position
            );
    }

    if (symbol->type() ==
        DataType::Void)
    {
        throw SemanticError(
            "Procedimento '" +
                symbol->name() +
                "' nao pode participar de uma expressao.",
            position
            );
    }

    // O tipo retornado passa a fazer parte da expressão.
    expressionTypes_
        .push_back(
            symbol->type()
            );
}

// Localiza e guarda a sub-rotina que está sendo processada.
void Semantico::captureCurrentSubroutine(
    const std::string &name,
    int position
    )
{
    Symbol *symbol =
        declarationProcessor_
            .scopeManager()
            .globalScope()
            .findLocal(
                name
                );

    if (symbol == nullptr)
    {
        throw SemanticError(
            "Nao foi possivel localizar a sub-rotina '" +
                name +
                "' apos sua declaracao.",
            position
            );
    }

    currentSubroutine_ =
        symbol;
}

// Verifica semanticamente o retorno de uma função.
void Semantico::completeReturn(
    int position
    )
{
    const DataType expressionType =
        popExpressionType(
            position
            );

    if (currentSubroutine_ == nullptr)
    {
        throw SemanticError(
            "Comando retorne utilizado fora de uma funcao.",
            position
            );
    }

    if (currentSubroutine_->type() ==
        DataType::Void)
    {
        throw SemanticError(
            "Procedimento nao pode retornar uma expressao.",
            position
            );
    }

    // Compara o tipo retornado com o tipo declarado pela função.
    const CompatibilityResult result =
        SemanticTable::atribType(
            currentSubroutine_->type(),
            expressionType
            );

    if (result ==
        CompatibilityResult::Error)
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

    if (result ==
        CompatibilityResult::Warning)
    {
        warnings_.push_back(
            "Retorno da funcao '" +
            currentSubroutine_->name() +
            "' realiza conversao de " +
            SemanticTable::typeName(
                expressionType
                ) +
            " para " +
            SemanticTable::typeName(
                currentSubroutine_->type()
                ) +
            " e pode causar perda de dados."
            );
    }
}

// Verifica se todas as estruturas semânticas foram finalizadas.
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

    // Remove tipos que podem ter sido produzidos por literais
    // usados diretamente em comandos de saída.
    expressionTypes_.clear();
}
