
#include "BipIr.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_map>
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
    return name == "_PRINCIPAL" ||
           name.rfind("__bip_tmp_", 0) == 0 ||
           name.rfind("__bip_vec_slot_", 0) == 0;
}

class EmitterState
{
public:
    explicit EmitterState(const Program &program)
        : program_(program)
    {
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
        }
    }

    std::string generate()
    {
        for (const Statement &statement :
             program_.statements)
        {
            switch (statement.kind)
            {
            case StatementKind::Assignment:
                emitAssignment(statement);
                break;

            case StatementKind::Input:
                emitInput(statement);
                break;

            case StatementKind::Output:
                emitOutput(statement);
                break;
            }
        }

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

        // Declara variaveis escalares e vetores.
        // Vetores sao representados por uma unica
        // declaracao com os elementos separados
        // por virgulas, conforme o formato BIP.
        for (const Declaration &declaration :
             program_.declarations)
        {
            output << declaration.name << ": 0";

            if (declaration.isVector)
            {
                for (std::size_t j = 1;
                     j < declaration.size;
                     ++j)
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
            output << "    " << line << '\n';
        }

        return output.str();
    }

private:
    const Declaration &requireDeclaration(
        const std::string &name
        ) const
    {
        const auto found = declarations_.find(name);

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

        // A numeracao comeca em 1 e nao reutiliza
        // nomes que ja pertencem ao codigo fonte.
        std::string name;

        do
        {
            name = "temp" +
                   std::to_string(nextTemporaryId_++);
        }
        while (declarations_.count(name) != 0);

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
            requireScalar(right.name);

            emitExpression(expression.left);

            instruction(
                memoryOpcode(expression.operation),
                right.name
                );

            return;
        }

        // Preserva o primeiro operando enquanto
        // o segundo e avaliado.
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
        }

        throw CodegenError(
            "Expressao desconhecida no gerador BIP."
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

        // O indice permanece reservado durante
        // toda a avaliacao do lado direito.
        const std::string indexTemporary =
            acquireTemporary();

        emitExpression(statement.destinationIndex);

        instruction("STO", indexTemporary);

        // A expressao pode acessar outros vetores
        // e modificar o registrador $indr.
        emitExpression(statement.expression);

        const std::string valueTemporary =
            acquireTemporary();

        instruction("STO", valueTemporary);

        // Restaura o indice antes de armazenar.
        instruction("LD", indexTemporary);
        instruction("STO", "$indr");

        // STOV recebe o valor do ACC.
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
            instruction(
                "STO",
                statement.destination
                );

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

    const Program &program_;

    std::unordered_map<std::string, Declaration> declarations_;

    std::size_t dataCells_ = 0;
    std::size_t nextTemporaryId_ = 1;

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