#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QString>
#include <QByteArray>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include "JQChecksum.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_open_clicked();

    void on_pushButton_Creating_clicked();

    void on_pushButton_OpenFolder_clicked();

    void on_pushButton_Clear_clicked();

private:
    Ui::MainWindow *ui;
    QString filename_Org;
};

#endif // MAINWINDOW_H
