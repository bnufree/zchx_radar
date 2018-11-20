#include "zchxRadarCtrlWorker.h"
#include "common.h"

zchxRadarCtrlWorker::zchxRadarCtrlWorker(zchxMulticastDataScoket* socket,
                                         QObject *parent) :
    QObject(parent),
    mCtrlSocket(socket)
{
    if(mCtrlSocket)
    {
        connect(this, SIGNAL(sendCtrlData(QByteArray)),
                mCtrlSocket, SLOT(slotWriteData(QByteArray)));
    }
}

void zchxRadarCtrlWorker::open()
{
    LOG_FUNC_DBG_START;
    UINT8 cmd[] = {0x00, 0xc1, 0x01, 0x01, 0xc1, 0x01};
    sendCtrlData(UINT82ByteArray(cmd, 3));
    sendCtrlData(UINT82ByteArray(cmd + 3, 3));
    LOG_FUNC_DBG_END;
}

void zchxRadarCtrlWorker::close()
{
    LOG_FUNC_DBG_START;
    UINT8 cmd[] = {0x00, 0xc1, 0x01, 0x01, 0xc1, 0x00};
    sendCtrlData(UINT82ByteArray(cmd, 3));
    sendCtrlData(UINT82ByteArray(cmd + 3, 3));
    LOG_FUNC_DBG_END;
}

QByteArray zchxRadarCtrlWorker::UINT82ByteArray(UINT8 *arr, int count)
{
    QByteArray res;
    res.resize(count);
    memcpy(res.data(), arr, count);
    return res;
}

void zchxRadarCtrlWorker::setCtrValue(int infotype, int value)
{
    LOG_FUNC_DBG<<infotype<<value;;

    //检查当前值是否存在
    switch (infotype) {
    case INFOTYPE::POWER:
    {
        if(value == 0) {
            close();
        } else {
            open();
        }

        break;
    }
    case INFOTYPE::SCAN_SPEED:
    {
        UINT8 cmd[] = {0x0f, 0xc1, (UINT8)value };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::ANTENNA_HEIGHT:
    {
        int v = value * 1000;  // radar wants millimeters, not meters
        int v1 = v / 256;
        int v2 = v & 255;
        UINT8 cmd[10] = { 0x30, 0xc1, 0x01, 0, 0, 0, (UINT8)v2, (UINT8)v1, 0, 0 };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::BEARING_ALIGNMENT:
    {
        if (value < 0)  value += 360;
        value = value % 360;
        int v = value * 10;
        int v1 = v / 256;
        int v2 = v & 255;
        UINT8 cmd[4] = { 0x05, 0xc1, (UINT8)v2, (UINT8)v1 };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::RANG:
    {
        unsigned int decimeters = (unsigned int)value * 10;
        UINT8 cmd[] = { 0x03,0xc1,
                        (UINT8)((decimeters >> 0) & 0XFFL),
                        (UINT8)((decimeters >> 8) & 0XFFL),
                        (UINT8)((decimeters >> 16) & 0XFFL),
                      };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::GAIN:
    {
        //自动增益默认赋值
        UINT8 cmd[] = {0x06, 0xc1, 0, 0, 0, 0, 0x01, 0, 0, 0, 0xad}; //
        if (value >= 0) {
            // Manual Gain
            int v = (value + 1) * 255 / 100;
            if (v > 255) v = 255;
            cmd[6] = 0x00;
            cmd[10] = (UINT8)v;
        }
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::SEA_CLUTTER:
    {
        //自动默认赋值
        UINT8 cmd[] = {0x06, 0xc1, 0x02, 0, 0, 0, 0x01, 0, 0, 0, 0xd3}; //
        if (value >= 0) {
            // Manual Gain
            int v = (value + 1) * 255 / 100;
            if (v > 255) v = 255;
            cmd[6] = 0x00;
            cmd[10] = (UINT8)v;
        }
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }

    case INFOTYPE::RAIN_CLUTTER: // 8
    {
        //自动默认赋值
        UINT8 cmd[] = {0x06, 0xc1, 0x04, 0, 0, 0, 0x01, 0, 0, 0, 0xd3}; //
        if (value >= 0) {
            int v = (value + 1) * 255 / 100;
            if (v > 255) v = 255;
            cmd[6] = 0x00;
            cmd[10] = (UINT8)v;
        }
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::NOISE_REJECTION: // 9
    {
        UINT8 cmd[] = { 0x21, 0xc1, (UINT8)value };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::SIDE_LOBE_SUPPRESSION: // 10
    {
        //自动默认赋值
        UINT8 cmd[] = {0x06, 0xc1, 0x05, 0, 0, 0, 0x01, 0, 0, 0, 0xc0}; //
        if (value >= 0) {
            int v = (value + 1) * 255 / 100;
            if (v > 255) v = 255;
            cmd[6] = 0x00;
            cmd[10] = (UINT8)v;
        }
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::INTERFERENCE_REJECTION: // 11
    {
        UINT8 cmd[] = { 0x08, 0xc1, (UINT8)value };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::LOCAL_INTERFERENCE_REJECTION:  // 12
    {
        if (value < 0) value = 0;
        if (value > 3) value = 3;
        UINT8 cmd[] = { 0x0e, 0xc1, (UINT8)value };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::TARGET_EXPANSION: // 13
    {
        UINT8 cmd[] = { 0x09, 0xc1, (UINT8)value };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::TARGET_BOOST: // 14
    {
        UINT8 cmd[] = { 0x0a, 0xc1, (UINT8)value };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    case INFOTYPE::TARGET_SEPARATION: // 15
    {

        UINT8 cmd[] = { 0x22, 0xc1, (UINT8)value };
        sendCtrlData(UINT82ByteArray(cmd, sizeof(cmd)));
        break;
    }
    default:
        break;
    }
}
