#include "VideoDataRecvThread.h"
#include "BR24.hpp"
#include "common.h"
#include <QFile>
#include <QDir>
#include <QThread>

using namespace BR24::Constants;

#define     FAKE_FILE_DATA      1

VideoDataRecvThread::VideoDataRecvThread(const QString host,
                                         int port,
                                         QObject * parent)
:zchxMulticastDataScoket(host,
                         port,
                         "VIDEO",
                         sizeof(radar_frame_pkt),
                         ModeSyncRecv,
                         parent)
{
    connect(this, SIGNAL(start()), this, SLOT(run()));
    moveToThread(&mThread);
    mThread.start();
}

void VideoDataRecvThread::run()
{
    LOG_FUNC_DBG<<"!!!!";
    if(FAKE_FILE_DATA)
    {
        int index = 0;
        while (true) {
            QFile file(QDir::currentPath()+tr("/data/video_%1.dat").arg(index%4+1));
            qDebug()<<file.fileName()<<file.exists();
            int num = 0;
            if(file.open(QIODevice::ReadOnly))
            {
                while (!file.atEnd()) {
                    //QByteArray array = file.read(sizeof(radar_frame_pkt));
                    num++;
                    analysisRadar(file.read(sizeof(radar_frame_pkt)));
                    QThread::msleep(10);
                }
                file.close();
            }
            qDebug()<<"file:"<<file.fileName()<<" contains packets:"<<num++;
            index++;
        }
    } else
    {
        startRecv();
    }
}

void VideoDataRecvThread::processRecvData(const QByteArray &data)
{
    analysisRadar(data);
}
