#include "BipIr.h"

#include "Constants.h"
#include "Lexico.h"
#include "Token.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace bip
{
namespace
{

struct SourceToken
{
    TokenId id;
    std::string lexeme;
    int position;
};

struct KnownSymbol
{
    bool isVector = false;
    bool isSupported = false;
    std::size_t size = 1;
    std::string assemblyName;

    bool isParameter = false;
    std::string type;
    std::optional<std::size_t> parameterPosition;
};

struct KnownRoutine
{
    std::string type;
    std::string scope;

    std::vector<const SymbolTableEntry *> parameters;
};

struct Target
{
    std::string name;
    ExpressionPtr index;
};

std::string normalizedName(const std::string &name)
{
    std::string result = name;

    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::toupper(character)
                );
        }
        );

    return result;
}

ExpressionPtr makeLiteral(int value)
{
    auto expression = std::make_shared<Expression>();
    expression->operation = Operation::Literal;
    expression->value = value;
    return expression;
}

ExpressionPtr makeBoolean(bool value)
{
    auto expression = std::make_shared<Expression>();
    expression->operation = Operation::BooleanLiteral;
    expression->value = value ? 1 : 0;
    return expression;
}

ExpressionPtr makeVariable(const std::string &name)
{
    auto expression = std::make_shared<Expression>();
    expression->operation = Operation::Variable;
    expression->name = name;
    return expression;
}

ExpressionPtr makeVectorElement(
    const std::string &name,
    ExpressionPtr index
    )
{
    auto expression = std::make_shared<Expression>();
    expression->operation = Operation::VectorElement;
    expression->name = name;
    expression->index = std::move(index);
    return expression;
}

ExpressionPtr makeUnary(
    Operation operation,
    ExpressionPtr operand
    )
{
    auto expression = std::make_shared<Expression>();
    expression->operation = operation;
    expression->left = std::move(operand);
    return expression;
}

ExpressionPtr makeBinary(
    Operation operation,
    ExpressionPtr left,
    ExpressionPtr right
    )
{
    auto expression = std::make_shared<Expression>();
    expression->operation = operation;
    expression->left = std::move(left);
    expression->right = std::move(right);
    return expression;
}

std::optional<long long> literalIndexValue(
    const ExpressionPtr &expression
    )
{
    if (!expression)
    {
        return std::nullopt;
    }

    if (expression->operation == Operation::Literal)
    {
        return static_cast<long long>(expression->value);
    }

    if (expression->operation == Operation::Negate)
    {
        const auto operand =
            literalIndexValue(expression->left);

        if (operand.has_value())
        {
            return -operand.value();
        }
    }

    return std::nullopt;
}

// Os niveis acompanham a ordem da gramatica v2.0.3.
// Todos os operadores binarios sao associativos a esquerda.
int precedence(TokenId id)
{
    switch (id)
    {
    case t_OU_LOGICO:
        return 1;

    case t_E_LOGICO:
        return 2;

    case t_OU_BIT_BIT:
        return 3;

    case t_OU_EXCLUSIVO_BIT_BIT:
        return 4;

    case t_E_BIT_BIT:
        return 5;

    case t_IGUAL:
    case t_DIFERENTE:
        return 6;

    case t_MAIOR_QUE:
    case t_MENOR_QUE:
    case t_MAIOR_IGUAL_QUE:
    case t_MENOR_IGUAL_QUE:
        return 7;

    case t_DESLOCA_DIREITA:
    case t_DESLOCA_ESQUERDA:
        return 8;

    case t_MAIS:
    case t_MENOS:
        return 9;

    default:
        return -1;
    }
}

Operation binaryOperation(TokenId id)
{
    switch (id)
    {
    case t_MAIS:
        return Operation::Add;

    case t_MENOS:
        return Operation::Subtract;

    case t_E_BIT_BIT:
        return Operation::BitwiseAnd;

    case t_OU_BIT_BIT:
        return Operation::BitwiseOr;

    case t_OU_EXCLUSIVO_BIT_BIT:
        return Operation::BitwiseXor;

    case t_DESLOCA_ESQUERDA:
        return Operation::ShiftLeft;

    case t_DESLOCA_DIREITA:
        return Operation::ShiftRight;

    case t_IGUAL:
        return Operation::Equal;

    case t_DIFERENTE:
        return Operation::NotEqual;

    case t_MAIOR_QUE:
        return Operation::Greater;

    case t_MENOR_QUE:
        return Operation::Less;

    case t_MAIOR_IGUAL_QUE:
        return Operation::GreaterEqual;

    case t_MENOR_IGUAL_QUE:
        return Operation::LessEqual;

    case t_E_LOGICO:
        return Operation::LogicalAnd;

    case t_OU_LOGICO:
        return Operation::LogicalOr;

    default:
        throw CodegenError(
            "Operador binario sem suporte no gerador BIP."
            );
    }
}

bool isIntegerLiteral(TokenId id)
{
    return id == t_INTEIRO_DECIMAL ||
           id == t_INTEIRO_BINARIO ||
           id == t_INTEIRO_HEXADECIMAL;
}

int parseInteger(const SourceToken &token)
{
    int base = 10;
    std::string digits = token.lexeme;

    if (token.id == t_INTEIRO_BINARIO)
    {
        base = 2;
        digits = token.lexeme.substr(2);
    }
    else if (token.id == t_INTEIRO_HEXADECIMAL)
    {
        base = 16;
        digits = token.lexeme.substr(2);
    }

    try
    {
        std::size_t consumed = 0;

        const long long parsed =
            std::stoll(digits, &consumed, base);

        if (consumed != digits.size() ||
            parsed < 0 ||
            parsed > std::numeric_limits<int>::max())
        {
            throw CodegenError(
                "Literal inteiro invalido para geracao BIP.",
                token.position
                );
        }

        return static_cast<int>(parsed);
    }
    catch (const std::invalid_argument &)
    {
        throw CodegenError(
            "Literal inteiro invalido para geracao BIP.",
            token.position
            );
    }
    catch (const std::out_of_range &)
    {
        throw CodegenError(
            "Literal inteiro fora da faixa suportada.",
            token.position
            );
    }
}

class Parser
{
public:
    Parser(
        std::vector<SourceToken> tokens,
        const AnalysisResult &analysis
        )
        : tokens_(std::move(tokens))
    {
        if (!analysis.success || !analysis.errors.empty())
        {
            throw CodegenError(
                "A geracao BIP exige analise semantica valida."
                );
        }

        // Reserva todos os identificadores originais.
        for (const SymbolTableEntry &symbol :
             analysis.symbolTable)
        {
            reservedAssemblyNames_.insert(
                normalizedName(symbol.name)
                );

            program_.reservedNames.push_back(symbol.name);
        }

        reservedAssemblyNames_.insert("_PRINCIPAL");

        // Registra as assinaturas das sub-rotinas.
        for (const SymbolTableEntry &symbol :
             analysis.symbolTable)
        {
            if (symbol.kind != "funcao")
            {
                continue;
            }

            if (symbol.scope != "global")
            {
                throw CodegenError(
                    "Sub-rotina fora do escopo global: " +
                    symbol.name
                    );
            }

            KnownRoutine routine;
            routine.type = symbol.type;
            routine.scope = "global::" + symbol.name;

            if (!routines_.emplace(
                              symbol.name,
                              std::move(routine)
                              ).second)
            {
                throw CodegenError(
                    "Sub-rotina duplicada na tabela: " +
                    symbol.name
                    );
            }
        }

        // Associa os parametros ao escopo da rotina.
        for (const SymbolTableEntry &symbol :
             analysis.symbolTable)
        {
            if (symbol.kind != "parametro")
            {
                continue;
            }

            bool associated = false;

            for (auto &entry : routines_)
            {
                if (entry.second.scope == symbol.scope)
                {
                    entry.second.parameters.push_back(&symbol);
                    associated = true;
                    break;
                }
            }

            if (!associated)
            {
                throw CodegenError(
                    "Parametro sem sub-rotina correspondente: " +
                    symbol.name
                    );
            }
        }

        // A ordem dos parametros segue a assinatura.
        for (auto &entry : routines_)
        {
            auto &parameters = entry.second.parameters;

            std::sort(
                parameters.begin(),
                parameters.end(),
                [](const SymbolTableEntry *left,
                   const SymbolTableEntry *right)
                {
                    return left->parameterPosition.value_or(0) <
                           right->parameterPosition.value_or(0);
                }
                );

            for (std::size_t i = 0;
                 i < parameters.size();
                 ++i)
            {
                if (parameters[i]->parameterPosition.value_or(0)
                    != i + 1)
                {
                    throw CodegenError(
                        "Posicao de parametro invalida na rotina '" +
                        entry.first + "'."
                        );
                }
            }
        }

        // Ordenacao deterministica para alocacao de memoria.
        std::vector<const SymbolTableEntry *> ordered;

        for (const SymbolTableEntry &symbol :
             analysis.symbolTable)
        {
            ordered.push_back(&symbol);
        }

        std::sort(
            ordered.begin(),
            ordered.end(),
            [](const SymbolTableEntry *left,
               const SymbolTableEntry *right)
            {
                if (left->scope != right->scope)
                {
                    return left->scope < right->scope;
                }

                if (left->name != right->name)
                {
                    return left->name < right->name;
                }

                return left->kind < right->kind;
            }
            );

        for (const SymbolTableEntry *symbol : ordered)
        {
            if (symbol->kind != "variavel" &&
                symbol->kind != "vetor" &&
                symbol->kind != "parametro")
            {
                continue;
            }

            KnownSymbol known;

            known.isParameter =
                symbol->kind == "parametro";

            known.isVector =
                symbol->kind == "vetor" ||
                (known.isParameter &&
                 symbol->vectorSize.has_value());

            known.isSupported = symbol->type == "inteiro";
            known.type = symbol->type;

            known.parameterPosition =
                symbol->parameterPosition;

            if (known.isVector)
            {
                known.size =
                    symbol->vectorSize.value_or(0);
            }

            if (symbol->scope == "global")
            {
                known.assemblyName = symbol->name;
            }
            else
            {
                // M3.2: utiliza o nome da rotina e
                // do identificador para gerar a memoria.
                known.assemblyName =
                    allocateLocalName(*symbol);
            }

            const bool inserted =
                symbols_[symbol->scope]
                    .emplace(symbol->name, known)
                    .second;

            if (!inserted)
            {
                throw CodegenError(
                    "Tabela de simbolos com declaracao "
                    "duplicada no mesmo escopo."
                    );
            }
        }
    }

    Program parse()
    {
        consume(
            t_PROGRAMA,
            "Palavra 'programa' esperada."
            );

        consume(
            t_IDENTIFICADOR,
            "Identificador do programa esperado."
            );

        // As definicoes aparecem antes de INICIO.
        while (check(t_PROCEDIMENTO) ||
               check(t_FUNCAO))
        {
            if (check(t_FUNCAO))
            {
                fail(
                    "Funcoes com retorno ainda nao possuem "
                    "geracao BIP nesta etapa da M3.2."
                    );
            }

            parseProcedure();
        }

        consume(
            t_INICIO,
            "Palavra 'inicio' esperada."
            );

        parseStatementList(program_.statements);

        consume(
            t_FIM,
            "Palavra 'fim' esperada."
            );

        consume(
            DOLLAR,
            "Tokens apos o fim do programa."
            );

        if (index_ != tokens_.size())
        {
            throw CodegenError(
                "Tokens inesperados apos o programa."
                );
        }

        if (program_.routines.size() != routines_.size())
        {
            throw CodegenError(
                "Definicoes de rotinas divergem "
                "da tabela de simbolos."
                );
        }

        return std::move(program_);
    }

private:
    const SourceToken &current() const
    {
        return tokens_.at(index_);
    }

    bool check(TokenId id) const
    {
        return current().id == id;
    }

    bool checkNext(TokenId id) const
    {
        return index_ + 1 < tokens_.size() &&
               tokens_[index_ + 1].id == id;
    }

    SourceToken advance()
    {
        const SourceToken token = current();
        ++index_;
        return token;
    }

    SourceToken consume(
        TokenId id,
        const std::string &message
        )
    {
        if (!check(id))
        {
            fail(message);
        }

        return advance();
    }

    [[noreturn]] void fail(
        const std::string &message
        ) const
    {
        throw CodegenError(
            message,
            current().position
            );
    }

    static bool reservedName(const std::string &name)
    {
        return normalizedName(name) == "_PRINCIPAL" ||
               name.rfind("__bip_tmp_", 0) == 0 ||
               name.rfind("__bip_vec_slot_", 0) == 0;
    }

    // ========================================================
    // M3.2 - NOMENCLATURA DE MEMORIA E ROTULOS
    // ========================================================

    // Reserva um nome legivel e exclusivo para o Assembly.
    // Em caso de colisao, acrescenta um sufixo numerico.
    std::string allocateUniqueName(
        const std::string &baseName
        )
    {
        std::string candidate = baseName;
        std::size_t suffix = 2;

        for (;;)
        {
            if (!reservedName(candidate) &&
                reservedAssemblyNames_.insert(
                                          normalizedName(candidate)
                                          ).second)
            {
                return candidate;
            }

            candidate =
                baseName + "_" +
                std::to_string(suffix++);
        }
    }

    // Mantem a nomenclatura anterior para variaveis
    // de escopos internos do programa principal.
    std::string allocateInternalLocalName()
    {
        for (;;)
        {
            const std::string name =
                "__bip_local_" +
                std::to_string(nextLocalId_++);

            if (reservedAssemblyNames_.insert(
                                          normalizedName(name)
                                          ).second)
            {
                return name;
            }
        }
    }

    // Gera nomes conforme o padrao dos slides:
    //
    // global::exibir
    // x -> EXIBIR_x
    //
    // global::exibir
    // resultado -> EXIBIR_resultado
    //
    // global::exibir::entao#1
    // resultado -> EXIBIR_ENTAO_1_resultado
    //
    // global::exibir::faca#2::entao#3
    // valor -> EXIBIR_FACA_2_ENTAO_3_valor
    std::string allocateLocalName(
        const SymbolTableEntry &symbol
        )
    {
        const std::string globalPrefix = "global::";

        if (symbol.scope.rfind(globalPrefix, 0) != 0)
        {
            return allocateInternalLocalName();
        }

        const std::string relativeScope =
            symbol.scope.substr(globalPrefix.size());

        const auto firstSeparator =
            relativeScope.find("::");

        const std::string routineName =
            relativeScope.substr(0, firstSeparator);

        // Escopos internos do principal nao pertencem
        // a uma rotina registrada na tabela.
        if (routines_.find(routineName) ==
            routines_.end())
        {
            return allocateInternalLocalName();
        }

        std::string prefix =
            normalizedName(routineName);

        if (firstSeparator != std::string::npos)
        {
            std::size_t start =
                firstSeparator + 2;

            while (start < relativeScope.size())
            {
                const auto end =
                    relativeScope.find("::", start);

                std::string part =
                    relativeScope.substr(
                        start,
                        end == std::string::npos
                            ? std::string::npos
                            : end - start
                        );

                // O caractere # pertence ao nome do
                // escopo semantico, mas nao ao Assembly.
                std::replace(
                    part.begin(),
                    part.end(),
                    '#',
                    '_'
                    );

                prefix += "_" + normalizedName(part);

                if (end == std::string::npos)
                {
                    break;
                }

                start = end + 2;
            }
        }

        const std::string baseName =
            prefix + "_" + symbol.name;

        return allocateUniqueName(baseName);
    }

    // Gera rotulos com o nome original da rotina:
    //
    // exibir -> _EXIBIR
    // dobro  -> _DOBRO
    //
    // O nome _PRINCIPAL permanece reservado.
    std::string allocateRoutineLabel(
        const std::string &routineName
        )
    {
        const std::string baseName =
            "_" + normalizedName(routineName);

        // Trata separadamente o rotulo reservado,
        // mantendo um nome significativo para a rotina.
        if (baseName == "_PRINCIPAL")
        {
            return allocateUniqueName(
                "_PRINCIPAL_2"
                );
        }

        return allocateUniqueName(baseName);
    }

    // ========================================================
    // GERENCIAMENTO DE ESCOPOS
    // ========================================================

    void enterScope(const std::string &label)
    {
        const std::string qualifiedName =
            scopeStack_.back() +
            "::" +
            label +
            "#" +
            std::to_string(nextInternalScopeIndex_++);

        scopeStack_.push_back(qualifiedName);
    }

    void leaveScope()
    {
        if (scopeStack_.size() <= 1)
        {
            throw CodegenError(
                "Tentativa de sair do escopo global."
                );
        }

        scopeStack_.pop_back();
    }

    const KnownSymbol &requireSymbol(
        const SourceToken &token
        ) const
    {
        if (reservedName(token.lexeme))
        {
            throw CodegenError(
                "Identificador reservado pelo gerador BIP: " +
                    token.lexeme,
                token.position
                );
        }

        // A busca percorre exclusivamente os ancestrais
        // do escopo atual.
        for (auto scope = scopeStack_.rbegin();
             scope != scopeStack_.rend();
             ++scope)
        {
            const auto scopeFound =
                symbols_.find(*scope);

            if (scopeFound == symbols_.end())
            {
                continue;
            }

            const auto found =
                scopeFound->second.find(token.lexeme);

            if (found == scopeFound->second.end())
            {
                continue;
            }

            if (!found->second.isSupported)
            {
                throw CodegenError(
                    "O gerador BIP suporta apenas "
                    "variaveis, vetores e parametros inteiros: " +
                        token.lexeme,
                    token.position
                    );
            }

            return found->second;
        }

        throw CodegenError(
            "Identificador '" +
                token.lexeme +
                "' nao corresponde a variavel ou vetor "
                "inteiro visivel no escopo atual.",
            token.position
            );
    }

    const KnownRoutine &requireRoutine(
        const SourceToken &identifier
        ) const
    {
        const auto found =
            routines_.find(identifier.lexeme);

        if (found == routines_.end())
        {
            throw CodegenError(
                "Sub-rotina '" +
                    identifier.lexeme +
                    "' nao encontrada na tabela de simbolos.",
                identifier.position
                );
        }

        return found->second;
    }

    // ========================================================
    // M3.2 - DEFINICAO DE PROCEDIMENTOS
    // ========================================================

    void parseProcedure()
    {
        consume(
            t_PROCEDIMENTO,
            "Palavra 'procedimento' esperada."
            );

        const SourceToken identifier = consume(
            t_IDENTIFICADOR,
            "Nome do procedimento esperado."
            );

        const KnownRoutine &known =
            requireRoutine(identifier);

        if (known.type != "void")
        {
            throw CodegenError(
                "Procedimento diverge da tabela de simbolos: " +
                    identifier.lexeme,
                identifier.position
                );
        }

        if (!definedRoutines_.insert(
                                 identifier.lexeme
                                 ).second)
        {
            throw CodegenError(
                "Procedimento duplicado no IR: " +
                    identifier.lexeme,
                identifier.position
                );
        }

        Routine routine;

        routine.name = identifier.lexeme;

        // O rotulo passa a utilizar o nome da rotina.
        routine.label =
            allocateRoutineLabel(identifier.lexeme);

        routine.scope = known.scope;
        routine.position = identifier.position;

        // O escopo da rotina nao consome o contador dos
        // escopos internos numerados.
        scopeStack_.push_back(routine.scope);

        currentRoutine_ = routine.name;

        consume(
            t_ABRE_PARENTESE,
            "Parenteses de abertura esperado no procedimento."
            );

        if (!check(t_FECHA_PARENTESE))
        {
            for (;;)
            {
                parseFormalParameter(routine, known);

                if (!check(t_VIRGULA))
                {
                    break;
                }

                advance();
            }
        }

        consume(
            t_FECHA_PARENTESE,
            "Parenteses de fechamento esperado no procedimento."
            );

        if (routine.parameters.size() !=
            known.parameters.size())
        {
            throw CodegenError(
                "Quantidade de parametros diverge da assinatura "
                "do procedimento '" + routine.name + "'.",
                identifier.position
                );
        }

        consume(
            t_INICIO,
            "Palavra 'inicio' esperada no procedimento."
            );

        parseStatementList(routine.statements);

        consume(
            t_FIMPROCEDIMENTO,
            "Palavra 'fimprocedimento' esperada."
            );

        currentRoutine_.clear();

        leaveScope();

        program_.routines.push_back(
            std::move(routine)
            );
    }

    void parseFormalParameter(
        Routine &routine,
        const KnownRoutine &known
        )
    {
        if (!check(t_INTEIRO))
        {
            fail(
                "A geracao BIP suporta apenas parametros "
                "escalares inteiros no procedimento '" +
                routine.name + "'."
                );
        }

        advance();

        const SourceToken identifier = consume(
            t_IDENTIFICADOR,
            "Identificador esperado no parametro."
            );

        const std::size_t index =
            routine.parameters.size();

        if (index >= known.parameters.size())
        {
            throw CodegenError(
                "Parametro excedente na assinatura de '" +
                    routine.name + "'.",
                identifier.position
                );
        }

        const SymbolTableEntry &entry =
            *known.parameters[index];

        if (check(t_ABRE_COLCHETE) ||
            entry.vectorSize.has_value())
        {
            throw CodegenError(
                "Passagem integral de vetores por parametro "
                "nao suportada pelo gerador BIP: " +
                    identifier.lexeme,
                identifier.position
                );
        }

        if (entry.name != identifier.lexeme ||
            entry.type != "inteiro" ||
            entry.scope != routine.scope ||
            entry.parameterPosition.value_or(0) != index + 1)
        {
            throw CodegenError(
                "Parametro '" +
                    identifier.lexeme +
                    "' diverge da tabela de simbolos.",
                identifier.position
                );
        }

        const KnownSymbol &symbol =
            requireSymbol(identifier);

        if (!symbol.isParameter ||
            symbol.isVector ||
            symbol.parameterPosition.value_or(0) != index + 1)
        {
            throw CodegenError(
                "Parametro formal invalido: " +
                    identifier.lexeme,
                identifier.position
                );
        }

        if (!declaredNames_.insert(
                               symbol.assemblyName
                               ).second)
        {
            throw CodegenError(
                "Memoria de parametro duplicada: " +
                    identifier.lexeme,
                identifier.position
                );
        }

        // Cada parametro possui uma celula independente.
        program_.declarations.push_back(
            {
                symbol.assemblyName,
                1,
                false
            }
            );

        FormalParameter parameter;

        parameter.name = entry.name;
        parameter.assemblyName = symbol.assemblyName;
        parameter.type = entry.type;
        parameter.scope = entry.scope;
        parameter.position =
            entry.parameterPosition.value();

        routine.parameters.push_back(
            std::move(parameter)
            );
    }

    // ========================================================
    // M3.2 - CHAMADAS DE PROCEDIMENTOS
    // ========================================================

    void parseCall(
        std::vector<Statement> &statements
        )
    {
        const SourceToken identifier = consume(
            t_IDENTIFICADOR,
            "Identificador esperado na chamada."
            );

        const KnownRoutine &routine =
            requireRoutine(identifier);

        if (routine.type != "void")
        {
            throw CodegenError(
                "Chamada de funcao ainda nao suportada "
                "pelo gerador BIP: " +
                    identifier.lexeme,
                identifier.position
                );
        }

        if (currentRoutine_ == identifier.lexeme)
        {
            throw CodegenError(
                "Recursao nao suportada com armazenamento "
                "estatico de parametros: " +
                    identifier.lexeme,
                identifier.position
                );
        }

        // Chamadas entre rotinas continuam fora
        // do escopo desta etapa.
        if (!currentRoutine_.empty())
        {
            throw CodegenError(
                "Chamadas entre sub-rotinas ainda nao "
                "sao suportadas pelo gerador BIP: " +
                    currentRoutine_ + " -> " +
                    identifier.lexeme,
                identifier.position
                );
        }

        Statement statement;

        statement.kind = StatementKind::Call;
        statement.routineName = identifier.lexeme;
        statement.position = identifier.position;

        consume(
            t_ABRE_PARENTESE,
            "Parenteses de abertura esperado na chamada."
            );

        // Cada argumento e uma expressao completa.
        if (!check(t_FECHA_PARENTESE))
        {
            for (;;)
            {
                statement.arguments.push_back(
                    parseExpression(1)
                    );

                if (!check(t_VIRGULA))
                {
                    break;
                }

                advance();
            }
        }

        consume(
            t_FECHA_PARENTESE,
            "Operacao de argumento sem suporte ou "
            "parenteses de fechamento ausente."
            );

        consume(
            t_PONTO_VIRGULA,
            "Ponto e virgula esperado apos a chamada."
            );

        if (statement.arguments.size() !=
            routine.parameters.size())
        {
            throw CodegenError(
                "O procedimento '" +
                    identifier.lexeme +
                    "' esperava " +
                    std::to_string(routine.parameters.size()) +
                    " argumentos, mas recebeu " +
                    std::to_string(statement.arguments.size()) +
                    ".",
                identifier.position
                );
        }

        for (const SymbolTableEntry *parameter :
             routine.parameters)
        {
            if (parameter->type != "inteiro" ||
                parameter->vectorSize.has_value())
            {
                throw CodegenError(
                    "Parametro sem suporte na chamada de '" +
                        identifier.lexeme +
                        "': " +
                        parameter->name,
                    identifier.position
                    );
            }
        }

        statements.push_back(
            std::move(statement)
            );
    }

    // ========================================================
    // COMANDOS
    // ========================================================

    void parseStatementList(
        std::vector<Statement> &statements
        )
    {
        while (!check(t_FIM) &&
               !check(t_FIMSE) &&
               !check(t_SENAO) &&
               !check(t_FIMENQUANTO) &&
               !check(t_FIMPARA) &&
               !check(t_ATE) &&
               !check(t_FIMPROCEDIMENTO))
        {
            if (check(DOLLAR))
            {
                fail(
                    "Fim inesperado da lista de comandos."
                    );
            }

            if (check(t_INTEIRO))
            {
                parseDeclaration();
            }
            else if (check(t_IDENTIFICADOR))
            {
                // IDENTIFICADOR '(' inicia chamada.
                // IDENTIFICADOR RECEBE ou '[' inicia atribuicao.
                if (checkNext(t_ABRE_PARENTESE))
                {
                    parseCall(statements);
                }
                else
                {
                    parseAssignment(statements);
                }
            }
            else if (check(t_LEIA))
            {
                parseInput(statements);
            }
            else if (check(t_MOSTRE))
            {
                parseOutput(statements);
            }
            else if (check(t_SE))
            {
                parseConditional(statements);
            }
            else if (check(t_ENQUANTO))
            {
                parseWhile(statements);
            }
            else if (check(t_PARA))
            {
                parseFor(statements);
            }
            else if (check(t_FACA))
            {
                parseRepeatUntil(statements);
            }
            else
            {
                fail(
                    "Comando ou declaracao sem suporte "
                    "no gerador BIP: '" +
                    current().lexeme +
                    "'."
                    );
            }
        }
    }

    void parseDeclaration()
    {
        consume(
            t_INTEIRO,
            "Tipo inteiro esperado."
            );

        do
        {
            const SourceToken identifier = consume(
                t_IDENTIFICADOR,
                "Identificador esperado na declaracao."
                );

            const KnownSymbol &symbol =
                requireSymbol(identifier);

            bool isVector = false;
            std::size_t size = 1;

            if (check(t_ABRE_COLCHETE))
            {
                advance();

                if (!isIntegerLiteral(current().id))
                {
                    fail(
                        "Tamanho literal inteiro esperado "
                        "na declaracao do vetor."
                        );
                }

                const SourceToken sizeToken =
                    advance();

                const int parsedSize =
                    parseInteger(sizeToken);

                if (parsedSize <= 0)
                {
                    throw CodegenError(
                        "Tamanho de vetor deve ser positivo.",
                        sizeToken.position
                        );
                }

                size =
                    static_cast<std::size_t>(parsedSize);

                isVector = true;

                consume(
                    t_FECHA_COLCHETE,
                    "Colchete de fechamento esperado."
                    );
            }

            if (isVector != symbol.isVector ||
                size != symbol.size)
            {
                throw CodegenError(
                    "Declaracao de '" +
                        identifier.lexeme +
                        "' diverge da tabela de simbolos.",
                    identifier.position
                    );
            }

            if (!declaredNames_.insert(
                                   symbol.assemblyName
                                   ).second)
            {
                throw CodegenError(
                    "Declaracao duplicada no IR: " +
                        identifier.lexeme,
                    identifier.position
                    );
            }

            program_.declarations.push_back(
                {
                    symbol.assemblyName,
                    size,
                    isVector
                }
                );

            if (!check(t_VIRGULA))
            {
                break;
            }

            advance();
        }
        while (true);

        consume(
            t_PONTO_VIRGULA,
            "Ponto e virgula esperado na declaracao."
            );
    }

    Target parseTarget()
    {
        const SourceToken identifier = consume(
            t_IDENTIFICADOR,
            "Identificador esperado."
            );

        const KnownSymbol &symbol =
            requireSymbol(identifier);

        Target target;
        target.name = symbol.assemblyName;

        if (check(t_ABRE_COLCHETE))
        {
            if (!symbol.isVector)
            {
                throw CodegenError(
                    "Identificador escalar nao aceita indice: " +
                        identifier.lexeme,
                    identifier.position
                    );
            }

            advance();

            target.index = parseExpression(1);

            consume(
                t_FECHA_COLCHETE,
                "Colchete de fechamento esperado."
                );

            const auto literal =
                literalIndexValue(target.index);

            if (literal.has_value())
            {
                const long long value =
                    literal.value();

                if (value < 0 ||
                    static_cast<std::size_t>(value) >=
                        symbol.size)
                {
                    throw CodegenError(
                        "Indice literal fora dos limites "
                        "do vetor '" +
                            identifier.lexeme +
                            "'.",
                        identifier.position
                        );
                }
            }
        }
        else if (symbol.isVector)
        {
            throw CodegenError(
                "Acesso ao vetor '" +
                    identifier.lexeme +
                    "' exige um indice.",
                identifier.position
                );
        }

        return target;
    }

    // O cabecalho de PARA usa atribuicoes sem
    // ponto e virgula, conforme a gramatica v2.0.3.
    Statement parseControlOperation()
    {
        Target target = parseTarget();

        if (target.index)
        {
            fail(
                "Variavel escalar esperada no controle do para."
                );
        }

        consume(
            t_RECEBE,
            "Operador 'recebe' esperado no laco para."
            );

        Statement statement;

        statement.kind = StatementKind::Assignment;
        statement.destination = std::move(target.name);
        statement.expression = parseExpression(1);

        return statement;
    }

    void parseAssignment(
        std::vector<Statement> &statements
        )
    {
        Target target = parseTarget();

        consume(
            t_RECEBE,
            "Operador 'recebe' esperado."
            );

        ExpressionPtr expression =
            parseExpression(1);

        consume(
            t_PONTO_VIRGULA,
            "Operacao sem suporte ou ponto e virgula "
            "ausente apos a expressao."
            );

        Statement statement;

        statement.kind = StatementKind::Assignment;
        statement.destination = std::move(target.name);
        statement.destinationIndex = std::move(target.index);
        statement.expression = std::move(expression);

        statements.push_back(
            std::move(statement)
            );
    }

    void parseInput(
        std::vector<Statement> &statements
        )
    {
        consume(
            t_LEIA,
            "Palavra 'leia' esperada."
            );

        consume(
            t_ABRE_PARENTESE,
            "Parenteses de abertura esperado em leia."
            );

        Target target = parseTarget();

        consume(
            t_FECHA_PARENTESE,
            "Parenteses de fechamento esperado em leia."
            );

        consume(
            t_PONTO_VIRGULA,
            "Ponto e virgula esperado apos leia."
            );

        Statement statement;

        statement.kind = StatementKind::Input;
        statement.destination = std::move(target.name);
        statement.destinationIndex = std::move(target.index);

        statements.push_back(
            std::move(statement)
            );
    }

    void parseOutput(
        std::vector<Statement> &statements
        )
    {
        consume(
            t_MOSTRE,
            "Palavra 'mostre' esperada."
            );

        consume(
            t_ABRE_PARENTESE,
            "Parenteses de abertura esperado em mostre."
            );

        ExpressionPtr expression;

        if (isIntegerLiteral(current().id))
        {
            const SourceToken token = advance();

            expression = makeLiteral(
                parseInteger(token)
                );
        }
        else if (check(t_IDENTIFICADOR))
        {
            Target target = parseTarget();

            if (target.index)
            {
                expression = makeVectorElement(
                    target.name,
                    std::move(target.index)
                    );
            }
            else
            {
                expression =
                    makeVariable(target.name);
            }
        }
        else
        {
            fail(
                "Saida BIP aceita literal inteiro, "
                "variavel ou elemento de vetor."
                );
        }

        consume(
            t_FECHA_PARENTESE,
            "Parenteses de fechamento esperado em mostre."
            );

        consume(
            t_PONTO_VIRGULA,
            "Ponto e virgula esperado apos mostre."
            );

        Statement statement;

        statement.kind = StatementKind::Output;
        statement.expression = std::move(expression);

        statements.push_back(
            std::move(statement)
            );
    }

    void parseConditional(
        std::vector<Statement> &statements
        )
    {
        consume(
            t_SE,
            "Palavra 'se' esperada."
            );

        Statement statement;

        statement.kind = StatementKind::Conditional;
        statement.expression = parseExpression(1);

        consume(
            t_ENTAO,
            "Palavra 'entao' esperada."
            );

        enterScope("entao");

        parseStatementList(statement.thenStatements);

        leaveScope();

        if (check(t_SENAO))
        {
            advance();

            statement.hasElse = true;

            enterScope("senao");

            parseStatementList(statement.elseStatements);

            leaveScope();
        }

        consume(
            t_FIMSE,
            "Palavra 'fimse' esperada."
            );

        statements.push_back(
            std::move(statement)
            );
    }

    void parseWhile(
        std::vector<Statement> &statements
        )
    {
        consume(
            t_ENQUANTO,
            "Palavra 'enquanto' esperada."
            );

        Statement statement;

        statement.kind = StatementKind::While;
        statement.expression = parseExpression(1);

        consume(
            t_FACA,
            "Palavra 'faca' esperada no laco enquanto."
            );

        // A acao semantica #46 entra no escopo
        // identificado pelo lexema 'faca'.
        enterScope("faca");

        parseStatementList(statement.loopStatements);

        leaveScope();

        consume(
            t_FIMENQUANTO,
            "Palavra 'fimenquanto' esperada."
            );

        statements.push_back(
            std::move(statement)
            );
    }

    void parseFor(
        std::vector<Statement> &statements
        )
    {
        consume(
            t_PARA,
            "Palavra 'para' esperada."
            );

        // A acao semantica #46 cria o escopo "para".
        enterScope("para");

        Statement statement;
        statement.kind = StatementKind::For;

        statement.initializationStatements.push_back(
            parseControlOperation()
            );

        consume(
            t_ATE,
            "Palavra 'ate' esperada no laco para."
            );

        statement.expression =
            parseExpression(1);

        consume(
            t_PASSO,
            "Palavra 'passo' esperada no laco para."
            );

        // Executa depois do corpo, apesar da posicao
        // desta operacao no codigo-fonte.
        statement.postStatements.push_back(
            parseControlOperation()
            );

        consume(
            t_FACA,
            "Palavra 'faca' esperada no laco para."
            );

        parseStatementList(statement.loopStatements);

        leaveScope();

        consume(
            t_FIMPARA,
            "Palavra 'fimpara' esperada."
            );

        statements.push_back(
            std::move(statement)
            );
    }

    void parseRepeatUntil(
        std::vector<Statement> &statements
        )
    {
        consume(
            t_FACA,
            "Palavra 'faca' esperada."
            );

        Statement statement;
        statement.kind = StatementKind::RepeatUntil;

        enterScope("faca");

        parseStatementList(statement.loopStatements);

        consume(
            t_ATE,
            "Palavra 'ate' esperada no laco faca."
            );

        // A condicao pertence ao escopo do corpo.
        statement.expression = parseExpression(1);

        leaveScope();

        consume(
            t_PONTO_VIRGULA,
            "Ponto e virgula esperado apos o laco faca."
            );

        statements.push_back(
            std::move(statement)
            );
    }

    // ========================================================
    // EXPRESSOES
    // ========================================================

    ExpressionPtr parseExpression(
        int minPrecedence
        )
    {
        ExpressionPtr left = parseUnary();

        while (true)
        {
            const int level =
                precedence(current().id);

            if (level < minPrecedence)
            {
                break;
            }

            const TokenId tokenId =
                advance().id;

            const Operation operation =
                binaryOperation(tokenId);

            ExpressionPtr right =
                parseExpression(level + 1);

            left = makeBinary(
                operation,
                std::move(left),
                std::move(right)
                );
        }

        return left;
    }

    ExpressionPtr parseUnary()
    {
        if (check(t_MENOS))
        {
            advance();

            return makeUnary(
                Operation::Negate,
                parseUnary()
                );
        }

        if (check(t_NAO_BIT_BIT))
        {
            advance();

            return makeUnary(
                Operation::BitwiseNot,
                parseUnary()
                );
        }

        if (check(t_NAO_LOGICO))
        {
            advance();

            return makeUnary(
                Operation::LogicalNot,
                parseUnary()
                );
        }

        return parsePrimary();
    }

    ExpressionPtr parsePrimary()
    {
        if (check(t_ABRE_PARENTESE))
        {
            advance();

            ExpressionPtr expression =
                parseExpression(1);

            consume(
                t_FECHA_PARENTESE,
                "Parenteses de fechamento esperado."
                );

            return expression;
        }

        if (check(t_VERDADEIRO))
        {
            advance();
            return makeBoolean(true);
        }

        if (check(t_FALSO))
        {
            advance();
            return makeBoolean(false);
        }

        if (isIntegerLiteral(current().id))
        {
            const SourceToken token =
                advance();

            return makeLiteral(
                parseInteger(token)
                );
        }

        if (check(t_IDENTIFICADOR))
        {
            // Identifica chamadas antes de procurar
            // o identificador como uma variavel.
            if (checkNext(t_ABRE_PARENTESE))
            {
                fail(
                    "Chamada de funcao sem suporte "
                    "em expressao BIP: " +
                    current().lexeme
                    );
            }

            Target target = parseTarget();

            if (target.index)
            {
                return makeVectorElement(
                    target.name,
                    std::move(target.index)
                    );
            }

            return makeVariable(target.name);
        }

        fail(
            "Operando sem suporte no gerador BIP: '" +
            current().lexeme +
            "'."
            );
    }

    std::vector<SourceToken> tokens_;
    std::size_t index_ = 0;

    Program program_;

    std::unordered_map<
        std::string,
        std::unordered_map<std::string, KnownSymbol>
        > symbols_;

    std::unordered_map<
        std::string,
        KnownRoutine
        > routines_;

    std::unordered_set<std::string>
        definedRoutines_;

    std::unordered_set<std::string>
        reservedAssemblyNames_;

    std::unordered_set<std::string>
        declaredNames_;

    std::vector<std::string> scopeStack_ = {
        "global"
    };

    std::string currentRoutine_;

    std::size_t nextInternalScopeIndex_ = 1;
    std::size_t nextLocalId_ = 1;
};

}

Program BipIrBuilder::build(
    const std::string &source,
    const AnalysisResult &analysis
    ) const
{
    if (!analysis.success || !analysis.errors.empty())
    {
        throw CodegenError(
            "Nao e permitido gerar Assembly de "
            "um programa semanticamente invalido."
            );
    }

    Lexico lexer(source.c_str());

    std::vector<SourceToken> tokens;

    for (;;)
    {
        std::unique_ptr<Token> token(
            lexer.nextToken()
            );

        if (!token)
        {
            break;
        }

        tokens.push_back(
            {
                token->getId(),
                token->getLexeme(),
                token->getPosition()
            }
            );
    }

    tokens.push_back(
        {
            DOLLAR,
            "$",
            static_cast<int>(source.size())
        }
        );

    Parser parser(
        std::move(tokens),
        analysis
        );

    return parser.parse();
}

}