#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_open_clicked()
{
    filename_Org = QFileDialog::getOpenFileName(this,
                                                "Open File",
                                                "",
                                                "Binary Files (*.bin);;All Files (*.*?)");
    if(filename_Org.isNull())
    {
        return;
    }
    ui->InFileLine->setText(filename_Org);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//#define KEYVAL  0x89,0x8E,0x3C,0x72,0xA7,0x19,0xF4,0x5D
//--------------------------------------------------------------------------------
void Encrypt(char* binf,__int32 Length)
{
    char Key[8]={(char)0x89,(char)0x8E,(char)0x3C,(char)0x72,(char)0xA7,(char)0x19,(char)0xF4,(char)0x5D,};
    unsigned char i;
    __int32 BaseIndex;

    BaseIndex = 0;
    while(0!=Length)
    {
        Key[0] = (char)0x89;
        Key[1] = (char)0x8E;
        Key[2] = (char)0x3C;
        Key[3] = (char)0x72;
        Key[4] = (char)0xA7;
        Key[5] = (char)0x19;
        Key[6] = (char)0xF4;
        Key[7] = (char)0x5D;

        for(i=0 ;i<16 ;i++)
        {
            binf[0+i*8+BaseIndex] = binf[0+i*8+BaseIndex] ^ Key[0];
            binf[1+i*8+BaseIndex] = binf[1+i*8+BaseIndex] ^ Key[1];
            binf[2+i*8+BaseIndex] = binf[2+i*8+BaseIndex] ^ Key[2];
            binf[3+i*8+BaseIndex] = binf[3+i*8+BaseIndex] ^ Key[3];
            binf[4+i*8+BaseIndex] = binf[4+i*8+BaseIndex] ^ Key[4];
            binf[5+i*8+BaseIndex] = binf[5+i*8+BaseIndex] ^ Key[5];
            binf[6+i*8+BaseIndex] = binf[6+i*8+BaseIndex] ^ Key[6];
            binf[7+i*8+BaseIndex] = binf[7+i*8+BaseIndex] ^ Key[7];


            char lastkeybit;
            if(Key[0] & 0x80)
            {
                lastkeybit = 0x01;
            }
            else
            {
                lastkeybit = 0x00;
            }
            unsigned char ics;
            for(ics=0;ics<7;ics++)
            {
                Key[ics] = Key[ics]<<1;
                if(Key[ics+1] & 0x80)
                {
                    Key[ics] |= 0x01;
                }
            }
            Key[ics] = Key[ics]<<1;
            Key[ics] |= lastkeybit;

        }
        Length = Length-128;
        BaseIndex += 128;
    }

}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define CheckSumWordEven          0x39ea2e76
#define CheckSumWordOdd           0x82b453c3
#define VersionTuf                0x00000001
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
__int32 MakeOneCheckText(__int32 Data,__int32 Count)
{
    if(Count&0x00000001)
    {
        Data=Data^CheckSumWordOdd;
    }
    else
    {
        Data=Data^CheckSumWordEven;
    }

    return(Data);
}
//--------------------------------------------------------------------------------
__int32 MakeCheckSumText(char* pData,__int32 Length)
{
    __int32 iWords,This_ids;
    __int32 Index;
    __int32 MakeSum,ReadData;
    iWords = (Length)/4;
    This_ids = 0;
    MakeSum = 0;
    Index = 0;
    while(iWords)
    {
        ReadData = (__int32)pData[Index]&0x000000ff;
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32)pData[Index+1]&0x000000ff);
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32)pData[Index+2]&0x000000ff);
        ReadData = ReadData<<8;
        ReadData = ReadData|((__int32)pData[Index+3]&0x000000ff);
        MakeSum+=MakeOneCheckText(ReadData,This_ids);
        Index = Index+4;
        iWords--;
        This_ids++;
    }
    return(MakeSum);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
QString Making_BinFile(QString The_Filename)
{
    QString MakeFileName;
    QFileInfo fileinf;

    QByteArray Filespath=The_Filename.toLocal8Bit();

    FILE* QReadBin = fopen(Filespath.data(),"rb");

    if(QReadBin)
    {
         fseek(QReadBin, 0, SEEK_END);
         unsigned long Rdbinlength = ftell(QReadBin);
         if(0==Rdbinlength)
         {
             fclose(QReadBin);
         }
         else
         {
             unsigned long PoilishLength;
             unsigned long MallocLength;

             if(0==(Rdbinlength%128))
             {
                 PoilishLength = 0;
             }
             else
             {
                 PoilishLength = (128-(Rdbinlength%128));
             }

             MallocLength = Rdbinlength +PoilishLength+128;
             fseek(QReadBin, 0, SEEK_SET);
             char* Bindata = (char *)malloc(MallocLength);
             memset(Bindata,0x00,MallocLength);
             if(Rdbinlength != fread((Bindata+128),1,Rdbinlength,QReadBin))
             {
             }
             else
             {
                 unsigned long i;
                 for(i = 0;i < PoilishLength;i++ )
                 {
                     //memset(&Bindata[MallocLength-PoilishLength+i],0xff,1);
                     Bindata[MallocLength-PoilishLength+i] = 0xff;
                 }

                 Bindata[0] = 'n';
                 Bindata[1] = 'a';
                 Bindata[2] = 'm';
                 Bindata[3] = 'e';
                 Bindata[4] = ':';

                 __int32 CheckSum;
                 CheckSum = MakeCheckSumText(&Bindata[128],(MallocLength-128));

                 Bindata[108] = CheckSum>>24;
                 Bindata[109] = CheckSum>>16;
                 Bindata[110] = CheckSum>>8;
                 Bindata[111] = CheckSum;

                 __int32 MsgLengthByte;
                 MsgLengthByte = (MallocLength-128);
                 Bindata[112] = MsgLengthByte>>24;
                 Bindata[113] = MsgLengthByte>>16;
                 Bindata[114] = MsgLengthByte>>8;
                 Bindata[115] = MsgLengthByte;

                 Encrypt((&Bindata[128]),(MallocLength-128));


                 QByteArray SrcData;
                 SrcData = QByteArray(&Bindata[128],(MallocLength-128));
                 __int32 crc32 = JQChecksum::crc32( SrcData );

                 Bindata[116] = crc32 >> 24; //Encrypt area CRC32
                 Bindata[117] = crc32 >> 16;
                 Bindata[118] = crc32 >> 8;
                 Bindata[119] = crc32;

                 Bindata[120] = VersionTuf>>24;  //Version
                 Bindata[121] = VersionTuf>>16;
                 Bindata[122] = VersionTuf>>8;
                 Bindata[123] = VersionTuf;

                 CheckSum = MakeCheckSumText(&Bindata[0],124);
                 Bindata[124] = CheckSum >>24;
                 Bindata[125] = CheckSum >>16;
                 Bindata[126] = CheckSum >>8;
                 Bindata[127] = CheckSum;

                 fileinf = QFileInfo(The_Filename);
                 MakeFileName = QDir::currentPath();
                 MakeFileName.append("/");
                 MakeFileName.append(fileinf.fileName());
                 MakeFileName.remove(".bin");
                 MakeFileName.append(".tuf");

                 //MakeFileName = QDir::currentPath();
                 //MakeFileName.append("/ForRelease.tuf");
                 QByteArray Make_Filepath=MakeFileName.toLocal8Bit();
                 FILE* QMakebin = fopen(Make_Filepath.data(),"w+b");
                 fwrite(&Bindata[0],1,MallocLength,QMakebin);
                 fclose(QMakebin);
             }
             free(Bindata);
             fclose(QReadBin);
         }
    }
    else
    {
        fclose(QReadBin);
    }

    return(MakeFileName);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void MainWindow::on_pushButton_Creating_clicked()
{
    QString MakeFileName;


    if(filename_Org.isNull())
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please select BinFile"), QMessageBox::Ok, this);
                    msgBox.exec();
    }
    else
    {
        MakeFileName = Making_BinFile(filename_Org);

        if(MakeFileName.isNull())
        {
            ui->OutFileLine->setText("This Make is Faild");
        }
        else
        {
            ui->OutFileLine->setText(MakeFileName);
        }
    }
}

void MainWindow::on_pushButton_OpenFolder_clicked()
{
    QString CurrentDir;
    CurrentDir = QDir::currentPath();
    QDesktopServices::openUrl(CurrentDir);
}

void MainWindow::on_pushButton_Clear_clicked()
{

    QFile::remove(ui->OutFileLine->text());
    ui->OutFileLine->clear();
}
