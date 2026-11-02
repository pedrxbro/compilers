
#include "BipIr.h"

#include <cstddef>
#include <limits>
#include <sstream>
#include <string>
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

struct Temporary
{
    std::string name;
    bool inUse = false;
};

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

class EmitterState
{
public:
    explicit EmitterState(const Program &program)
        : program_(program)
    {
        for (const std::string &name : program.variables)
        {
            if (name == "_PRINCIPAL" ||
                name.rfind("__bip_tmp_", 0) == 0)
            {
                throw CodegenError(
                    "Identificador reservado: " + name
                    );
            }

            if (!variableNames_.insert(name).second)
            {
                throw CodegenError(
                    "Variavel duplicada no IR: " + name
                    );
            }
        }
    }

    std::string generate()
    {
        for (const Assignment &assignment :
             program_.assignments)
        {
            requireVariable(assignment.destination);

            if (!assignment.expression)
            {
                throw CodegenError(
                    "Atribuicao sem expressao."
                    );
            }

            emitExpression(assignment.expression);
            instruction("STO", assignment.destination);
        }

        instruction("HLT", "0");

        std::ostringstream output;

        output << ".data\n";

        for (const std::string &name : program_.variables)
        {
            output << name << ": 0\n";
        }

        for (const Temporary &temporary : temporaries_)
        {
            output << temporary.name << ": 0\n";
        }

        output << ".text\n";
        output << "_PRINCIPAL:\n";

        for (const std::string &line : instructions_)
        {
            output << "    " << line << '\n';
        }

        return output.str();
    }

private:
    void requireVariable(const std::string &name) const
    {
        if (variableNames_.count(name) == 0)
        {
            throw CodegenError(
                "Variavel nao declarada no IR: " + name
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
        instruction(opcode, std::to_string(operand));
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
                "neste nucleo BIP: " +
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

        const std::string name =
            "__bip_tmp_" +
            std::to_string(temporaries_.size());

        if (variableNames_.count(name) != 0)
        {
            throw CodegenError(
                "Conflito com variavel temporaria: " + name
                );
        }

        temporaries_.push_back({name, true});

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

    static std::string memoryOpcode(Operation operation)
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

    static std::string immediateOpcode(Operation operation)
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

        if (expression.operation == Operation::BitwiseNot)
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
                    loadLiteral(static_cast<int>(value));
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
            expression.operation == Operation::ShiftLeft
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

        if (expression.operation == Operation::ShiftLeft ||
            expression.operation == Operation::ShiftRight)
        {
            emitShift(expression);
            return;
        }

        const Expression &right = *expression.right;

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
            requireVariable(right.name);

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

    void emitExpression(const ExpressionPtr &expression)
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
            requireVariable(expression->name);
            instruction("LD", expression->name);
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
        }

        throw CodegenError(
            "Expressao desconhecida no gerador BIP."
            );
    }

    const Program &program_;

    std::unordered_set<std::string> variableNames_;

    std::vector<Temporary> temporaries_;
    std::vector<std::string> instructions_;
};

}

std::string BipEmitter::emit(const Program &program) const
{
    EmitterState emitter(program);
    return emitter.generate();
}

}
