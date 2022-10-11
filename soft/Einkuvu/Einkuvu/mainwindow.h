#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QStringList>
#include <QString>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QFile>
#include <QByteArray>
#include "checksumxor.h"
#include "jqchecksum.h"

namespace Ui {
class MainWindow;
}

typedef struct
{
    QString portName;
    QString portAttr;
} mySerial_t;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void Read_Data();
    void Timer_Done();
    void ReceiveTimeout_Done();
    void on_pushButton_Connect_clicked();

    void on_pushButton_Open_clicked();

    void on_pushButton_Start_clicked();


    void on_pushButton_3_clicked();

    void on_pushButton_6_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort* serial;
    QVector<mySerial_t> Serial_list;
    QByteArray ReadFlieData;
    QTimer* TimerTxd;
    uint16_t DevQueryNumber;
    uint16_t FileTotalSegment;
    uint16_t Query_Interval;
    uint16_t TRxd_Timeout;
    QTimer* FrameTimeout;

    __int32 TxdBytes;
    __int32 TxdDataIdx;

   uint8_t MessageByteOperation(uint8_t _In_Byte,uint8_t* _Out_MsgBuffer);
};

#endif // MAINWINDOW_H
