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
#include <QList>
#include <QDesktopServices>
#include <mergebinarydata.h>
#include <binarytohexstring.h>
namespace Ui {
class MainWindow;
}

typedef struct
{
    QString model           ;
    QString boot            ;
    QString app             ;
    QString upgrade         ;
    int boot_max_sizeofbyte ;
    int app_max_sizeofbyte  ;
    int flash_addr_str      ;
    int flash_resver_data   ;
}mcuinfo;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_pushButton_CreatMerge_clicked();

    void on_pushButton_OpenBoot_clicked();

    void on_pushButton_OpenApp_clicked();

    void on_pushButton_OpenFolder_clicked();

    void on_pushButton_ClearMerge_clicked();

    void on_comboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QString boot_file_name;
    QString app_file_name;
    QList<mcuinfo> mcu_info_list;
};

#endif // MAINWINDOW_H
