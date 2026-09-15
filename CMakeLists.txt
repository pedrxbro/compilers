cmake_minimum_required(VERSION 3.16)

project(compiler VERSION 0.1 LANGUAGES CXX)

# ============================================================
# CONFIGURAÇÕES GERAIS
# ============================================================

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# QT
# ============================================================

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# ============================================================
# NÚCLEO DO COMPILADOR
# ============================================================

add_library(compiler_core STATIC

    # Camada da aplicação
    src/compiler/CompilerService.cpp
    src/compiler/CompilerService.h
    src/compiler/AnalysisResult.h

    # Código gerado pelo WebGALS
    generated/webgals/Constants.cpp
    generated/webgals/Lexico.cpp
    generated/webgals/Sintatico.cpp
    generated/webgals/Semantico.cpp
)

# ============================================================
# HEADERS DO COMPILER CORE
# ============================================================

target_include_directories(compiler_core

    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/compiler

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/webgals
)

# ============================================================
# INTERFACE GRÁFICA
# ============================================================

set(PROJECT_SOURCES

    src/main.cpp

    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.ui
)

# ============================================================
# EXECUTÁVEL PRINCIPAL
# ============================================================

if(QT_VERSION_MAJOR GREATER_EQUAL 6)

    qt_add_executable(compiler
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

else()

    if(ANDROID)

        add_library(compiler SHARED
            ${PROJECT_SOURCES}
        )

    else()

        add_executable(compiler
            ${PROJECT_SOURCES}
        )

    endif()

endif()

# ============================================================
# INCLUDES DA APLICAÇÃO
# ============================================================

target_include_directories(compiler

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

# ============================================================
# DEPENDÊNCIAS DO EXECUTÁVEL
# ============================================================

target_link_libraries(compiler

    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        compiler_core
)

# ============================================================
# CONFIGURAÇÕES DE BUNDLE
# ============================================================

if(QT_VERSION VERSION_LESS 6.1.0)

    set(BUNDLE_ID_OPTION
        MACOSX_BUNDLE_GUI_IDENTIFIER com.example.compiler
    )

endif()

set_target_properties(compiler PROPERTIES

    ${BUNDLE_ID_OPTION}

    MACOSX_BUNDLE_BUNDLE_VERSION
        ${PROJECT_VERSION}

    MACOSX_BUNDLE_SHORT_VERSION_STRING
        ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}

    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ============================================================
# INSTALAÇÃO
# ============================================================

include(GNUInstallDirs)

install(
    TARGETS compiler

    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ============================================================
# TESTE DO COMPILER SERVICE
# ============================================================

add_executable(compiler_service_smoke_test

    tests/CompilerServiceSmokeTest.cpp
)

target_link_libraries(compiler_service_smoke_test

    PRIVATE
        compiler_core
)

# ============================================================
# FINALIZAÇÃO QT 6
# ============================================================

if(QT_VERSION_MAJOR EQUAL 6)

    qt_finalize_executable(compiler)

endif()
