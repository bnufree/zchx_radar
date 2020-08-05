#include "zchxsimulatethread.h"
#include <QDir>
#include <QDirIterator>
#include <QDebug>

zchxSimulateThread::zchxSimulateThread(const QString& dirName,  int default_len, QObject *parent)
    : QThread(parent)
    , mDirName(dirName)
    , mDefaultBytesLen(default_len)
    , mCancelFlg(false)
{

}

void zchxSimulateThread::run()
{
    QDir dir(mDirName);
    if(!dir.exists()) return;

    //获取所选文件类型过滤器
    QStringList filters;
    filters<<QString("*.dat");

    while(1)
    {
        if(mCancelFlg) break;
        //定义迭代器并设置过滤器
        QDirIterator dir_iterator(mDirName, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while(dir_iterator.hasNext())
        {
            if(mCancelFlg) break;
            dir_iterator.next();
            QFileInfo file_info = dir_iterator.fileInfo();
            QString file_path = file_info.absoluteFilePath();
            qDebug()<<"now read file:"<<file_path;
            QFile file(file_path);
            if(!file.open(QIODevice::ReadOnly))
            {
                qDebug()<<"open file failed...";
                continue;
            }

            while (!mCancelFlg)
            {
                QByteArray contents;
                //检查需要读取的文件长度
                if(mDefaultBytesLen == -1)
                {
                    //读取到/r/n结束
                    contents = file.readLine();
                    contents.remove(contents.size()-2, 2);
                } else
                {
                    //读取指定的文件长度
                    contents = file.read(mDefaultBytesLen);

                }
                if(file.atEnd()) break;
                emit signalSendContents(contents, contents.size());
                msleep(500);
            }
            //关闭文件
            file.close();

        }


    }


}
