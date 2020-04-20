#include "zchxRadarVideoRecvThread.h"
#include "common.h"
#include <QFile>
#include <QDir>
#include <QThread>

#define     VIDEO_FRAME_SIZE        17160

#define     FAKE_FILE_DATA      1
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
const bool join_group = (FAKE_FILE_DATA == 1? false : true);

zchxVideoDataRecvThread::zchxVideoDataRecvThread(const QString host, int port, int size, const QString& fakePath)
    :zchxMulticastDataScoket(host, port, "VIDEO", fakePath.size() > 0 ? false:true, size, ModeAsyncRecv, 0),
      mFake(false),
      mFakePath(fakePath)
{
    mFake = (mFakePath.size() > 0 ? true : false);
     cout<<"mFake"<<mFake<<"fakePath"<<fakePath;
    if(mFake)
    {
        connect(this, SIGNAL(start()), this, SLOT(run()));
        moveToThread(&mThread);
        mThread.start();
    }
}

void zchxVideoDataRecvThread::startRecv()
{
    cout<<"mFake"<<mFake;
    if(mFake)
    {

        emit start();
    }
}

void zchxVideoDataRecvThread::run()
{
    LOG_FUNC_DBG<<"!!!!";
    cout<<"接收回波进程开始";
    if(mFake)
    {
        int index = 0;
        while (true) {
            QFile file(tr("%1/video_%2.dat").arg(mFakePath).arg((++index)%5));
            qDebug()<<file.fileName()<<file.exists();
            int num = 0;
            if(file.open(QIODevice::ReadOnly))
            {
                while (!file.atEnd()) {
                    //QByteArray array = file.read(sizeof(radar_frame_pkt));
                    num++;
                    signalSendRecvData(file.read(VIDEO_FRAME_SIZE));
                    QThread::msleep(20);
                    //qDebug()<<"file:"<<file.fileName()<<" contains packets:"<<num;
                }
                file.close();
            }
            qDebug()<<"file:"<<file.fileName()<<" contains packets:"<<num++;
            index = index % 5;
        }
    } else
    {
        startRecv();
    }
}

void zchxVideoDataRecvThread::processRecvData(const QByteArray &data)
{
    signalSendRecvData(data);
}
