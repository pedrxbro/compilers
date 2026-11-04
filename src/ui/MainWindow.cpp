#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "CompilerService.h"

#include <QString>
#include <QTableWidgetItem>

#include <string>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(
        ui->compileButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            // Limpa os resultados da compilacao anterior.
            ui->messagesArea->clear();
            ui->symbolTable->setRowCount(0);
            ui->assemblyArea->clear();

            // Exibe a aba de mensagens durante a compilacao.
            ui->resultsTabs->setCurrentWidget(
                ui->messagesTab
                );

            // Obtem o codigo-fonte do editor.
            const std::string source =
                ui->sourceEditor
                    ->toPlainText()
                    .toUtf8()
                    .toStdString();

            // Executa o fluxo completo pelo CompilerService.
            const CompilerService compilerService;

            const AnalysisResult result =
                compilerService.compile(source);

            // Apresenta as mensagens da compilacao.
            QString message =
                QString::fromUtf8(
                    result.message.c_str()
                    );

            if (result.position >= 0)
            {
                message += QStringLiteral(
                               "\nPosição: %1"
                               ).arg(result.position);
            }

            ui->messagesArea->setPlainText(
                message
                );

            // Prepara a tabela com os simbolos recebidos.
            ui->symbolTable->setRowCount(
                static_cast<int>(
                    result.symbolTable.size()
                    )
                );

            int row = 0;

            for (const SymbolTableEntry &symbol :
                 result.symbolTable)
            {
                // Nome
                ui->symbolTable->setItem(
                    row,
                    0,
                    new QTableWidgetItem(
                        QString::fromUtf8(
                            symbol.name.c_str()
                            )
                        )
                    );

                // Tipo
                ui->symbolTable->setItem(
                    row,
                    1,
                    new QTableWidgetItem(
                        QString::fromUtf8(
                            symbol.type.c_str()
                            )
                        )
                    );

                // Modalidade
                ui->symbolTable->setItem(
                    row,
                    2,
                    new QTableWidgetItem(
                        QString::fromUtf8(
                            symbol.kind.c_str()
                            )
                        )
                    );

                // Escopo
                ui->symbolTable->setItem(
                    row,
                    3,
                    new QTableWidgetItem(
                        QString::fromUtf8(
                            symbol.scope.c_str()
                            )
                        )
                    );

                // Inicializado
                ui->symbolTable->setItem(
                    row,
                    4,
                    new QTableWidgetItem(
                        symbol.initialized
                            ? QStringLiteral("Sim")
                            : QStringLiteral("Não")
                        )
                    );

                // Usado
                ui->symbolTable->setItem(
                    row,
                    5,
                    new QTableWidgetItem(
                        symbol.used
                            ? QStringLiteral("Sim")
                            : QStringLiteral("Não")
                        )
                    );

                ++row;
            }

            // Ajusta as larguras para facilitar a leitura.
            ui->symbolTable->resizeColumnsToContents();

            // Publica somente Assembly completo e valido.
            if (result.success && !result.assembly.empty())
            {
                ui->assemblyArea->setPlainText(
                    QString::fromUtf8(
                        result.assembly.c_str()
                        )
                    );
            }
        }
        );
}

MainWindow::~MainWindow()
{
    delete ui;
}