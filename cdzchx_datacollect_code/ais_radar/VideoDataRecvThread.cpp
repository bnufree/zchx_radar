#include "VideoDataRecvThread.h"
#include "BR24.hpp"
#include "common.h"

using namespace BR24::Constants;

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
}

void VideoDataRecvThread::run()
{
    startRecv();
}

void VideoDataRecvThread::processRecvData(const QByteArray &data)
{
    analysisRadar(data);
}
