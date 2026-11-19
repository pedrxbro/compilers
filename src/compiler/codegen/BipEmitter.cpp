#include "BipIr.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace bip
{
namespace
{

constexpr int MIN_IMMEDIATE = -1024;
constexpr int MAX_IMMEDIATE = 1023;
constexpr int MIN_WORD = -32768;
constexpr int MAX_WORD = 32767;

constexpr std::size_t MAX_DATA_CELLS = 2048;

struct Temporary
{
    std::string name;
    bool inUse = false;
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

bool fitsImmediate(int value)
{
    return value >= MIN_IMMEDIATE &&
           value <= MAX_IMMEDIATE;
}

bool fitsWord(int value)
{
    return value >= MIN_WORD &&
           value <= MAX_WORD;
}

bool reservedName(const std::string &name)
{
    return normalizedName(name) == "_PRINCIPAL" ||
           name.rfind("__bip_tmp_", 0) == 0 ||
           name.rfind("__bip_vec_slot_", 0) == 0;
}

bool isRelational(Operation operation)
{
    switch (operation)
    {
    case Operation::Equal:
    case Operation::NotEqual:
    case Operation::Greater:
    case Operation::Less:
    case Operation::GreaterEqual:
    case Operation::LessEqual:
        return true;

    default:
        return false;
    }
}

const char *comparisonOpcode(Operation operation)
{
    switch (operation)
    {
    case Operation::Equal:
        return "BEQ";

    case Operation::NotEqual:
        return "BNE";

    case Operation::Greater:
        return "BGT";

    case Operation::Less:
        return "BLT";

    case Operation::GreaterEqual:
        return "BGE";

    case Operation::LessEqual:
        return "BLE";

    default:
        throw CodegenError(
            "Operador relacional desconhecido."
            );
    }
}

class EmitterState
{
public:
    explicit EmitterState(const Program &program)
        : program_(program)
    {
        // Os nomes de origem tambem ficam reservados,
        // inclusive quando sao substituidos por nomes
        // exclusivos de memoria para escopos internos.
        for (const std::string &name :
             program_.reservedNames)
        {
            occupiedNames_.insert(
                normalizedName(name)
                );
        }

        occupiedNames_.insert("_PRINCIPAL");

        std::unordered_set<std::string>
            declarationLabels;

        for (const Declaration &declaration :
             program_.declarations)
        {
            if (declaration.name.empty() ||
                reservedName(declaration.name))
            {
                throw CodegenError(
                    "Identificador invalido ou reservado: " +
                    declaration.name
                    );
            }

            if (declaration.size == 0 ||
                (!declaration.isVector &&
                 declaration.size != 1))
            {
                throw CodegenError(
                    "Declaracao de memoria BIP invalida: " +
                    declaration.name
                    );
            }

            if (!declarationLabels.insert(
                                      normalizedName(declaration.name)
                                      ).second)
            {
                throw CodegenError(
                    "Rotulo de memoria duplicado no IR: " +
                    declaration.name
                    );
            }

            if (!declarations_.emplace(
                                  declaration.name,
                                  declaration
                                  ).second)
            {
                throw CodegenError(
                    "Identificador duplicado no IR: " +
                    declaration.name
                    );
            }

            if (declaration.size >
                MAX_DATA_CELLS - dataCells_)
            {
                throw CodegenError(
                    "Declaracoes excedem a memoria "
                    "de dados enderecavel do BIP."
                    );
            }

            dataCells_ += declaration.size;

            occupiedNames_.insert(
                normalizedName(declaration.name)
                );
        }
    }

    std::string generate()
    {
        emitStatementList(program_.statements);

        instruction("HLT", "0");

        if (temporaries_.size() >
            MAX_DATA_CELLS - dataCells_)
        {
            throw CodegenError(
                "Variaveis e temporarios excedem "
                "a memoria de dados BIP."
                );
        }

        for (const Temporary &temporary : temporaries_)
        {
            if (temporary.inUse)
            {
                throw CodegenError(
                    "Temporario ainda ocupado ao "
                    "finalizar a geracao."
                    );
            }
        }

        std::ostringstream output;

        output << ".data\n";

        // Mantem o formato contiguo de vetores da M2.2.
        for (const Declaration &declaration :
             program_.declarations)
        {
            output << declaration.name << ": 0";

            if (declaration.isVector)
            {
                for (std::size_t index = 1;
                     index < declaration.size;
                     ++index)
                {
                    output << ", 0";
                }
            }

            output << '\n';
        }

        for (const Temporary &temporary : temporaries_)
        {
            output << temporary.name << ": 0\n";
        }

        output << ".text\n";
        output << "_PRINCIPAL:\n";

        for (const std::string &line : instructions_)
        {
            if (!line.empty() && line.back() == ':')
            {
                output << line << '\n';
            }
            else
            {
                output << "    " << line << '\n';
            }
        }

        return output.str();
    }

private:
    const Declaration &requireDeclaration(
        const std::string &name
        ) const
    {
        const auto found =
            declarations_.find(name);

        if (found == declarations_.end())
        {
            throw CodegenError(
                "Identificador nao declarado no IR: " +
                name
                );
        }

        return found->second;
    }

    void requireScalar(
        const std::string &name
        ) const
    {
        const Declaration &declaration =
            requireDeclaration(name);

        if (declaration.isVector)
        {
            throw CodegenError(
                "Variavel escalar esperada: " + name
                );
        }
    }

    void requireVector(
        const std::string &name
        ) const
    {
        const Declaration &declaration =
            requireDeclaration(name);

        if (!declaration.isVector)
        {
            throw CodegenError(
                "Vetor esperado: " + name
                );
        }
    }

    void instruction(
        const std::string &opcode,
        const std::string &operand
        )
    {
        instructions_.push_back(
            opcode + " " + operand
            );
    }

    void instruction(
        const std::string &opcode,
        int operand
        )
    {
        instruction(
            opcode,
            std::to_string(operand)
            );
    }

    void label(const std::string &name)
    {
        if (name.empty())
        {
            throw CodegenError(
                "Rotulo de desvio vazio."
                );
        }

        instructions_.push_back(name + ":");
    }

    // O estado e exclusivo desta compilacao.
    // Nunca utiliza um rotulo global fixo de controle.
    std::string newLabel()
    {
        for (;;)
        {
            const std::string candidate =
                "__bip_label_" +
                std::to_string(nextLabelId_++);

            if (occupiedNames_.insert(
                                  normalizedName(candidate)
                                  ).second)
            {
                return candidate;
            }
        }
    }

    void loadLiteral(int value)
    {
        if (!fitsWord(value))
        {
            throw CodegenError(
                "Literal fora da faixa de 16 bits do BIP."
                );
        }

        if (!fitsImmediate(value))
        {
            throw CodegenError(
                "Literal nao representavel por LDI "
                "neste gerador BIP: " +
                std::to_string(value)
                );
        }

        instruction("LDI", value);
    }

    std::string acquireTemporary()
    {
        for (Temporary &temporary : temporaries_)
        {
            if (!temporary.inUse)
            {
                temporary.inUse = true;
                return temporary.name;
            }
        }

        std::string name;

        for (;;)
        {
            name = "temp" +
                   std::to_string(nextTemporaryId_++);

            if (occupiedNames_.insert(
                                  normalizedName(name)
                                  ).second)
            {
                break;
            }
        }

        temporaries_.push_back(
            {name, true}
            );

        return name;
    }

    void releaseTemporary(const std::string &name)
    {
        for (Temporary &temporary : temporaries_)
        {
            if (temporary.name == name)
            {
                if (!temporary.inUse)
                {
                    throw CodegenError(
                        "Liberacao duplicada de temporario."
                        );
                }

                temporary.inUse = false;
                return;
            }
        }

        throw CodegenError(
            "Temporario desconhecido: " + name
            );
    }

    static std::string memoryOpcode(
        Operation operation
        )
    {
        switch (operation)
        {
        case Operation::Add:
            return "ADD";

        case Operation::Subtract:
            return "SUB";

        case Operation::BitwiseAnd:
            return "AND";

        case Operation::BitwiseOr:
            return "OR";

        case Operation::BitwiseXor:
            return "XOR";

        default:
            throw CodegenError(
                "Operacao sem instrucao BIP de memoria."
                );
        }
    }

    static std::string immediateOpcode(
        Operation operation
        )
    {
        switch (operation)
        {
        case Operation::Add:
            return "ADDI";

        case Operation::Subtract:
            return "SUBI";

        case Operation::BitwiseAnd:
            return "ANDI";

        case Operation::BitwiseOr:
            return "ORI";

        case Operation::BitwiseXor:
            return "XORI";

        default:
            throw CodegenError(
                "Operacao sem instrucao BIP imediata."
                );
        }
    }

    void emitUnary(const Expression &expression)
    {
        if (!expression.left)
        {
            throw CodegenError(
                "Operacao unaria sem operando."
                );
        }

        if (expression.operation ==
            Operation::BitwiseNot)
        {
            emitExpression(expression.left);
            instruction("NOT", "0");
            return;
        }

        if (expression.operation == Operation::Negate)
        {
            if (expression.left->operation ==
                Operation::Literal)
            {
                const long long value =
                    -static_cast<long long>(
                        expression.left->value
                        );

                if (value >= MIN_IMMEDIATE &&
                    value <= MAX_IMMEDIATE)
                {
                    loadLiteral(
                        static_cast<int>(value)
                        );

                    return;
                }
            }

            const std::string temporary =
                acquireTemporary();

            emitExpression(expression.left);

            instruction("STO", temporary);
            instruction("LDI", "0");
            instruction("SUB", temporary);

            releaseTemporary(temporary);
            return;
        }

        throw CodegenError(
            "Operador unario sem suporte BIP."
            );
    }

    void emitShift(const Expression &expression)
    {
        if (!expression.left || !expression.right)
        {
            throw CodegenError(
                "Deslocamento sem operandos."
                );
        }

        if (expression.right->operation !=
            Operation::Literal)
        {
            throw CodegenError(
                "Deslocamento com quantidade dinamica "
                "ainda nao possui suporte."
                );
        }

        const int amount = expression.right->value;

        if (amount < 0 || amount > 15)
        {
            throw CodegenError(
                "Quantidade de deslocamento deve "
                "estar entre 0 e 15 bits."
                );
        }

        emitExpression(expression.left);

        instruction(
            expression.operation ==
                    Operation::ShiftLeft
                ? "SLL"
                : "SRL",
            amount
            );
    }

    void emitBinary(const Expression &expression)
    {
        if (!expression.left || !expression.right)
        {
            throw CodegenError(
                "Operacao binaria sem operandos."
                );
        }

        if (expression.operation ==
                Operation::ShiftLeft ||
            expression.operation ==
                Operation::ShiftRight)
        {
            emitShift(expression);
            return;
        }

        const Expression &right =
            *expression.right;

        if (right.operation == Operation::Literal &&
            fitsImmediate(right.value))
        {
            emitExpression(expression.left);

            instruction(
                immediateOpcode(expression.operation),
                right.value
                );

            return;
        }

        if (right.operation == Operation::Variable)
        {
            requireScalar(right.name);

            emitExpression(expression.left);

            instruction(
                memoryOpcode(expression.operation),
                right.name
                );

            return;
        }

        const std::string leftTemporary =
            acquireTemporary();

        emitExpression(expression.left);
        instruction("STO", leftTemporary);

        emitExpression(expression.right);

        const std::string rightTemporary =
            acquireTemporary();

        instruction("STO", rightTemporary);
        instruction("LD", leftTemporary);

        instruction(
            memoryOpcode(expression.operation),
            rightTemporary
            );

        releaseTemporary(rightTemporary);
        releaseTemporary(leftTemporary);
    }

    void emitExpression(
        const ExpressionPtr &expression
        )
    {
        if (!expression)
        {
            throw CodegenError(
                "Expressao nula encontrada no IR."
                );
        }

        switch (expression->operation)
        {
        case Operation::Literal:
            loadLiteral(expression->value);
            return;

        case Operation::Variable:
            requireScalar(expression->name);
            instruction("LD", expression->name);
            return;

        case Operation::VectorElement:
            requireVector(expression->name);

            if (!expression->index)
            {
                throw CodegenError(
                    "Acesso a vetor sem indice."
                    );
            }

            emitExpression(expression->index);
            instruction("STO", "$indr");
            instruction("LDV", expression->name);
            return;

        case Operation::Negate:
        case Operation::BitwiseNot:
            emitUnary(*expression);
            return;

        case Operation::ShiftLeft:
        case Operation::ShiftRight:
        case Operation::Add:
        case Operation::Subtract:
        case Operation::BitwiseAnd:
        case Operation::BitwiseOr:
        case Operation::BitwiseXor:
            emitBinary(*expression);
            return;

        default:
            throw CodegenError(
                "Expressao logica ou relacional "
                "utilizada em contexto inteiro nao suportado."
                );
        }
    }

    // Avalia a comparacao como um desvio para dois
    // destinos explicitos. A ordem de avaliacao dos
    // operandos e preservada.
    void emitComparison(
        const Expression &expression,
        const std::string &trueLabel,
        const std::string &falseLabel
        )
    {
        if (!expression.left || !expression.right)
        {
            throw CodegenError(
                "Comparacao sem operandos."
                );
        }

        const std::string leftTemporary =
            acquireTemporary();

        emitExpression(expression.left);
        instruction("STO", leftTemporary);

        const std::string rightTemporary =
            acquireTemporary();

        emitExpression(expression.right);
        instruction("STO", rightTemporary);

        const Operation operation = expression.operation;

        if (operation == Operation::Equal ||
            operation == Operation::NotEqual)
        {
            // Igualdade e desigualdade dependem apenas
            // de zero. O wrap da subtracao de 16 bits
            // nao altera a igualdade de operandos.
            instruction("LD", leftTemporary);
            instruction("SUB", rightTemporary);

            instruction(
                comparisonOpcode(operation),
                trueLabel
                );

            instruction("JMP", falseLabel);
        }
        else
        {
            // Para ordenacao, a subtracao de inteiros
            // de sinais opostos pode produzir overflow.
            //
            // Com sinais opostos, a comparacao e
            // decidida diretamente pelos sinais.
            // Somente com sinais iguais fazemos SUB.
            const std::string leftNegative =
                newLabel();

            const std::string positiveNegative =
                newLabel();

            const std::string negativePositive =
                newLabel();

            const std::string sameSign =
                newLabel();

            instruction("LD", leftTemporary);
            instruction("BLT", leftNegative);

            instruction("LD", rightTemporary);
            instruction("BLT", positiveNegative);
            instruction("JMP", sameSign);

            label(leftNegative);

            instruction("LD", rightTemporary);
            instruction("BGE", negativePositive);
            instruction("JMP", sameSign);

            label(positiveNegative);

            const bool positiveIsGreater =
                operation == Operation::Greater ||
                operation == Operation::GreaterEqual;

            instruction(
                "JMP",
                positiveIsGreater
                    ? trueLabel
                    : falseLabel
                );

            label(negativePositive);

            instruction(
                "JMP",
                positiveIsGreater
                    ? falseLabel
                    : trueLabel
                );

            label(sameSign);

            instruction("LD", leftTemporary);
            instruction("SUB", rightTemporary);

            instruction(
                comparisonOpcode(operation),
                trueLabel
                );

            instruction("JMP", falseLabel);
        }

        releaseTemporary(rightTemporary);
        releaseTemporary(leftTemporary);
    }

    // Compila expressoes logicas com curto-circuito.
    // Nao usa AND/OR bit a bit para representar
    // operadores logicos da linguagem.
    void emitCondition(
        const ExpressionPtr &expression,
        const std::string &trueLabel,
        const std::string &falseLabel
        )
    {
        if (!expression)
        {
            throw CodegenError(
                "Condicao nula no comando se."
                );
        }

        switch (expression->operation)
        {
        case Operation::BooleanLiteral:
            instruction(
                "JMP",
                expression->value != 0
                    ? trueLabel
                    : falseLabel
                );
            return;

        case Operation::LogicalNot:
            emitCondition(
                expression->left,
                falseLabel,
                trueLabel
                );
            return;

        case Operation::LogicalAnd:
        {
            if (!expression->left || !expression->right)
            {
                throw CodegenError(
                    "Operacao logica 'e' incompleta."
                    );
            }

            const std::string evaluateRight =
                newLabel();

            emitCondition(
                expression->left,
                evaluateRight,
                falseLabel
                );

            label(evaluateRight);

            emitCondition(
                expression->right,
                trueLabel,
                falseLabel
                );
            return;
        }

        case Operation::LogicalOr:
        {
            if (!expression->left || !expression->right)
            {
                throw CodegenError(
                    "Operacao logica 'ou' incompleta."
                    );
            }

            const std::string evaluateRight =
                newLabel();

            emitCondition(
                expression->left,
                trueLabel,
                evaluateRight
                );

            label(evaluateRight);

            emitCondition(
                expression->right,
                trueLabel,
                falseLabel
                );
            return;
        }

        default:
            break;
        }

        if (isRelational(expression->operation))
        {
            emitComparison(
                *expression,
                trueLabel,
                falseLabel
                );
            return;
        }

        throw CodegenError(
            "Condicao BIP exige expressao relacional "
            "ou expressao logica valida."
            );
    }

    void emitAssignment(const Statement &statement)
    {
        if (!statement.expression)
        {
            throw CodegenError(
                "Atribuicao sem expressao."
                );
        }

        if (!statement.destinationIndex)
        {
            requireScalar(statement.destination);

            emitExpression(statement.expression);

            instruction(
                "STO",
                statement.destination
                );

            return;
        }

        requireVector(statement.destination);

        const std::string indexTemporary =
            acquireTemporary();

        emitExpression(statement.destinationIndex);
        instruction("STO", indexTemporary);

        // A expressao direita pode acessar vetores e
        // alterar o registrador especial $indr.
        emitExpression(statement.expression);

        const std::string valueTemporary =
            acquireTemporary();

        instruction("STO", valueTemporary);

        instruction("LD", indexTemporary);
        instruction("STO", "$indr");

        instruction("LD", valueTemporary);
        instruction("STOV", statement.destination);

        releaseTemporary(valueTemporary);
        releaseTemporary(indexTemporary);
    }

    void emitInput(const Statement &statement)
    {
        if (!statement.destinationIndex)
        {
            requireScalar(statement.destination);

            instruction("LD", "$in_port");
            instruction("STO", statement.destination);
            return;
        }

        requireVector(statement.destination);

        emitExpression(statement.destinationIndex);

        instruction("STO", "$indr");
        instruction("LD", "$in_port");
        instruction("STOV", statement.destination);
    }

    void emitOutput(const Statement &statement)
    {
        if (!statement.expression)
        {
            throw CodegenError(
                "Comando mostre sem operando."
                );
        }

        emitExpression(statement.expression);

        instruction("STO", "$out_port");
    }

    void emitConditional(const Statement &statement)
    {
        if (!statement.expression)
        {
            throw CodegenError(
                "Comando se sem expressao condicional."
                );
        }

        const std::string thenLabel =
            newLabel();

        const std::string falseLabel =
            newLabel();

        emitCondition(
            statement.expression,
            thenLabel,
            falseLabel
            );

        label(thenLabel);

        emitStatementList(
            statement.thenStatements
            );

        if (statement.hasElse)
        {
            const std::string endLabel =
                newLabel();

            instruction("JMP", endLabel);

            label(falseLabel);

            emitStatementList(
                statement.elseStatements
                );

            label(endLabel);
        }
        else
        {
            label(falseLabel);
        }
    }

    void emitStatement(const Statement &statement)
    {
        switch (statement.kind)
        {
        case StatementKind::Assignment:
            emitAssignment(statement);
            return;

        case StatementKind::Input:
            emitInput(statement);
            return;

        case StatementKind::Output:
            emitOutput(statement);
            return;

        case StatementKind::Conditional:
            emitConditional(statement);
            return;
        }

        throw CodegenError(
            "Comando desconhecido no gerador BIP."
            );
    }

    void emitStatementList(
        const std::vector<Statement> &statements
        )
    {
        for (const Statement &statement : statements)
        {
            emitStatement(statement);
        }
    }

    const Program &program_;

    std::unordered_map<
        std::string,
        Declaration
        > declarations_;

    std::unordered_set<std::string>
        occupiedNames_;

    std::size_t dataCells_ = 0;
    std::size_t nextTemporaryId_ = 1;
    std::size_t nextLabelId_ = 1;

    std::vector<Temporary> temporaries_;
    std::vector<std::string> instructions_;
};

}

std::string BipEmitter::emit(
    const Program &program
    ) const
{
    EmitterState emitter(program);
    return emitter.generate();
}

}