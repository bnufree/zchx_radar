#include "up_ais_pthread.h"
#include <QThread>
#include <QFile>
#include <QString>
#include <QDialog>
#include <QFileDialog>
#include <QDebug>
#include "profiles.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
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
        cout<<"打开失败"<<mFileName;
        return;
    }
    cout<<"打开成功"<<mFileName;

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
            cout<<"解析完毕";
            ais_file.seek(0);
        }
        msleep(mInterval * 1000);
    }

    //关闭文件
    ais_file.close();
}
