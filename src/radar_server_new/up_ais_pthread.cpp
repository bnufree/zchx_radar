#include "up_ais_pthread.h"
#include <QThread>
#include <QFile>
#include <QString>
#include <QDialog>
#include <QFileDialog>
#include "profiles.h"
#include "Log.h"

up_ais_pthread::up_ais_pthread(QString filename, int interval, QObject *parent)
    : mFileName(filename)
    , mInterval(interval)
    , mIsOver(false)
    , QThread(parent)
{

}

void up_ais_pthread::run()
{
    QFile ais_file(mFileName);
    if(!ais_file.open(QIODevice::ReadOnly))
    {
        ZCHX_LOG_OUT("打开失败")<<mFileName;
        return;
    }
    ZCHX_LOG_OUT("打开成功")<<mFileName;

    //操作文件
    while(!mIsOver)
    {
        QString ais_str = ais_file.readLine();
        QByteArray ais_array;
        ais_array = ais_str.toLatin1();
        int a = ais_str.size();
        if(a != 0)
        {
            emit send_ais_signal(ais_array);
            //break;
        }
        if(ais_file.atEnd())
        {
            ZCHX_LOG_OUT("解析完毕");
            ais_file.seek(0);
        }
        msleep(mInterval * 1000);
    }

    //关闭文件
    ais_file.close();
}
