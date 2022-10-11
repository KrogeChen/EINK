#include <QObject>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox->clear();
    mcu_info_list.clear();

    //加载 config.json 文件
    QString qConfigPath = QDir::currentPath() + "/config.json";
    QFile jsFile(qConfigPath);
    if(jsFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QByteArray jsData = jsFile.readAll();
        jsFile.close();//关闭文件
        if(jsData.size() > 0)
        {
            //JSON解析
            qint32 i,linecount;
            mcuinfo mcuItem;
            QJsonParseError error ;
            QJsonObject JsConfigRoot;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(jsData, &error);//解析
            if((error.error == QJsonParseError::NoError)&&(jsonDocument.isObject()))//解析正确
            {
                JsConfigRoot = jsonDocument.object();
                if(JsConfigRoot.contains("mculist"))
                {
                    QJsonArray js_array = JsConfigRoot.value("mculist").toArray();
                    linecount = js_array.count();
                    for(i = 0; i < linecount; i++)
                    {
                        QJsonObject JsItem           = js_array[i].toObject();
                        mcuItem.model                = JsItem.value("model").toString();
                        mcuItem.boot                 = JsItem.value("boot").toString();
                        mcuItem.app                  = JsItem.value("app").toString();
                        //mcuItem.upgrade              = JsItem.value("upgrade").toString();
                        mcuItem.boot_max_sizeofbyte  = JsItem.value("boot_max_sizeofbyte").toInt();
                        mcuItem.app_max_sizeofbyte   = JsItem.value("app_max_sizeofbyte").toInt();
                        mcuItem.flash_addr_str       = JsItem.value("flash_addr_str").toInt();
                        mcuItem.flash_resver_data    = JsItem.value("flash_resver_data").toInt();

                        //添加
                        mcu_info_list.append(mcuItem);
                        ui->comboBox->addItem(mcuItem.model);
                    }
                }
            }
        }
    }

    if(0 == ui->comboBox->count())
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Load config.json failed !!"), QMessageBox::Ok, this);
                    msgBox.exec();
        return;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    ui->chipaddr->clear();
    if((index >= 0)&&(index < mcu_info_list.count()))
    {
        ui->chipaddr->append("MCU:     " + mcu_info_list[index].model);
        ui->chipaddr->append("boot：   " + mcu_info_list[index].boot);
        ui->chipaddr->append("app:     " + mcu_info_list[index].app);
        //ui->chipaddr->append("upgrade：" + mcu_info_list[index].upgrade);
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//创建合并后的hex文件
//-------------------------------------------------------------------------------------
void MainWindow::on_pushButton_CreatMerge_clicked()
{
//-------------------------------------------------------------------------------------
    if(boot_file_name.isNull())
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please select Boot BinFile"), QMessageBox::Ok, this);
                    msgBox.exec();
        return;
    }
    if(app_file_name.isNull())
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Please select App BinFile"), QMessageBox::Ok, this);
                    msgBox.exec();
        return;
    }
//-------------------------------------------------------------------------------------
    unsigned char* pBootArray;
    unsigned char* pAppArray;
    unsigned int BootBinlength;
    unsigned int AppBinlength;
    unsigned int boot_max_sizeofbyte = 0;
    unsigned int app_max_sizeofbyte = 0;

    QByteArray FilePath=boot_file_name.toLocal8Bit();
    FILE* QReadBoot = fopen(FilePath.data(),"rb");

    if(QReadBoot)
    {
        fseek(QReadBoot, 0, SEEK_END);
        BootBinlength = ftell(QReadBoot);

        pBootArray =(unsigned char*)malloc(BootBinlength);
        memset(pBootArray,0,BootBinlength);
        fseek(QReadBoot, 0, SEEK_SET);
        if(BootBinlength!=fread(pBootArray,1,BootBinlength,QReadBoot))  //读取文件到内存
        {
            free(pBootArray);
            fclose(QReadBoot);
            QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Boot File sRead Error "), QMessageBox::Ok, this);
                        msgBox.exec();
            return;
        }
        else
        {
            fclose(QReadBoot);  //读取文件完毕，关闭文件
        }
    }
    else
    {
        fclose(QReadBoot);
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Do'not Open Boot File "), QMessageBox::Ok, this);
                    msgBox.exec();
        return;
    }
//-------------------------------------------------------------读取APP文件
    FilePath.clear();
    FilePath=app_file_name.toLocal8Bit();
    FILE* QReadApp = fopen(FilePath.data(),"rb");


    if(QReadApp)
    {
        fseek(QReadApp, 0, SEEK_END);
        AppBinlength = ftell(QReadApp);


        pAppArray =(unsigned char*)malloc(AppBinlength);
        memset(pAppArray,0,AppBinlength);
        fseek(QReadApp, 0, SEEK_SET);
        if(AppBinlength!=fread(pAppArray,1,AppBinlength,QReadApp))  //读取文件到内存
        {
            free(pAppArray);
            fclose(QReadApp);
            QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("App File sRead Error "), QMessageBox::Ok, this);
                        msgBox.exec();
            return;
        }
        else
        {
            fclose(QReadApp);  //读取文件完毕，关闭文件
        }
    }
    else
    {
        fclose(QReadApp);
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("Do'not Open App File "), QMessageBox::Ok, this);
                    msgBox.exec();
        return;
    }
//-------------------------------------------------------------------------------------
    unsigned int flash_addr_str = 0;   //根据MCU选择分区大小和保留数据
    unsigned char flash_resver_data = 0;

    int crtindex = ui->comboBox->currentIndex();
    if((crtindex < 0)||(crtindex >= mcu_info_list.count()))
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("index error !!"), QMessageBox::Ok, this);
                    msgBox.exec();
        return;
    }

    boot_max_sizeofbyte = mcu_info_list[crtindex].boot_max_sizeofbyte;
    app_max_sizeofbyte  = mcu_info_list[crtindex].app_max_sizeofbyte;
    flash_addr_str      = mcu_info_list[crtindex].flash_addr_str;
    flash_resver_data   = mcu_info_list[crtindex].flash_resver_data;

//------------------------------------------------------------------------------------
    QByteArray MergeQbyte;
    mergeBinaryData theMergeBin;
    binaryTransfer_def the_binTransfer;
    bool mergeIsRight;

    the_binTransfer.app_max_size_byte = app_max_sizeofbyte;
    the_binTransfer.boot_max_size_byte = boot_max_sizeofbyte;
    the_binTransfer.in_bootArea_length = BootBinlength;
    the_binTransfer.in_pBootArea_data = pBootArray;
    the_binTransfer.in_appArea_length = AppBinlength;
    the_binTransfer.in_pAppArea_data = pAppArray;

    mergeIsRight = theMergeBin.mergeBinary_createMerge(the_binTransfer,&MergeQbyte,flash_resver_data);
    free(pAppArray);
    free(pBootArray);

    if(false == mergeIsRight)
    {
        QMessageBox msgBox(QMessageBox::Warning, QString::fromUtf8("Error"),QString::fromUtf8("File Length Error "), QMessageBox::Ok, this);
                    msgBox.exec();
        return;
    }
//-------------------------------------------------------------------------------------
    QString HexSting;
    binaryToHexString bToHexString;
    bToHexString.create_hexString(flash_addr_str,MergeQbyte,&HexSting);   //创建HEX格式的字符串
    QByteArray HexStrRaw = HexSting.toUtf8();

    QString MakeFileName;
    QFileInfo fileinf;

    fileinf = QFileInfo(app_file_name);
    MakeFileName = QDir::currentPath();
    MakeFileName.append("/");
    MakeFileName.append(fileinf.fileName());
    MakeFileName.remove(".bin");
    MakeFileName.append(".hex");
    QByteArray Make_Filepath=MakeFileName.toLocal8Bit();
    FILE* QMakebin = fopen(Make_Filepath.data(),"w+b");
    fwrite(HexStrRaw.data(),1,HexStrRaw.size(),QMakebin);
    fclose(QMakebin);
    if(MakeFileName.isNull())
    {
        ui->lineEdit_Merge->setText("This Make is Faild");
    }
    else
    {
        ui->lineEdit_Merge->setText(MakeFileName);
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void MainWindow::on_pushButton_OpenBoot_clicked()
{
    boot_file_name = QFileDialog::getOpenFileName(this,
                                                "Open File",
                                                "",
                                                "Binary Files (*.bin);;All Files (*.*?)");
    if(boot_file_name.isNull())
    {
        return;
    }
    ui->lineEdit_Boot->setText(boot_file_name);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void MainWindow::on_pushButton_OpenApp_clicked()
{
    app_file_name = QFileDialog::getOpenFileName(this,
                                                "Open File",
                                                "",
                                                "Binary Files (*.bin);;All Files (*.*?)");
    if(app_file_name.isNull())
    {
        return;
    }
    ui->lineEdit_App->setText(app_file_name);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void MainWindow::on_pushButton_OpenFolder_clicked()
{
    QString CurrentDir;
    CurrentDir = QDir::currentPath();
    QDesktopServices::openUrl(CurrentDir);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void MainWindow::on_pushButton_ClearMerge_clicked()
{
    QFile::remove(ui->lineEdit_Merge->text());
    ui->lineEdit_Merge->clear();
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

