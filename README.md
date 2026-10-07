# Compiladores

Projeto desenvolvido para a disciplina de **Compiladores** do curso de **Ciência da Computação** da **UNIVALI – Campus Itajaí**.

## Alunos

- João Victor da Silva
- Pedro Henrique de Paula Cordeiro

## Sobre o projeto

Este projeto implementa uma IDE para análise léxica, sintática e semântica de uma linguagem desenvolvida durante a disciplina de Compiladores.

Os analisadores foram construídos com auxílio do **WebGALS** e integrados a uma aplicação desktop desenvolvida em **C++ com Qt Widgets**.

Atualmente o projeto contempla as funcionalidades desenvolvidas até a **M2.1**, incluindo:

- análise léxica;
- análise sintática;
- ações semânticas;
- tabela de símbolos;
- controle de escopos;
- verificação de declaração e visibilidade de identificadores;
- verificação de unicidade;
- controle de uso e inicialização;
- sistema de compatibilidade de tipos;
- visualização da tabela de símbolos na IDE.

## Fluxo da aplicação

O fluxo principal da compilação é:

    Código-fonte
        ↓
    MainWindow
        ↓
    CompilerService
        ↓
    Lexico
        ↓
    Sintatico
        ↓
    Ações semânticas
        ↓
    Semantico
        ↓
    Tabela de símbolos
        ↓
    Escopos
        ↓
    Sistema de tipos
        ↓
    AnalysisResult
        ↓
    MainWindow
        ↓
    Mensagens e tabela de símbolos

A interface gráfica não acessa diretamente os componentes internos do compilador.

Toda a execução da análise é centralizada no `CompilerService`.

## Funcionalidades da IDE

A IDE possui:

- editor de código-fonte;
- fonte tamanho 14 no editor;
- botão para compilar o programa;
- área de mensagens e depuração;
- fonte tamanho 14 na área de mensagens;
- exibição de erros léxicos;
- exibição de erros sintáticos;
- exibição de erros semânticos;
- exibição de avisos semânticos;
- indicação da posição do erro quando disponível;
- tabela visual de símbolos.

A tabela de símbolos apresenta:

- nome;
- tipo;
- modalidade;
- escopo;
- estado de inicialização;
- estado de uso.

## Análise semântica

A M2.1 acrescenta ações semânticas à gramática utilizada pelo WebGALS.

O analisador realiza as seguintes verificações:

- inserção dos identificadores na tabela de símbolos;
- armazenamento do tipo do identificador;
- armazenamento da modalidade;
- armazenamento do escopo;
- declaração antes do uso;
- visibilidade entre escopos;
- unicidade dentro do mesmo escopo;
- aviso de identificadores declarados e não utilizados;
- aviso de identificadores utilizados antes da inicialização;
- compatibilidade de tipos nas expressões;
- compatibilidade de tipos nas atribuições.

## Identificadores

A tabela de símbolos trabalha com as seguintes modalidades:

- variável;
- vetor;
- parâmetro;
- função.

Procedimentos também são representados como sub-rotinas na tabela, utilizando tipo de retorno `void`.

## Escopos

Os escopos são organizados hierarquicamente.

A estrutura básica é:

    global
        ├── função/procedimento
        │       └── blocos internos
        │
        └── blocos internos do programa principal

A busca de identificadores ocorre a partir do escopo atual em direção aos seus escopos pais.

Dessa forma:

- identificadores do escopo pai podem ser utilizados pelos filhos;
- identificadores de blocos internos não vazam para o escopo pai;
- identificadores de escopos irmãos não são visíveis entre si;
- o mesmo nome pode existir em escopos diferentes;
- o mesmo nome não pode ser declarado duas vezes no mesmo escopo.

## Sistema de tipos

Os tipos suportados semanticamente são:

- `INTEIRO`;
- `FLUTUANTE`;
- `TEXTO`;
- `LOGICO`;
- `CARACTERE`.

Também existem internamente os tipos:

- `VOID`;
- `UNKNOWN`.

O sistema de tipos verifica operações aritméticas, relacionais e lógicas, além da compatibilidade das atribuições.

Conversões potencialmente perigosas podem produzir avisos semânticos sem impedir a compilação.

## Estrutura do projeto

    gals/
    ├── lexical/
    │   └── analisador-lexico.gals
    │
    └── syntatic/
        └── analisador-sintatico-v2.0.2.vgls

    generated/
    └── webgals/
        ├── AnalysisError.h
        ├── Constants.cpp
        ├── Constants.h
        ├── LexicalError.h
        ├── Lexico.cpp
        ├── Lexico.h
        ├── SemanticError.h
        ├── Semantico.cpp
        ├── Semantico.h
        ├── Sintatico.cpp
        ├── Sintatico.h
        ├── SyntacticError.h
        └── Token.h

    src/
    ├── compiler/
    │   ├── AnalysisResult.h
    │   ├── CompilerService.cpp
    │   ├── CompilerService.h
    │   │
    │   └── semantic/
    │       ├── DeclarationProcessor.cpp
    │       ├── DeclarationProcessor.h
    │       ├── Scope.cpp
    │       ├── Scope.h
    │       ├── ScopeManager.cpp
    │       ├── ScopeManager.h
    │       ├── SemanticTable.cpp
    │       ├── SemanticTable.h
    │       ├── SemanticTypes.h
    │       ├── Symbol.cpp
    │       └── Symbol.h
    │
    ├── ui/
    │   ├── MainWindow.cpp
    │   ├── MainWindow.h
    │   └── MainWindow.ui
    │
    └── main.cpp

    tests/
    ├── CompilerServiceSmokeTest.cpp
    ├── ScopeManagerTest.cpp
    ├── SemanticIntegrationTest.cpp
    └── TypeSystemTest.cpp

## Organização das camadas

### `gals`

Contém os projetos utilizados no WebGALS.

O arquivo utilizado para a versão sintática e semântica atual é:

    gals/syntatic/analisador-sintatico-v2.0.2.vgls

Esse projeto contém a gramática e as ações semânticas utilizadas para gerar o analisador integrado à aplicação.

### `generated/webgals`

Contém as classes produzidas a partir do projeto WebGALS.

Os arquivos `Semantico.cpp` e `Semantico.h` contêm atualmente a implementação das ações semânticas utilizadas pela M2.1.

Por esse motivo, uma nova geração pelo WebGALS deve ser feita com cuidado para que a implementação semântica existente não seja perdida.

### `src/compiler`

Contém a camada responsável por integrar a aplicação aos analisadores.

Os principais componentes são:

- `CompilerService`: coordena as análises;
- `AnalysisResult`: transporta resultado, erros, avisos e tabela de símbolos.

### `src/compiler/semantic`

Contém a infraestrutura semântica da aplicação.

Principais componentes:

- `Symbol`: representa um identificador;
- `Scope`: representa um escopo;
- `ScopeManager`: gerencia a árvore de escopos e visibilidade;
- `DeclarationProcessor`: processa declarações produzidas pelas ações semânticas;
- `SemanticTable`: implementa as regras de compatibilidade de tipos;
- `SemanticTypes`: define os tipos e modalidades utilizados pela análise.

### `src/ui`

Contém a interface gráfica desenvolvida com Qt Widgets.

A `MainWindow` é responsável por:

- obter o código-fonte;
- executar o `CompilerService`;
- exibir mensagens;
- exibir a tabela de símbolos.

## Testes

O projeto possui testes para diferentes partes do compilador.

### CompilerServiceSmokeTest

Exercita o fluxo geral do compilador com programas válidos e inválidos.

### ScopeManagerTest

Valida:

- escopo global;
- escopos de funções;
- escopos internos;
- visibilidade;
- sombreamento;
- unicidade;
- prevenção de vazamento entre escopos.

### TypeSystemTest

Valida:

- operações entre tipos;
- operações incompatíveis;
- conversões;
- atribuições;
- operadores unários;
- expressões compostas;
- condições lógicas.

### SemanticIntegrationTest

Valida a integração dos requisitos da M2.1, incluindo:

- declarações;
- unicidade;
- escopos;
- inicialização;
- utilização;
- vetores;
- parâmetros;
- funções;
- expressões;
- atribuições.

## Build

O projeto utiliza:

- C++17;
- CMake;
- Qt 5 ou Qt 6 Widgets.

O `CMakeLists.txt` cria:

- a biblioteca `compiler_core`;
- o executável gráfico `compiler`;
- os executáveis de testes quando `BUILD_TESTING` está habilitado.

## Tecnologias utilizadas

- C++
- CMake
- Qt Widgets
- WebGALS
- Analisador Sintático SLR
- CTest

## Instituição

**Universidade do Vale do Itajaí – UNIVALI**

**Curso:** Ciência da Computação  
**Disciplina:** Compiladores  
**Campus:** Itajaí

## Autores

**João Victor da Silva**  
**Pedro Henrique de Paula Cordeiro**