#include "mainwindow.h"
#include "ui_mainwindow.h"

uint8_t MakeMbusCheckSum(uint8_t* _In_CheckData,uint8_t _In_Length);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    TxdBytes = 0;

    //查找可用的串口
    Serial_list.clear();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        mySerial_t sPort;
        sPort.portName = info.portName();
        sPort.portAttr = "[" + info.description();
        sPort.portAttr.append("(" + info.manufacturer() + ")]");
        Serial_list.append(sPort);
        ui->comboBox_port->addItem(sPort.portName + sPort.portAttr);
    }

    serial = new QSerialPort;
    QObject::connect(serial, &QSerialPort::readyRead, this, &MainWindow::Read_Data);
    TimerTxd = new QTimer();
    QObject::connect(TimerTxd,SIGNAL(timeout()),this,SLOT(Timer_Done()));
    TimerTxd->start(10);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_Connect_clicked()
{

    if(ui->pushButton_Connect->text() == QString("Connect"))
    {
        if(0 == ui->comboBox_port->count())
        {
            QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Didn't find This COM！"), QMessageBox::Ok, this);
                        msgBox.exec();
                        return;
        }
        if(ui->comboBox_port->currentIndex() < 0)
        {
            QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please select COM!"), QMessageBox::Ok, this);
                        msgBox.exec();
                        return;
        }
        //设置串口名
        serial->setPortName(Serial_list[ui->comboBox_port->currentIndex()].portName);
        //打开串口
        if(false == serial->open(QIODevice::ReadWrite))//打开失败处理
        {
            QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Open this COM failed!"), QMessageBox::Ok, this);
                        msgBox.exec();
                        return;
        }
        if(ui->comboBox_baud->currentText()=="2400")
        {
            //设置波特率
            serial->setBaudRate(QSerialPort::Baud2400);
            //设置数据位数
            serial->setDataBits(QSerialPort::Data8);
            //设置奇偶校验
            serial->setParity(QSerialPort::EvenParity);
            //设置停止位
            serial->setStopBits(QSerialPort::OneStop);
            //设置流控制
            serial->setFlowControl(QSerialPort::NoFlowControl);
        }
        else if(ui->comboBox_baud->currentText()=="9600")
        {
            //设置波特率
            serial->setBaudRate(QSerialPort::Baud9600);
            //设置数据位数
            serial->setDataBits(QSerialPort::Data8);
            //设置奇偶校验
            serial->setParity(QSerialPort::NoParity);
            //设置停止位
            serial->setStopBits(QSerialPort::OneStop);
            //设置流控制
            serial->setFlowControl(QSerialPort::NoFlowControl);
        }

        //关闭设置菜单使能
        ui->comboBox_port->setEnabled(false);
        ui->pushButton_Connect->setText(QString::fromUtf8("Disconnect"));
    }
    else
    {
        //关闭串口
        serial->clear();
        serial->close();
        //恢复设置使能
        ui->comboBox_port->setEnabled(true);
        ui->pushButton_Connect->setText(QString::fromUtf8("Connect"));
        if(ui->pushButton_Start->text() ==QString("Start"))
        {
        }
        else
        {
            ui->pushButton_Start->click();
        }

    }
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const char AsciiTable[16]={
                                   0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,
                                   0x41,0x42,0x43,0x44,0x45,0x46,
                          };
void QHexflietoString(QByteArray pHexSrc,QString* pStrDst)
{
    __int32 i,Index,length;

    length = pHexSrc.size();
    Index = 0;
    while(length)
    {
        for(i=0;i<(16);i++)
        {
            unsigned char RdHex;
            RdHex  = char(pHexSrc[Index]);
            pStrDst->append(AsciiTable[RdHex>>4]);
            pStrDst->append(AsciiTable[RdHex&0x0f]);
            pStrDst->append(" ");

            Index++;
            length--;
            if(length == 0)
            {
                break;
            }
        }
        if(length)
        {
            pStrDst->append("\r\n");
        }
    }
}
//-----------------------------------------------------------------
void QHexOneUnittoString(QByteArray pHexSrc,QString* pStrDst)
{
    __int32 Index,length;

    length = pHexSrc.size();
    Index = 0;
    while(length)
    {
        unsigned char RdHex;
        RdHex  = char(pHexSrc[Index]);
        pStrDst->append(AsciiTable[RdHex>>4]);
        pStrDst->append(AsciiTable[RdHex&0x0f]);
        pStrDst->append(" ");

        Index++;
        length--;
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//报文字节处理程序
//入口: _In_Byte           收到的字节数据
//     _Out_MsgBuffer     返回报文的数据指针
//出口: 报文的长度
//-----------------------------------------------------------------
typedef enum
{
   MSS_Waitsycn         = 0x00,
   MSS_ReceiveData      = 0x01,

}MsgStatus_Def;
static MsgStatus_Def MsgStatus= MSS_Waitsycn;

//-----------------------------------------------------------------
void MainWindow::ReceiveTimeout_Done()
{
    MsgStatus = MSS_Waitsycn;

}
//-----------------------------------------------------------------
uint8_t MainWindow::MessageByteOperation(uint8_t _In_Byte,uint8_t* _Out_MsgBuffer)
{

    static uint8_t index;
    static uint8_t Buffer[256];
    static bool TimeCfged = false;
    uint8_t MsgLength;

    MsgLength = 0;
    if(TimeCfged)
    {

    }
    else
    {
        FrameTimeout = new QTimer();
        TimeCfged = true;
        QObject::connect(MainWindow::FrameTimeout,SIGNAL(timeout()),this,SLOT(ReceiveTimeout_Done()));
        FrameTimeout->setSingleShot(true);
    }
    switch(MsgStatus)
    {
       case MSS_Waitsycn:
       {
           if(0x68 == _In_Byte)
           {
               Buffer[0] = _In_Byte;
               index = 1;
               MsgStatus = MSS_ReceiveData;
               FrameTimeout->start(500);
           }
           break;
       }
       case MSS_ReceiveData:
       {
           Buffer[index] = _In_Byte;
           index++;
           FrameTimeout->start(500);
           if(index>10)
           {
               if(index>(Buffer[10]+12))  //一个完整的报文 index == size
               {
                   if((Buffer[index-2]==MakeMbusCheckSum(&Buffer[0],(index-2)))&&(0x16==Buffer[index-1]))
                   {
                       uint8_t i;
                       for(i=0;i<index;i++)
                       {
                           _Out_MsgBuffer[i] = Buffer[i];
                       }

                       MsgLength = index;
                       MsgStatus = MSS_Waitsycn;
                       FrameTimeout->stop();
                   }
               }
           }

           break;
       }
       default:
       {
           MsgStatus = MSS_Waitsycn;
           break;
       }
    }
    return(MsgLength);

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
uint8_t MakeMbusCheckSum(uint8_t* _In_CheckData,uint8_t _In_Length)
{
    uint8_t CheckSum,index;

    CheckSum = 0;
    index = 0;
    while(_In_Length)
    {
        CheckSum += _In_CheckData[index];
        index++;
        _In_Length--;
    }
    return(CheckSum);
}
//-----------------------------------------------------------------
void SetDelayTime(uint16_t* _Out_DelayClock,uint16_t _In_Millisecond)
{
    *_Out_DelayClock = _In_Millisecond/10;
}
//-----------------------------------------------------------------
typedef enum
{
    UpFuncode_Query         =0x01,  //查询设备状态
    UpFuncode_EntryBoot     =0x02,  //进入boot状态
    UpFuncode_Start         =0x03,  //重传功能
    UpFuncode_Resumer       =0x04,  //续传功能
    UpFuncode_Transmit      =0x05,  //传输内容
}UpdataFunCode_Def;
uint8_t CreatTxdMessage(UpdataFunCode_Def _In_FunCode, uint16_t _In_Number,uint8_t* _In_TrasmitData,uint8_t* _Out_TxdBuffer)
{

    if((_In_FunCode == UpFuncode_Transmit) || (_In_FunCode == UpFuncode_Start) || (_In_FunCode == UpFuncode_Resumer))
    {
        _Out_TxdBuffer[0]  = (0xfe);
        _Out_TxdBuffer[1]  = (0xfe);
        _Out_TxdBuffer[2]  = (0xfe);
        _Out_TxdBuffer[3]  = (0x68);
        _Out_TxdBuffer[4]  = (0x42);
        _Out_TxdBuffer[5]  = (0xAA);
        _Out_TxdBuffer[6]  = (0xAA);
        _Out_TxdBuffer[7]  = (0xAA);
        _Out_TxdBuffer[8]  = (0xAA);
        _Out_TxdBuffer[9]  = (0xAA);
        _Out_TxdBuffer[10] = (0xAA);
        _Out_TxdBuffer[11] = (0xAA);
        _Out_TxdBuffer[12] = (0x00);
        _Out_TxdBuffer[13] = 136;     //DataLength
        _Out_TxdBuffer[14] = (0x82);  //DM
        _Out_TxdBuffer[15] = (0x52);  //DM
        _Out_TxdBuffer[16] = (0x01);  //VER
        _Out_TxdBuffer[17] = (_In_FunCode); //CMD
        _Out_TxdBuffer[18] = (0xFF);
        _Out_TxdBuffer[19] = (0xFF);
        _Out_TxdBuffer[20] = (_In_Number>>8);
        _Out_TxdBuffer[21] = (_In_Number&0x00ff);

        unsigned char i;
        for(i=0;i<128;i++)
        {
            _Out_TxdBuffer[i+22] = _In_TrasmitData[i];
        }
        _Out_TxdBuffer[150] = MakeMbusCheckSum(&_Out_TxdBuffer[3],(150-3));  //CS
        _Out_TxdBuffer[151] =(0x16);  //16

        return(152);
    }
    else
    {
        _Out_TxdBuffer[0]  = (0xfe);
        _Out_TxdBuffer[1]  = (0xfe);
        _Out_TxdBuffer[2]  = (0xfe);
        _Out_TxdBuffer[3]  = (0x68);
        _Out_TxdBuffer[4]  = (0x42);
        _Out_TxdBuffer[5]  = (0xAA);
        _Out_TxdBuffer[6]  = (0xAA);
        _Out_TxdBuffer[7]  = (0xAA);
        _Out_TxdBuffer[8]  = (0xAA);
        _Out_TxdBuffer[9]  = (0xAA);
        _Out_TxdBuffer[10] = (0xAA);
        _Out_TxdBuffer[11] = (0xAA);
        _Out_TxdBuffer[12] = (0x00);
        _Out_TxdBuffer[13] = 8;       //DataLength
        _Out_TxdBuffer[14] = (0x82);  //DM
        _Out_TxdBuffer[15] = (0x52);  //DM
        _Out_TxdBuffer[16] = (0x01);  //VER
        _Out_TxdBuffer[17] = (_In_FunCode);
        _Out_TxdBuffer[18] = (0xFF);
        _Out_TxdBuffer[19] = (0xFF);
        _Out_TxdBuffer[20] = (_In_Number>>8);
        _Out_TxdBuffer[21] = (_In_Number&0x00ff);

        _Out_TxdBuffer[22] = MakeMbusCheckSum(&_Out_TxdBuffer[3],(22-3));  //CS
        _Out_TxdBuffer[23] =(0x16);  //16
        return(24);
    }
}
//-----------------------------------------------------------------
//-----------------------------------------------------------------
//数据收发处理
static enum
          {
             SerRSS_IDLE              = 0x00,
             SerRSS_Query             = 0x01,
             SerRSS_Query_RxdW        = 0x02,
             SerRSS_EntryBoot         = 0x03,
             SerRSS_EntryBoot_RxdW    = 0x04,
             SerRSS_Resume            = 0x05,
             SerRSS_Resume_RxdW       = 0x06,
             SerRSS_Start             = 0x07,
             SerRSS_Start_RxdW        = 0x08,
             SerRSS_TxdFiles          = 0x09,
             SerRSS_TxdFiles_RxdW     = 0x0A,
             SerRSS_End               = 0x0B,
          }SerialRunStatus;
static bool updateResumer;
//-----------------------------------------------------------------
#define  DevIsBooted       0x01
#define  DevIsUpdated      0x02
#define  DevIsFvError      0x04
#define  DevIsCsError      0x08
#define  DevIsVlSegQy      0x10//有效的块请求
#define  DevIsOtherErr     0x20
//-----------------------------------------------------------------
//读取接收到的数据
void MainWindow::Read_Data()
{
    QByteArray Rxd_Buffer;
    static uint8_t i;
    uint8_t Rxd_Length;
    uint8_t Msg_Buffer[256];

    Rxd_Buffer = serial->readAll();

    for(i = 0;i<Rxd_Buffer.size();i++)
    {
        if(0!=(Rxd_Length = MessageByteOperation((uint8_t)Rxd_Buffer[i],&Msg_Buffer[0])))
        {
            break;
        }
    }
    if(Rxd_Length)
    {
        switch(SerialRunStatus)
        {
            case SerRSS_Query_RxdW:
            {
                QString Disp_buff;
                Disp_buff.clear();
                Rxd_Buffer.clear();
                Rxd_Buffer.setRawData((char*)&Msg_Buffer[0],Rxd_Length);
                if((0x82 == Msg_Buffer[11])&& //DM
                   (0x52 == Msg_Buffer[12])&& //DM
                   (0x01 == Msg_Buffer[13])&& //VER
                   (0x81 == Msg_Buffer[14]))  //CMD

                {
                    if(0x10 == Msg_Buffer[15])  //BOOT状态
                    {
                        if(updateResumer)
                        {
                            SerialRunStatus = SerRSS_Resume;
                        }
                        else
                        {
                            SerialRunStatus = SerRSS_Start;
                        }
                    }
                    else
                    {
                        SerialRunStatus = SerRSS_EntryBoot;
                    }
                    Disp_buff.append("Rxd:Query\r\n");
                }
                else
                {
                    Disp_buff.append("Rxd:Error\r\n");
                }
                QHexOneUnittoString(Rxd_Buffer,&Disp_buff);
                ui->textEdit->append(Disp_buff);
                ui->textEdit->backgroundRole();
                break;
            }
            case SerRSS_EntryBoot_RxdW:
            {
                QString Disp_buff;
                Disp_buff.clear();
                Rxd_Buffer.clear();
                Rxd_Buffer.setRawData((char*)&Msg_Buffer[0],Rxd_Length);
                if((0x82 == Msg_Buffer[11])&& //DM
                   (0x52 == Msg_Buffer[12])&& //DM
                   (0x01 == Msg_Buffer[13])&& //VER
                   ((0x81 == Msg_Buffer[14])||(0x82 == Msg_Buffer[14])))  //CMD
                {
                  //  if(0x10 == Msg_Buffer[15])  //状态
                 //   {
                  //      if()
                 //       SerialRunStatus = SerRSS_TxdFMap;  //已经处于boot状态，发送FileMap
                  //  }
                  //  else
                   // {

                   // }
                    if(0x81 == Msg_Buffer[14])
                    {
                        SerialRunStatus = SerRSS_Query;
                        Disp_buff.append("Rxd:Query\r\n");
                    }
                    else
                    {
                        Disp_buff.append("Rxd:Goto Boot\r\n");
                    }
                }
                else
                {
                    Disp_buff.append("Rxd:Error\r\n");
                }
                QHexOneUnittoString(Rxd_Buffer,&Disp_buff);
                ui->textEdit->append(Disp_buff);
                ui->textEdit->backgroundRole();
                break;
            }
            case SerRSS_Start_RxdW:  //
            case SerRSS_Resume_RxdW:
            case SerRSS_TxdFiles_RxdW:
            {
                updateResumer = true;
                QString Disp_buff;
                Disp_buff.clear();
                Rxd_Buffer.clear();
                Rxd_Buffer.setRawData((char*)&Msg_Buffer[0],Rxd_Length);
                if((0x82 == Msg_Buffer[11])&& //DM
                   (0x52 == Msg_Buffer[12])&& //DM
                   (0x01 == Msg_Buffer[13])&& //VER
                   ((0x83 == Msg_Buffer[14])||(0x84 == Msg_Buffer[14])||(0x85 == Msg_Buffer[14])))  //CMD
                {
                    if(0x1F == Msg_Buffer[15])  //状态
                    {
                        SerialRunStatus = SerRSS_IDLE;     //完成
                        ui->pushButton_Start->click();
                        Disp_buff.append("Rxd:Updata Finish\r\n");
                    }
                    else if(0x10 == Msg_Buffer[15]) //
                    {

                    }
                    else if(0x11 == Msg_Buffer[15])
                    {
                        SerialRunStatus = SerRSS_TxdFiles;  //发送文件
                        Disp_buff.append("Rxd:Map\r\n");
                    }
                    else if(0x12 == Msg_Buffer[15]) //请求file内容
                    {
                        MainWindow::DevQueryNumber = Msg_Buffer[17];
                        MainWindow::DevQueryNumber = MainWindow::DevQueryNumber<<8;
                        MainWindow::DevQueryNumber|= ((uint16_t)Msg_Buffer[18]&0x00ff);

                        SerialRunStatus = SerRSS_TxdFiles;  //发送文件
                        Disp_buff.append("Rxd:Files\r\n");
                    }
                    else
                    {
                        SerialRunStatus = SerRSS_IDLE;     //发送错误退出
                        ui->pushButton_Start->click();
                        Disp_buff.append("Rxd:StatusBitsError\r\n");
                    }

                }
                else
                {
                    if(0xFE == Msg_Buffer[14])
                    {
                        if(0x01 == Msg_Buffer[16]) //ERR
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:length Error\r\n");
                        }
                        else if(0x02 == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:protocol version Error\r\n");
                        }
                        else if(0x03 == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:Command Error\r\n");
                        }
                        else if(0x04 == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:FileMAP Checksum Error\r\n");
                        }
                        else if(0x05 == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:File Checksum Error\r\n");
                        }
                        else if(0x06 == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:Number Error\n");
                        }
                        else if(0x07 == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:Fireware Version Error\n");
                        }
                        else if(0x08 == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //错误退出
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:File Map Version Error\n");
                        }
                        else if(0xfe == Msg_Buffer[16])
                        {
                            SerialRunStatus = SerRSS_IDLE;     //
                            ui->pushButton_Start->click();
                            Disp_buff.append("Rxd:Other Error\r\n");
                        }
                        else
                        {
                            Disp_buff.append("Rxd:Error\r\n");
                        }
                    }

                }
                QHexOneUnittoString(Rxd_Buffer,&Disp_buff);
                ui->textEdit->append(Disp_buff);
                ui->textEdit->backgroundRole();
                break;
            }

            default:
            {
                break;
            }
        }
    }

}
//-----------------------------------------------------------------
//-----------------------------------------------------------------
//定时器运行 10ms，发送数据
void MainWindow::Timer_Done()
{
    uint8_t Read_Transmit[256],TxdBuffer[256];
    uint8_t Txd_Length;
    uint8_t i;


    if(0 != MainWindow::Query_Interval)
    {
        MainWindow::Query_Interval --;
    }
    if(0 != MainWindow::TRxd_Timeout)
    {
        MainWindow::TRxd_Timeout --;
    }
    if(serial->isOpen())
    {
        switch(SerialRunStatus)
        {
            case SerRSS_IDLE:
            {
                break;
            }
            case SerRSS_Query:
            {
                if(0 == MainWindow::Query_Interval)
                {
                    Txd_Length = CreatTxdMessage(UpFuncode_Query,0x0000,&Read_Transmit[0],&TxdBuffer[0]);

                    QByteArray Qtuf;
                    Qtuf.setRawData((char*)&TxdBuffer[0],Txd_Length);
                    serial->write(Qtuf);

                    QString Disp_buff;
                    Disp_buff.clear();

                    Disp_buff.append("Txd:Query\r\n");
                    QHexOneUnittoString(Qtuf,&Disp_buff);

                    ui->textEdit->append(Disp_buff);
                    ui->textEdit->backgroundRole();
                    SerialRunStatus = SerRSS_Query_RxdW;
                    SetDelayTime(&(MainWindow::TRxd_Timeout),2000);
                }
                break;
            }
            case SerRSS_EntryBoot:
            {
                Txd_Length = CreatTxdMessage(UpFuncode_EntryBoot,0x0000,&Read_Transmit[0],&TxdBuffer[0]);

                QByteArray Qtuf;
                Qtuf.setRawData((char*)&TxdBuffer[0],Txd_Length);
                serial->write(Qtuf);

                QString Disp_buff;
                Disp_buff.clear();

                Disp_buff.append("Txd:EntryBoot\r\n");
                QHexOneUnittoString(Qtuf,&Disp_buff);

                ui->textEdit->append(Disp_buff);
                ui->textEdit->backgroundRole();
                SerialRunStatus = SerRSS_EntryBoot_RxdW;
                SetDelayTime(&(MainWindow::TRxd_Timeout),2000);
                break;
            }
            case SerRSS_Resume:
            {
                for(i=0;i<128;i++)
                {
                    Read_Transmit[i] = MainWindow::ReadFlieData[i];
                }
                Txd_Length = CreatTxdMessage(UpFuncode_Resumer,0x0000,&Read_Transmit[0],&TxdBuffer[0]);

                QByteArray Qtuf;
                Qtuf.setRawData((char*)&TxdBuffer[0],Txd_Length);
                serial->write(Qtuf);

                QString Disp_buff;
                Disp_buff.clear();

                Disp_buff.append("Txd:Resume FMap\r\n");
                QHexOneUnittoString(Qtuf,&Disp_buff);

                ui->textEdit->append(Disp_buff);
                ui->textEdit->backgroundRole();
                SerialRunStatus = SerRSS_Resume_RxdW;
                SetDelayTime(&(MainWindow::TRxd_Timeout),2000);
                break;
            }
            case SerRSS_Start:
            {
                for(i=0;i<128;i++)
                {
                    Read_Transmit[i] = MainWindow::ReadFlieData[i];
                }
                Txd_Length = CreatTxdMessage(UpFuncode_Start,0x0000,&Read_Transmit[0],&TxdBuffer[0]);

                QByteArray Qtuf;
                Qtuf.setRawData((char*)&TxdBuffer[0],Txd_Length);
                serial->write(Qtuf);

                QString Disp_buff;
                Disp_buff.clear();

                Disp_buff.append("Txd:Start FMap\r\n");
                QHexOneUnittoString(Qtuf,&Disp_buff);

                ui->textEdit->append(Disp_buff);
                ui->textEdit->backgroundRole();
                SerialRunStatus = SerRSS_Start_RxdW;
                SetDelayTime(&(MainWindow::TRxd_Timeout),2000);
                break;
            }
            case SerRSS_TxdFiles:
            {
                if(MainWindow::DevQueryNumber < (MainWindow::FileTotalSegment))
                {
                    for(i=0;i<128;i++)
                    {
                        Read_Transmit[i] = MainWindow::ReadFlieData[MainWindow::DevQueryNumber*128+i];
                    }
                    Txd_Length = CreatTxdMessage(UpFuncode_Transmit,MainWindow::DevQueryNumber,&Read_Transmit[0],&TxdBuffer[0]);

                    QByteArray Qtuf;
                    Qtuf.setRawData((char*)&TxdBuffer[0],Txd_Length);
                    serial->write(Qtuf);

                    QString Disp_buff;
                    Disp_buff.clear();

                    Disp_buff.append("Txd:Files_");
                    Disp_buff.append(QString("%1").arg(MainWindow::DevQueryNumber,4,10,QChar('0')));
                    Disp_buff.append("\r\n");
                    QHexOneUnittoString(Qtuf,&Disp_buff);

                    ui->textEdit->append(Disp_buff);
                    ui->textEdit->backgroundRole();
                    SerialRunStatus = SerRSS_TxdFiles_RxdW;
                    SetDelayTime(&(MainWindow::TRxd_Timeout),2000);
                }
                else
                {
                    SerialRunStatus = SerRSS_IDLE;
                    QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Query Number Error!"), QMessageBox::Ok, this);
                                msgBox.exec();
                }
                break;
            }
            case SerRSS_Query_RxdW:
            case SerRSS_EntryBoot_RxdW:
            case SerRSS_Resume_RxdW:
            case SerRSS_Start_RxdW:
            case SerRSS_TxdFiles_RxdW:
            {
                if(0 == (MainWindow::TRxd_Timeout))
                {
                    SerialRunStatus = SerRSS_Query;
                }
                break;
            }
            default:
            {
                break;
            }
        }

    }
}

void MainWindow::on_pushButton_Open_clicked()
{
    QString filename_Org;
    filename_Org = QFileDialog::getOpenFileName(this,
                                                "Open File",
                                                "",
                                                "Binary Files (*.tuf);;All Files (*.*?)");
    if(filename_Org.isNull())
    {
        return;
    }
    ui->InFileLine->setText(filename_Org);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//-----------------------------------------------------------------
void MainWindow::on_pushButton_Start_clicked()
{
    if(ui->pushButton_Start->text() ==QString("Start"))
    {
        if(ui->InFileLine->text().isEmpty())
        {
            QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please Open File!"), QMessageBox::Ok, this);
                        msgBox.exec();
        }
        else if(!serial->isOpen())
        {
            QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please Open COM!"), QMessageBox::Ok, this);
                        msgBox.exec();

        }
        else
        {
             QFile Readtuf(ui->InFileLine->text());
             if(Readtuf.open(QIODevice::ReadOnly))
             {
                 //static __int32 Count;
                 MainWindow::ReadFlieData = Readtuf.readAll();
                 Readtuf.close();
                 __int32 ChecksumV;
                 __int32 ChecksumR;
                 char *pRdData = MainWindow::ReadFlieData.data();
                 ChecksumV = MakeCheckSumText(&pRdData[0],124);

                 ChecksumR = (__int32)pRdData[124]&0x000000ff;
                 ChecksumR = ChecksumR<<8;
                 ChecksumR = ChecksumR|((__int32)pRdData[124+1]&0x000000ff);
                 ChecksumR = ChecksumR<<8;
                 ChecksumR = ChecksumR|((__int32)pRdData[124+2]&0x000000ff);
                 ChecksumR = ChecksumR<<8;
                 ChecksumR = ChecksumR|((__int32)pRdData[124+3]&0x000000ff);

                 qint32 CrcMakeV,CrcRead;

                 CrcRead = (__int32)pRdData[116]&0x000000ff;
                 CrcRead = CrcRead<<8;
                 CrcRead = CrcRead|((__int32)pRdData[116+1]&0x000000ff);
                 CrcRead = CrcRead<<8;
                 CrcRead = CrcRead|((__int32)pRdData[116+2]&0x000000ff);
                 CrcRead = CrcRead<<8;
                 CrcRead = CrcRead|((__int32)pRdData[116+3]&0x000000ff);

                 QByteArray SrcData;
                 SrcData = QByteArray(&pRdData[128],(ReadFlieData.size()-128));
                 CrcMakeV = JQChecksum::crc32( SrcData );

                 if(CrcMakeV != CrcRead)
                 {
                     QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("CRC32 is Error"), QMessageBox::Ok, this);
                                 msgBox.exec();
                 }
                 else if(ChecksumV!= ChecksumR)
                 {
                     QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("CheckSum is Error"), QMessageBox::Ok, this);
                                 msgBox.exec();
                 }
                 else
                 {
                     MainWindow::FileTotalSegment = MainWindow::ReadFlieData.size()/128;

                     /*
                     QString Disp_buff;
                     Disp_buff.clear();
                     ReadFlieData.toHex(0);
                     QHexflietoString(ReadFlieData,&Disp_buff);
                     ui->textEdit->append(Disp_buff);
                     ui->textEdit->append(QString("%1").arg(ReadFlieData.size()));
                     ui->textEdit->append("1111");
                     ui->textEdit->append("2222");
                     Count++;
                     ui->textEdit->append(QString("%1").arg(Count,5,10,QChar('0')));

                     ui->textEdit->backgroundRole();
                     */
                     serial->clear();
                     SerialRunStatus = SerRSS_Query;
                     MainWindow::Query_Interval = 0;
                     updateResumer = false;
                     ui->pushButton_Start->setText("Stop");
                     ui->pushButton_3->setEnabled(false);
                 }
             }
        }
    }
    else
    {
        SerialRunStatus = SerRSS_IDLE;
        ui->pushButton_Start->setText("Start");
        ui->pushButton_3->setEnabled(true);
    }

}

void MainWindow::on_pushButton_3_clicked()
{

    if(ui->InFileLine->text().isEmpty())
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please Open File!"), QMessageBox::Ok, this);
                    msgBox.exec();
    }
    else if(!serial->isOpen())
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please Open COM!"), QMessageBox::Ok, this);
                    msgBox.exec();

    }
    else
    {
         QFile Readtuf(ui->InFileLine->text());
         if(Readtuf.open(QIODevice::ReadOnly))
         {
             //static __int32 Count;
             MainWindow::ReadFlieData = Readtuf.readAll();
             Readtuf.close();
             __int32 ChecksumV;
             __int32 ChecksumR;
             char *pRdData = MainWindow::ReadFlieData.data();
             ChecksumV = MakeCheckSumText(&pRdData[0],124);

             ChecksumR = (__int32)pRdData[124]&0x000000ff;
             ChecksumR = ChecksumR<<8;
             ChecksumR = ChecksumR|((__int32)pRdData[124+1]&0x000000ff);
             ChecksumR = ChecksumR<<8;
             ChecksumR = ChecksumR|((__int32)pRdData[124+2]&0x000000ff);
             ChecksumR = ChecksumR<<8;
             ChecksumR = ChecksumR|((__int32)pRdData[124+3]&0x000000ff);

             qint32 CrcMakeV,CrcRead;

             CrcRead = (__int32)pRdData[116]&0x000000ff;
             CrcRead = CrcRead<<8;
             CrcRead = CrcRead|((__int32)pRdData[116+1]&0x000000ff);
             CrcRead = CrcRead<<8;
             CrcRead = CrcRead|((__int32)pRdData[116+2]&0x000000ff);
             CrcRead = CrcRead<<8;
             CrcRead = CrcRead|((__int32)pRdData[116+3]&0x000000ff);

             QByteArray SrcData;
             SrcData = QByteArray(&pRdData[128],(ReadFlieData.size()-128));
             CrcMakeV = JQChecksum::crc32( SrcData );

             if(CrcMakeV != CrcRead)
             {
                 QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("CRC32 is Error"), QMessageBox::Ok, this);
                             msgBox.exec();
             }
             else if(ChecksumV!= ChecksumR)
             {
                 QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("CheckSum is Error"), QMessageBox::Ok, this);
                             msgBox.exec();
             }
             else
             {
                 MainWindow::FileTotalSegment = MainWindow::ReadFlieData.size()/128;

                 /*
                 QString Disp_buff;
                 Disp_buff.clear();
                 ReadFlieData.toHex(0);
                 QHexflietoString(ReadFlieData,&Disp_buff);
                 ui->textEdit->append(Disp_buff);
                 ui->textEdit->append(QString("%1").arg(ReadFlieData.size()));
                 ui->textEdit->append("1111");
                 ui->textEdit->append("2222");
                 Count++;
                 ui->textEdit->append(QString("%1").arg(Count,5,10,QChar('0')));

                 ui->textEdit->backgroundRole();
                 */
                 serial->clear();
                 SerialRunStatus = SerRSS_Query;
                 MainWindow::Query_Interval = 0;
                 updateResumer = true;
                 ui->pushButton_3->setEnabled(false);
                 ui->pushButton_Start->setText("Stop");
             }
         }
     }
}

void MainWindow::on_pushButton_6_clicked()
{
    ui->textEdit->clear();
}
