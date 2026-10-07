#include "ScopeManager.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace std;

int main()
{
    ScopeManager manager;

    // ========================================================
    // 1. ESCOPO GLOBAL
    // ========================================================

    assert(manager.atGlobalScope());

    assert(
        manager.currentScope().kind() ==
        ScopeKind::Global
        );

    assert(
        manager.currentScope().qualifiedName() ==
        "global"
        );

    Symbol *globalX =
        manager.declareSymbol(
            "x",
            DataType::Integer,
            SymbolKind::Variable
            );

    assert(globalX != nullptr);

    assert(
        globalX->scope() ==
        "global"
        );

    // ========================================================
    // 2. DUPLICIDADE NO MESMO ESCOPO
    // ========================================================

    Symbol *duplicatedGlobalX =
        manager.declareSymbol(
            "x",
            DataType::Float,
            SymbolKind::Variable
            );

    assert(duplicatedGlobalX == nullptr);

    // ========================================================
    // 3. ENTRADA EM ESCOPO DE FUNÇÃO
    // ========================================================

    Scope &functionScope =
        manager.enterFunctionScope(
            "calcula"
            );

    assert(
        functionScope.kind() ==
        ScopeKind::Function
        );

    assert(
        functionScope.parent() ==
        &manager.globalScope()
        );

    assert(
        functionScope.qualifiedName() ==
        "global::calcula"
        );

    assert(
        manager.globalScope().childCount() ==
        1
        );

    // ========================================================
    // 4. GLOBAL VISÍVEL DENTRO DA FUNÇÃO
    // ========================================================

    assert(
        manager.findVisible("x") ==
        globalX
        );

    // ========================================================
    // 5. SOMBREAMENTO EM OUTRO ESCOPO
    // ========================================================

    Symbol *functionX =
        manager.declareSymbol(
            "x",
            DataType::Float,
            SymbolKind::Variable
            );

    assert(functionX != nullptr);

    assert(
        functionX != globalX
        );

    assert(
        manager.findVisible("x") ==
        functionX
        );

    assert(
        manager.findInCurrentScope("x") ==
        functionX
        );

    // Mesmo nome novamente dentro da função:
    // deve ser rejeitado.
    Symbol *duplicatedFunctionX =
        manager.declareSymbol(
            "x",
            DataType::Text,
            SymbolKind::Variable
            );

    assert(duplicatedFunctionX == nullptr);

    // ========================================================
    // 6. PRIMEIRO ESCOPO INTERNO
    // ========================================================

    Scope &firstInternalScope =
        manager.enterInternalScope(
            "se"
            );

    assert(
        firstInternalScope.kind() ==
        ScopeKind::Internal
        );

    assert(
        firstInternalScope.parent() ==
        &functionScope
        );

    Symbol *internalValue =
        manager.declareSymbol(
            "interno",
            DataType::Integer,
            SymbolKind::Variable
            );

    assert(internalValue != nullptr);

    assert(
        manager.findVisible("interno") ==
        internalValue
        );

    // O x da função continua visível.
    assert(
        manager.findVisible("x") ==
        functionX
        );

    // ========================================================
    // 7. SAÍDA DO ESCOPO INTERNO
    // ========================================================

    manager.leaveScope();

    assert(
        &manager.currentScope() ==
        &functionScope
        );

    // A variável do escopo encerrado não deve vazar.
    assert(
        manager.findVisible("interno") ==
        nullptr
        );

    // ========================================================
    // 8. SEGUNDO ESCOPO INTERNO / IRMÃO
    // ========================================================

    Scope &secondInternalScope =
        manager.enterInternalScope(
            "senao"
            );

    assert(
        secondInternalScope.parent() ==
        &functionScope
        );

    // Símbolo do primeiro bloco não pode ser visto
    // pelo segundo bloco.
    assert(
        manager.findVisible("interno") ==
        nullptr
        );

    // Símbolo do pai continua disponível.
    assert(
        manager.findVisible("x") ==
        functionX
        );

    // O mesmo nome usado pelo outro bloco pode ser
    // declarado porque são escopos distintos.
    Symbol *secondInternalValue =
        manager.declareSymbol(
            "interno",
            DataType::Text,
            SymbolKind::Variable
            );

    assert(secondInternalValue != nullptr);

    assert(
        manager.findVisible("interno") ==
        secondInternalValue
        );

    // ========================================================
    // 9. RETORNO AO ESCOPO DA FUNÇÃO
    // ========================================================

    manager.leaveScope();

    assert(
        &manager.currentScope() ==
        &functionScope
        );

    assert(
        manager.findVisible("interno") ==
        nullptr
        );

    // ========================================================
    // 10. RETORNO AO GLOBAL
    // ========================================================

    manager.leaveScope();

    assert(manager.atGlobalScope());

    // O x local da função não pode vazar.
    assert(
        manager.findVisible("x") ==
        globalX
        );

    assert(
        manager.findVisible("interno") ==
        nullptr
        );

    // ========================================================
    // 11. NÃO PODE SAIR DO GLOBAL
    // ========================================================

    bool exceptionThrown = false;

    try
    {
        manager.leaveScope();
    }
    catch (const logic_error &)
    {
        exceptionThrown = true;
    }

    assert(exceptionThrown);

    cout
        << "Todos os testes do ScopeManager passaram."
        << endl;

    return 0;
}