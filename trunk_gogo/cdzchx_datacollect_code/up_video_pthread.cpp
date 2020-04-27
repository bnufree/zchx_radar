#include "up_video_pthread.h"
#include <QThread>
#include <QFile>
#include <QString>
#include <QDialog>
#include <QFileDialog>
#include "Log.h"
#include <QDirIterator>

up_video_pthread::up_video_pthread(QString str,QString filename,QObject *parent) : type(str),file_name(filename),QThread(parent)
{

}

up_video_pthread::~up_video_pthread()
{
    if(this->isRunning())
    {
        this->quit();
    }
    this->terminate();
}
void up_video_pthread::run()
{
//    QString file_name = QFileDialog::getOpenFileName(NULL,"导入回波文件","../回波数据/","*");
    readAllFile(file_name);

    exec();
}
//按顺序读取回波文件
void up_video_pthread::readAllFile(QString path)
{
    QDir dir(path);
    if(!dir.exists())
    {
        return;
    }

    //获取所选文件类型过滤器
    QStringList filters;
    filters<<QString("*.dat");

    while(dir.exists())
    {
        //定义迭代器并设置过滤器
        QDirIterator dir_iterator(path, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        QStringList string_list;

        while(dir_iterator.hasNext())
        {
            dir_iterator.next();
            QFileInfo file_info = dir_iterator.fileInfo();
            QString file_path = file_info.absoluteFilePath();
            string_list.append(file_path);
            ZCHX_LOG_OUT("file_path")<<file_path;
            QFile video_file(file_path);
            if(!video_file.open(QIODevice::ReadOnly)){
                   ZCHX_LOG_OUT("打开失败");
               }else{
                   ZCHX_LOG_OUT("打开成功");
                   //操作文件
                   int a =17160;
                   while (1)
                   {
                       QByteArray video_array;
                       video_array = video_file.read(17160);/*
                                  cout<<"回波原始数据大小"<<video_array.size();
                                  cout<<"发送的内容"<<QString(video_array).toLatin1();*/
                       a = video_array.size();
                       if(a != 0)
                       {
                           msleep(50);
//                           cout<<"发送回波数据  size:"<<a;
                           emit send_video_signal(video_array,"zchx240",4096,512,0);
                       }
                       if(video_file.atEnd())
                       {
                           ZCHX_LOG_OUT("str:")<<file_path;
                           break;
                       }

                   }
                   //关闭文件
                   video_file.close();

                }


        }


    }

}
