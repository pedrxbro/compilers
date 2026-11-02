
#include "BipIr.h"

#include "Constants.h"
#include "Lexico.h"
#include "Token.h"

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
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

ExpressionPtr makeLiteral(int value)
{
    auto expression = std::make_shared<Expression>();
    expression->operation = Operation::Literal;
    expression->value = value;
    return expression;
}

ExpressionPtr makeVariable(const std::string &name)
{
    auto expression = std::make_shared<Expression>();
    expression->operation = Operation::Variable;
    expression->name = name;
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

int precedence(TokenId id)
{
    switch (id)
    {
    case t_OU_BIT_BIT:
        return 1;

    case t_OU_EXCLUSIVO_BIT_BIT:
        return 2;

    case t_E_BIT_BIT:
        return 3;

    case t_DESLOCA_DIREITA:
    case t_DESLOCA_ESQUERDA:
        return 4;

    case t_MAIS:
    case t_MENOS:
        return 5;

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

        for (const auto &symbol : analysis.symbolTable)
        {
            if (symbol.scope == "global" &&
                symbol.kind == "variavel" &&
                symbol.type == "inteiro" &&
                !symbol.vectorSize.has_value())
            {
                validVariables_.insert(symbol.name);
            }
        }
    }

    Program parse()
    {
        consume(t_PROGRAMA, "Palavra 'programa' esperada.");
        consume(
            t_IDENTIFICADOR,
            "Identificador do programa esperado."
            );

        if (!check(t_INICIO))
        {
            fail(
                "Procedimentos e funcoes ainda nao possuem "
                "geracao de Assembly BIP."
                );
        }

        consume(t_INICIO, "Palavra 'inicio' esperada.");

        while (!check(t_FIM))
        {
            if (check(DOLLAR))
            {
                fail("Fim do programa nao encontrado.");
            }

            if (check(t_INTEIRO))
            {
                parseDeclaration();
            }
            else if (check(t_IDENTIFICADOR))
            {
                parseAssignment();
            }
            else
            {
                fail(
                    "Comando ou declaracao sem suporte "
                    "no nucleo BIP: '" +
                    current().lexeme + "'."
                    );
            }
        }

        consume(t_FIM, "Palavra 'fim' esperada.");
        consume(DOLLAR, "Tokens apos o fim do programa.");

        if (index_ != tokens_.size())
        {
            throw CodegenError(
                "Tokens inesperados apos o programa."
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

    void requireVariable(
        const SourceToken &token
        ) const
    {
        if (validVariables_.count(token.lexeme) == 0)
        {
            throw CodegenError(
                "Identificador '" + token.lexeme +
                    "' nao corresponde a uma variavel "
                    "inteira escalar global validada.",
                token.position
                );
        }

        if (token.lexeme == "_PRINCIPAL" ||
            token.lexeme.rfind("__bip_tmp_", 0) == 0)
        {
            throw CodegenError(
                "Identificador reservado pelo gerador BIP: " +
                    token.lexeme,
                token.position
                );
        }
    }

    void parseDeclaration()
    {
        consume(t_INTEIRO, "Tipo inteiro esperado.");

        do
        {
            const SourceToken identifier = consume(
                t_IDENTIFICADOR,
                "Identificador esperado na declaracao."
                );

            requireVariable(identifier);

            if (check(t_ABRE_COLCHETE))
            {
                fail(
                    "Declaracoes de vetores ainda nao "
                    "sao suportadas neste nucleo."
                    );
            }

            if (!declaredVariables_.insert(
                                       identifier.lexeme
                                       ).second)
            {
                throw CodegenError(
                    "Declaracao duplicada no IR: " +
                        identifier.lexeme,
                    identifier.position
                    );
            }

            program_.variables.push_back(identifier.lexeme);

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

    void parseAssignment()
    {
        const SourceToken destination = consume(
            t_IDENTIFICADOR,
            "Destino da atribuicao esperado."
            );

        requireVariable(destination);

        if (check(t_ABRE_COLCHETE))
        {
            fail(
                "Atribuicao em posicao de vetor "
                "ainda nao possui suporte BIP."
                );
        }

        consume(
            t_RECEBE,
            "Operador 'recebe' esperado."
            );

        ExpressionPtr expression = parseExpression(1);

        consume(
            t_PONTO_VIRGULA,
            "Operacao sem suporte ou ponto e virgula ausente "
            "apos a expressao."
            );

        program_.assignments.push_back(
            {
                destination.lexeme,
                std::move(expression)
            }
            );
    }

    ExpressionPtr parseExpression(int minPrecedence)
    {
        ExpressionPtr left = parseUnary();

        while (true)
        {
            const int level = precedence(current().id);

            if (level < minPrecedence)
            {
                break;
            }

            const TokenId tokenId = advance().id;
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

        if (isIntegerLiteral(current().id))
        {
            const SourceToken token = advance();

            return makeLiteral(
                parseInteger(token)
                );
        }

        if (check(t_IDENTIFICADOR))
        {
            const SourceToken token = advance();

            requireVariable(token);

            if (check(t_ABRE_COLCHETE) ||
                check(t_ABRE_PARENTESE))
            {
                fail(
                    "Acesso a vetor ou chamada de funcao "
                    "sem suporte em expressao BIP."
                    );
            }

            return makeVariable(token.lexeme);
        }

        fail(
            "Operando sem suporte no gerador BIP: '" +
            current().lexeme + "'."
            );
    }

    std::vector<SourceToken> tokens_;
    std::size_t index_ = 0;

    Program program_;

    std::unordered_set<std::string> validVariables_;
    std::unordered_set<std::string> declaredVariables_;
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
        std::unique_ptr<Token> token(lexer.nextToken());

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

    Parser parser(std::move(tokens), analysis);

    return parser.parse();
}

}
