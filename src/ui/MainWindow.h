#pragma once

#include <QMainWindow>

class QPlainTextEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void handleCompile();

    QPlainTextEdit *sourceEditor_;
    QPushButton *compileButton_;
    QPlainTextEdit *messagesArea_;
};
