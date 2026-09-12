# Compiladores

Projeto desenvolvido para a disciplina de **Compiladores** do curso de **Ciência da Computação** da **UNIVALI – Campus Itajaí**.

## Alunos

- João Victor da Silva
- Pedro Henrique de Paula Cordeiro

## Sobre o projeto

Este projeto implementa uma IDE simples para realizar a análise léxica e sintática de uma linguagem desenvolvida durante a disciplina de Compiladores.

Os analisadores léxico e sintático foram gerados utilizando o **WebGALS** e integrados a uma aplicação desktop desenvolvida em **C++ com Qt**.

O fluxo principal da aplicação é:

Editor de código-fonte  
→ MainWindow  
→ CompilerService  
→ Analisador Léxico  
→ Analisador Sintático  
→ Resultado da análise  
→ Área de mensagens

## Funcionalidades

A IDE possui:

- editor de código-fonte;
- fonte tamanho 14 no editor;
- botão para realizar a análise sintática;
- integração com o analisador léxico gerado pelo WebGALS;
- integração com o analisador sintático gerado pelo WebGALS;
- exibição de erros léxicos;
- exibição de erros sintáticos;
- indicação da posição do erro;
- mensagem de sucesso para programas sintaticamente válidos;
- área para mensagens e depuração;
- fonte tamanho 14 na área de mensagens.

## Estrutura do projeto

    gals/
    ├── lexical/
    │   └── analisador-lexico.gals
    └── syntatic/
        └── analisador-sintatico.gals

    generated/
    └── webgals/
        ├── Lexico.cpp
        ├── Lexico.h
        ├── Sintatico.cpp
        ├── Sintatico.h
        ├── Semantico.cpp
        ├── Semantico.h
        ├── Constants.cpp
        ├── Constants.h
        └── demais arquivos gerados pelo WebGALS

    src/
    ├── compiler/
    │   ├── AnalysisResult.h
    │   ├── CompilerService.cpp
    │   └── CompilerService.h
    ├── ui/
    │   ├── MainWindow.cpp
    │   ├── MainWindow.h
    │   └── MainWindow.ui
    └── main.cpp

    tests/
    └── CompilerServiceSmokeTest.cpp

## Organização

### gals

Contém os arquivos utilizados no WebGALS para definição dos analisadores da linguagem.

### generated/webgals

Contém os arquivos C++ gerados automaticamente pelo WebGALS.

Esses arquivos representam o núcleo dos analisadores léxico e sintático e não devem ser alterados manualmente.

### src/compiler

Contém a camada responsável pela comunicação entre a aplicação e os analisadores gerados pelo WebGALS.

Os principais componentes são:

- `CompilerService`: executa a análise do código-fonte;
- `AnalysisResult`: representa o resultado da análise.

### src/ui

Contém a interface gráfica da aplicação desenvolvida utilizando Qt Widgets.

A `MainWindow` é responsável por:

- obter o código digitado pelo usuário;
- enviar o código ao `CompilerService`;
- receber o resultado da análise;
- apresentar as mensagens ao usuário.

## Arquitetura

A aplicação segue o seguinte fluxo:

    Editor
       ↓
    MainWindow
       ↓
    CompilerService
       ↓
    Lexico
       ↓
    Sintatico
       ↓
    AnalysisResult
       ↓
    Área de mensagens

A interface gráfica não acessa diretamente os analisadores gerados pelo WebGALS.

Toda a comunicação com o analisador léxico e o analisador sintático é centralizada no `CompilerService`.

## Tecnologias utilizadas

- C++
- CMake
- Qt Widgets
- WebGALS
- Analisador Sintático SLR

## Instituição

**Universidade do Vale do Itajaí – UNIVALI**

**Curso:** Ciência da Computação  
**Disciplina:** Compiladores  
**Campus:** Itajaí

## Autores

**João Victor da Silva**  
**Pedro Henrique de Paula Cordeiro**