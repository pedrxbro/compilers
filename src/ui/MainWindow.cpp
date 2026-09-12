#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "CompilerService.h"

#include <QString>
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
            const std::string source =
                ui->sourceEditor
                    ->toPlainText()
                    .toUtf8()
                    .toStdString();

            const CompilerService compilerService;
            const AnalysisResult result =
                compilerService.analyze(source);

            QString message =
                QString::fromUtf8(result.message.c_str());

            if (result.position >= 0)
            {
                message += QStringLiteral(
                               "\nPosição: %1"
                               ).arg(result.position);
            }

            ui->messagesArea->setPlainText(message);
        }
        );
}

MainWindow::~MainWindow()
{
    delete ui;
}