#include "zchxradarcommon.h"


QByteArray zchxRadarUtils::surfaceTrackToByteArray(const zchxRadarSurfaceTrack &tracks)
{
    QByteArray totalData;
    totalData.resize(tracks.ByteSize());
    tracks.SerializeToArray(totalData.data(),totalData.size());
    return totalData;
}

QByteArray zchxRadarUtils::protoBufMsg2ByteArray(google::protobuf::Message *msg)
{
    QByteArray totalData;
    if(msg)
    {
        totalData.resize(msg->ByteSize());
        msg->SerializeToArray(totalData.data(),totalData.size());
    }
    return totalData;

}
