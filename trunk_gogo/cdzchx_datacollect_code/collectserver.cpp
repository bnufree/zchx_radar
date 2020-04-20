#include "collectserver.h"
#include "zmq.h"
#include "zmq_utils.h"
#include "zmq.hpp"
#include "recivecomdata.h"
#include "profiles.h"
#include "Log.h"
#include <QDateTime>
#include <windows.h>
#include <QDebug>
#include "ZmqMonitorThread.h"
#include <QDateTime>
#include <QFile>
#include <QString>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

enum JSON_PARSE_CMD{
    JSON_PARSE_CMD_PROJECT_ADD_MODIFY = 0,
    JSON_PARSE_CMD_PROJECT_DELETE,
    JSON_PARSE_CMD_ROUTE_DELETE,
    JSON_PARSE_CMD_ROUTE_ADD,
    JSON_PARSE_CMD_ROUTE_MODIFY,
    JSON_PARSE_CMD_ROUTE_UPDATE_PATHDATA,
    JSON_PARSE_CMD_CABLE_BASE_ADD_MODIFY,
    JSON_PARSE_CMD_CABLE_BASE_DELETE,
    JSON_PARSE_CMD_CABLE_INRETFACE_ADD_MODIFY,
    JSON_PARSE_CMD_CABLE_INRETFACE_DELETE,
    JSON_PARSE_CMD_CABLE_ASSEMBLY_ADD_MODIFY,
    JSON_PARSE_CMD_CABLE_ASSEMBLY_DELETE,
    JSON_PARSE_CMD_PLAN_ADD_MODIFY,
    JSON_PARSE_CMD_PLAN_DELETE,
    JSON_PARSE_CMD_PLAN_DETAIL_ADD_MODIFY,
    JSON_PARSE_CMD_PLAN_DETAIL_DELETE,
    JSON_PARSE_CMD_FORECAST_PLAN_ADD_MODIFY,
    JSON_PARSE_CMD_FORECAST_PLAN_DELETE,
    JSON_PARSE_CMD_FORECAST_PLAN_DELETE_INDEX,
    JSON_PARSE_CMD_PLAN_PAR_DETAIL_MODIFY,
    JSON_PARSE_CMD_IMPORT_OUT_XLS_FILE,
    JSON_PARSE_CMD_CONSTRUCTION_START,
    JSON_PARSE_CMD_CONSTRUCTION_END,
};

const double M_2PREP30 =  0.000000000931322574615478515625;
const double M_2PREP15 =  0.000030517578125;
const double M_2PREP13 =  0.0001220703125;
#define GLOB_PI  (3.14159265358979323846)

#define         HOST_KEY                    "host"
#define         CLIENT_UPDATE_KEY           "update"
#define         CLIENT_UPDATE_CMD           "command"
#define         CLIENT_UPDATE_VAL           "info"

double ang2Cir(double angle)
{
    double pi=GLOB_PI;
    return (pi/180)*angle;
}

static unsigned char auchCRCLo[] = {
0x00,0xC0,0xCl,0x0l,0xC3,0x03,0x02,0xC2,0xC6,0x06,
0x07,0xC7,0x05,0xC5,0xC4,0x04,0xCC,0x0C,0x0D,0xCD,
0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,
0x08,0xC8,0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,
0x1E,0xDE,0xDF,0x1F,0xDD,0x1D,0x1C,0xDC,0x14,0xD4,
0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,
0x11,0xDl,0xD0,0x10,0xF0,0x30,0x31,0xFl,0x33,0xF3,
0xF2,0x32,0x36,0xF6,0xF7,0x37,0xF5,0x35,0x34,0xF4,
0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,
0x3B,0xFB,0x39,0xF9,0xF8,0x38,0x28,0xE8,0xE9,0x29,
0xEB,0x2B,0x2A,0xEA,0xEE,0x2E,0x2F,0xEF,0x2D,0xED,
0xEC,0x2C,0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,
0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,0xA0,0x60,
0x61,0xA1,0x63,0xA3,0xA2,0x62,0x66,0xA6,0xA7,0x67,
0xA5,0x65,0x64,0xA4,0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,
0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,
0x78,0xB8,0xB9,0x79,0xBB,0x7B,0x7A,0xBA,0xBE,0x7E,
0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,0xB4,0x74,0x75,0xB5,
0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,
0x70,0xB0,0x50,0x90,0x9l,0x51,0x93,0x53,0x52,0x92,
0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9C,0x5C,
0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,
0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4B,0x8B,
0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,
0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,
0x43,0x83,0x41,0x81,0x80,0x40
};



static unsigned char auchCRCHi[] = {
0x00,0xCl,0x81,0x40,0x0l,0xC0,0x80,0x41,0x0l,0xC0,
0x80,0x41,0x00,0xCl,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xCl,0x81,0x40,0x00,0xCl,0x8l,0x40,0x0l,0xC0,
0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
0x00,0xCl,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xCl,
0x81,0x40,0x0l,0xC0,0x80,0x41,0x0l,0xC0,0x80,0x41,
0x00,0xCl,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xCl,
0x8l,0x40,0x00,0xCl,0x8l,0x40,0x0l,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xCl,0x81,0x40,0x00,0xCl,0x8l,0x40,
0x01,0xC0,0x80,0x41,0x0l,0xC0,0x80,0x41,0x00,0xCl,
0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
0x01,0xC0,0x80,0x41,0x00,0xCl,0x81,0x40,0x01,0xC0,
0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x00,0xCl,0x81,0x40,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
0x01,0xC0,0x80,0x41,0x0l,0xC0,0x80,0x41,0x00,0xC1,
0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40
};

unsigned short CRC16(const QByteArray& data)
{
    unsigned char high = 0xFF;
    unsigned char low = 0xFF;
    unsigned index;
    int size = data.size();
    int i=0;
    while (i<size) {
        index = high ^ data[i];
        high = low ^ auchCRCHi[index];
        low = auchCRCLo[index];
        i++;
    }

    return ((high << 8) | low);
}

QByteArray      short2Byte(unsigned short val)
{
    QByteArray res;
    res.resize(2);
    res[0] = (val >> 8 ) & 0xFF;
    res[1] = val & 0xFF;

    return res;
}

short    bytes2Short(const QByteArray& bytes)
{
    short val = 0;
    if(bytes.size() != 2) return 0;
    //qDebug()<<"num bytes:"<<bytes.toHex().toUpper();
    val = bytes[1] & 0xFF;
    val |= ((bytes[0] & 0xFF) << 8);

    return val;
 }

bool  getValue(const QByteArray& val, short &res)
{
    if(val.size() <5) return false;
    char addr = val[0];
    char func = val[1];
    char num_bytes = val[2];

    if(val.size() != 5+num_bytes) return false;
    QByteArray num = val.mid(3, num_bytes);
    //CRC检查
    QByteArray data = val.mid(0, 3+num_bytes);
    if(short2Byte(CRC16(data)) == val.right(2))
    {
        res = bytes2Short(num);
    }
}

bool  getValue2(const QByteArray& val, double &res)
{
    if(val.size() <10) return false;
    QByteArray ushortArray  = val.mid(5, 2);
    QByteArray decimalArray;
    decimalArray.resize(2);
    decimalArray[0] = 0x00;
    decimalArray[1] = val.mid(7,1).at(0);

    short ushortVal = bytes2Short(ushortArray);
    short decimalVal = bytes2Short(decimalArray);
    double coeff = 0.1;
    if(decimalVal >= 2)
    {
       coeff = 0.01;
    }

    res = ushortVal * coeff;
    return true;
}

CollectServer::CollectServer(void* ctx, QObject *parent) : QObject(parent)
{
    mGpsBuf.clear();
    mCableBuf.clear();
    mDpBuf.clear();
    mCtx = ctx;
    mSocket = NULL;
    mDisplayTimer = NULL;
    mFakeIndex = 0;
    mFakeConstructionMode = 0;
    mCalCourseFlag = Utils::Profiles::instance()->value("OtherSet", "course_calculate", false).toBool();
    mLastPosFlag = false;
    mFirstRecvMeterCounterFlag = true;
    mWorkNow = false;
    mLastMeterCounterNum = 0;
    Utils::Profiles::instance()->setDefault("SaveData", "Cable_Length", 0.0);
    Utils::Profiles::instance()->setDefault("SaveData", "MeterCounter_Max", 10);
    mSavedCableLength = Utils::Profiles::instance()->value("SaveData", "Cable_Length", 0.0).toDouble();
    mMeterConterMax = Utils::Profiles::instance()->value("SaveData", "MeterCounter_Max", 10).toInt();
    init();
    mSurfaceChannelNum = 8;
    m40171ChannelNum = 8;
    m40172ChannelNum = 8;
    mLastSendTime = 0;
    mLastCalCableLength = 0.0;
    mLastCalShipSpeedTime = 0;
}

CollectServer::~CollectServer()
{
    stop();
    mDevInfo.release_lay_info();
    mDevInfo.release_plow_info();
    mDevInfo.release_ship_info();
    mDevInfo.release_surface_4017();
    mDevInfo.release_under_4017();
    mDevInfo.release_construct_info();
}

void CollectServer::start()
{
    mRunning = true;
    connect(this, SIGNAL(signalSetWorkStatus(bool)), this, SLOT(slotSetWorkStatus(bool)));
    connect(this, SIGNAL(signalStartComConnection()), this, SLOT(slotStartComConnection()));
    mHeartTimer = new QTimer;
    mHeartTimer->setInterval(3000);
    connect(mHeartTimer, SIGNAL(timeout()), this, SLOT(slotHeartMsg()));
    this->moveToThread(&mWorkThread);
    mWorkThread.start();
    mHeartTimer->start();
    if(mFakeflag)
    {
        mFakeData = FakeData();
        mFakeConstructionMode = 0;
        mFakeInterval = Utils::Profiles::instance()->value("Host","FakeInterval", 2).toInt();
        emit signalSendLogMsg("fake sever has been start, waiting for dp data.");
        mDisplayTimer = new QTimer();
        mDisplayTimer->setInterval(2*1000);

        connect(mDisplayTimer, SIGNAL(timeout()), this, SLOT(slotTimeOut()));
        mDisplayTimer->start();
    }

}

void CollectServer::startFakeServer(int index)
{
    if(mFakeflag)
    {
        if(!readFile())
        {
            emit signalSendLogMsg("initialize ship fake data failed.");
            return;
        }
        mFakeIndex = index;
        mFakeConstructionMode = 1;
        emit signalSendLogMsg(" fake construction start.send data now");
    }
}

void CollectServer::stopFakeServer()
{
    mFakeConstructionMode = 0;
    //mFakeData = FakeData();
}

void CollectServer::pauseFakeServer()
{
    mFakeConstructionMode = 2;
}

bool CollectServer::readFile()
{
    mFakeDataList.clear();
    QString fileName = Utils::Profiles::instance()->value("Host","FakeFileName").toString();
    //qDebug()<<"work ship file is:"<<fileName;
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)) return false;

    QList<FakeData> srcFakeDatalist;
    QTextStream out(&file);//文本流
    QStringList tempOption = out.readAll().split("\n");//每行以\n区分
    //qDebug()<<"temp:"<<tempOption.size();
    for(int i=2;i<tempOption.count();i++)
    {
        QStringList line = tempOption.at(i).split(",");//一行中的单元格以，区分
        //qDebug()<<""<<line;//
        int x = line.count();
        //qDebug()<<""<<x;
        if(x == 17)
        {
            //qDebug()<<"lenth:17";
            //添加新的数据
            FakeData data;
            data.lat = convertLonlat2DoubleWithSNWE(line[1]);
            data.lon = convertLonlat2DoubleWithSNWE(line[2]);
            data.alter_course = line[4].toDouble();
            data.pointTotalDis = line[6].toDouble();
            data.slack = line[7].toDouble();
            data.pointCableTotaLength = line[9].toDouble();
            data.comment = line[10];
            data.shipspd = line[11].toDouble();
            data.cableSpd = line[12].toDouble();
            //这里的实时时间不考虑
            srcFakeDatalist.append(data);
        } else if(x == 16)
        {
            //qDebug()<<"lenth:16";

            //这个是更新段落数据
            FakeData &data = srcFakeDatalist.last();
            data.segmentCourse = line[3].toDouble();
            data.segmentDis = line[5].toDouble();
            data.segmentCableLength = line[8].toDouble();
            QString durStr = line[15];
            durStr.remove("\r");
            if(durStr.trimmed().length() == 8)
            {
                data.segmentDuration = QTime::fromString(durStr, "hh:mm:ss");
            } else
            {
                data.segmentDuration = QTime::fromString(durStr, "h:mm:ss");
            }
        }

    }


    //省略具体对数据的操作

    file.close();//操作完成后记得关闭文件

    //在施工数据的中间段添加一次收揽的操作
    //假定实时数据20米更新一次
    qDebug()<<"src size:"<<srcFakeDatalist.size();
    for(int i=0; i<srcFakeDatalist.size() - 1; i++)
    {
        FakeData data = srcFakeDatalist[i];
        //qDebug()<<"segDis:"<<data.segmentDis<<endl;
        int totalgaps = (int)(floor(data.segmentDis / (mFakeInterval * 0.001)));
        if(totalgaps == 1 || totalgaps == 0)
        {
            mFakeDataList.append(data);
            continue;
        }

        //需要额外插入数据的情况
        FakeData nextData = srcFakeDatalist[i+1];
        for(int k=0; k<totalgaps; k++)
        {
            QPointF divPnt = getDividePoint(QPointF(data.lon, data.lat), QPointF(nextData.lon, nextData.lat), totalgaps, k);
            QPointF nextPnt = QPointF(nextData.lon, nextData.lat);
            if(k < totalgaps -1) {
                nextPnt = getDividePoint(QPointF(data.lon, data.lat), QPointF(nextData.lon, nextData.lat), totalgaps, k+1);
            }
            FakeData divData = data;
            divData.lon = divPnt.x();
            divData.lat = divPnt.y();
            divData.segmentDis = getDistanceDeg(divPnt.y(), divPnt.x(), nextPnt.y(), nextPnt.x()) / 1000;
            divData.segmentCableLength = divData.segmentDis * (1+divData.slack/100);
            //检查前一个数据是否存在
            if(mFakeDataList.length() > 0)
            {
                FakeData preData = mFakeDataList.last();
                divData.pointTotalDis = preData.pointTotalDis + preData.segmentDis;
                divData.pointCableTotaLength = preData.pointCableTotaLength + preData.segmentCableLength;
            } /*else
            {
                divData.pointTotalDis = 0;
                divData.pointCableTotaLength = 0;
            }*/
            mFakeDataList.append(divData);
        }

    }
    mFakeDataList.append(srcFakeDatalist[srcFakeDatalist.size()-1]);
    qDebug()<<"fake total data size:"<<mFakeDataList.size();
//    foreach (FakeData data, mFakeDataList) {
//        qDebug()<<" "<<data.lon<<" "<<data.lat<<" "<<data.segmentDuration<<" "<<data.pointCableTotaLength;
//    }
    return true;
}

//src:E114 05.107867
double CollectServer::convertLonlat2DoubleWithSNWE(const QString &src)
{
    QString wkstr = src;
    int positive = 1;
    //首先判断是否NWSE标记经纬度
    if(src.contains(QRegularExpression("[NWSE]")))
    {
        if(!src.contains(QRegularExpression("[NE]")))
        {
            positive = -1;
        }

        wkstr.remove(QRegularExpression("[NWSE]"));
    }

    QStringList valist = wkstr.split(QRegularExpression("[ \.]"));
    if(valist.length() < 3) return 0.00;
    int deg = valist[0].toInt();
    double decimal = (valist[1].toInt() + valist[2].toInt() / 1000000.00) / 60.0;
    return deg + decimal;
}

QPointF CollectServer::getDividePoint(const QPointF &start, const QPointF &end, int totalGaps, int cur)
{
    //x = (x1+lamd*x2)/(1+lamd) y = (y1+lamd * y2) /(1+lamd)
    double x, y;
    double lamd = cur / (double) (totalGaps - cur);
    x = (start.x() + lamd * end.x()) / (1+lamd);
    y = (start.y() + lamd * end.y()) / (1+lamd);

    return QPointF(x, y);
}

void CollectServer::slotHeartMsg()
{
    slotSendComData(QByteArray("HEART_MSG"), QByteArray("12345678"));
}

void CollectServer::stop()
{
    LOG(LOG_RTM, "server end now------");
    if(mHeartTimer)
    {
        mHeartTimer->stop();
        mHeartTimer->deleteLater();
    }
    if(mDisplayTimer)
    {
        mDisplayTimer->stop();
        mDisplayTimer->deleteLater();
    }
    foreach (ReciveComData * data, mStartedComList.values()) {
        if(data)
        {
            data->stop();
            delete data;
        }
    }
    mRunning = false;
    mWorkThread.wait(5);
    if(mSocket)
    {
        zmq_close(mSocket);
        mSocket = 0;
    }




    LOG(LOG_RTM, "server end now success.");
}

void* CollectServer::getContext()
{
    return mCtx;
}

void CollectServer::stopComWork(ReciveComData *data)
{
    if(!data) return;
    data->stop();
    mStartedComList.remove(data->getTopic());
    delete data;
}

void CollectServer::setComDevParams(const QMap<QString, COMDEVPARAM>& map)
{
    mDevComParamsMap = map;
    if(mWorkNow)
    {
        slotStartComConnection();
    }
}

void CollectServer::slotSetWorkStatus(bool sts)
{
    mWorkNow = sts;
    if(mWorkNow)
    {
        slotStartComConnection();
    } else
    {
        slotCloseComConnection();
    }
}

void CollectServer::slotCloseComConnection()
{
    foreach (QString key, mStartedComList.keys()) {
        //串口已经启动
        ReciveComData *recv = mStartedComList.value(key, NULL);
        if(recv)
        {
            //结束当前串口的执行
            stopComWork(recv);

        }
    }
}

void CollectServer::slotStartComConnection()
{
    foreach (QString key, mDevComParamsMap.keys()) {
        //先更新设备窗口的情况
        COMDEVPARAM dev = mDevComParamsMap.value(key);
        if(key == MSG_SURFACE_HMR)
        {
            mDevInfo.set_surface_hmr3000_checked_flg(dev.mStatus);
        } else if(key == MSG_GPS)
        {
            mDevInfo.set_gsp_checked_flg(dev.mStatus);
        } else if(key == MSG_USBL)
        {
            mDevInfo.set_usbl_checked_flg(dev.mStatus);
        } else if(key == MSG_UNDER_HMR)
        {
            mDevInfo.set_under_hmr3000_checked_flg(dev.mStatus);
        } else if(key == MSG_UNDER_4017)
        {
            mDevInfo.set_under_4017_checked_flg(dev.mStatus);
        } else if(key == MSG_SURFACE_4017)
        {
            mDevInfo.set_surface_4017_checked_flg(dev.mStatus);
        } else if(key == MSG_METER_COUNTER)
        {
            mDevInfo.set_metercounter_checked_flg(dev.mStatus);
        } else if(key == MSG_NAVI_DEV)
        {
            mDevInfo.set_navi_device_checked_flg(dev.mStatus);
        } else if(key == MSG_CABLE_DEV)
        {
            mDevInfo.set_cable_integrated_checked_flg(dev.mStatus);
        } else if(key == MSG_DP_DEV)
        {
            mDevInfo.set_dp_checked_flg(dev.mStatus);
        }

        //如果对应的串口已经启动
        if(mStartedComList.contains(dev.mTopic))
        {
            //串口已经启动
            ReciveComData *recv = mStartedComList.value(key, NULL);
            if(recv)
            {
                if(!dev.mStatus)
                {
                    //结束当前串口的执行
                    stopComWork(recv);
                    continue;
                } else
                {
//                    //检查对应的参数是否已经改变
//                    if(recv->getComName() == dev.mName && recv->getBaudRate() == dev.mBaudRate
//                            && recv->getParity() == dev.mParity && recv->getStopBit() == dev.mStopBit
//                            && recv->getDataBit() == dev.mDataBit)
//                    {
//                        //参数没有变，什么都不做
//                        continue;
//                    } else
//                    {
                        //参数改变了。停止后。重新开始
                        stopComWork(recv);
//                    }
                }
            }
        }

        //如果串口没有选择停止，根据新的参数启动
        if(!dev.mStatus) continue;
        //开启串口，读取
        ReciveComData *recv = new ReciveComData(dev.mTopic, dev.mName, dev.mBaudRate, dev.mParity, dev.mDataBit, dev.mStopBit);
        connect(recv,SIGNAL(signalReciveComData(QString,QString,qint64,QByteArray)), this,SLOT(slotRecvComData(QString,QString,qint64,QByteArray)));
        connect(recv, SIGNAL(signalSerialPortErrorStr(QString)), this, SIGNAL(signalSendLogMsg(QString)));
        //构造发送的命令
        QByteArray cmd;
        if(dev.mTopic == MSG_TENSION_DEV)
        {
            cmd = makeTensionCmd();
        }
        if(recv->open(dev.mOpenMode, cmd))
        {
            emit signalSendLogMsg(tr("com opend success(%1:%2). openmode:%3, parity:%4, databits:%5, stopbits:%6").
                                  arg(dev.mTopic).arg(dev.mName).arg(recv->openMode()).arg(dev.mParity).arg(dev.mDataBit).arg(dev.mStopBit));
        }
        mStartedComList[dev.mTopic] = recv;

    }
}

void CollectServer::init()
{
    //Zmq
    if(!mCtx) return;
    mSocket = zmq_socket (mCtx, ZMQ_PUB);
    //监听zmq
    QString monitorUrl = "inproc://monitor.pubtoclient";
    zmq_socket_monitor (mSocket, monitorUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    ZmqMonitorThread *thread = new ZmqMonitorThread(mCtx, monitorUrl, 0);
    connect(thread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    thread->start();


    portNum = Utils::Profiles::instance()->value("Host", "PublishPort").toInt();
    QString url;
    url.sprintf("tcp://*:%d", portNum);
    int res = zmq_bind(mSocket, url.toUtf8().data());
    LOG(LOG_RTM, "start zmq publist at port = %d. bind returned:%d", portNum, res);

    //检查设定是否使用虚拟数据
    mFakeflag = false;
    if(Utils::Profiles::instance()->value("Host", "FakeServer", false).toBool())
    {
        mFakeflag = true;

    }

    //初始化设备数据
    //船体数据
    mShipInfo.set_ship_update_time(0);
    mShipInfo.set_ship_lat(0.0);
    mShipInfo.set_ship_lon(0.0);
    mShipInfo.set_ship_speed(0.0);
    mShipInfo.set_ship_speed_ground(0.0);
    mShipInfo.set_ship_head(0.0);
    mShipInfo.set_ship_pitch(0.0);
    mShipInfo.set_ship_roll(0.0);
    mShipInfo.set_ship_course(0.0);
    mShipInfo.set_heave(0.0);
    mShipInfo.set_wind_dir(0.0);
    mShipInfo.set_wind_speed(0.0);
    mShipInfo.set_surge_demand(0.0);
    mShipInfo.set_surge_feedback(0.0);
    mShipInfo.set_sway_demand(0.0);
    mShipInfo.set_sway_feedback(0.0);
    mShipInfo.set_ship_mode(0);
    //犁设备数据
    mPlowInfo.set_plow_update_time(0);
    mPlowInfo.set_plow_lat(0.0);
    mPlowInfo.set_plow_lon(0.0);
    mPlowInfo.set_plow_pos_mode(0);
    mPlowInfo.set_plow_flag(0);
    mPlowInfo.set_plow_head(0.0);
    mPlowInfo.set_plow_roll(0.0);
    mPlowInfo.set_plow_pitch(0.0);
    mPlowInfo.set_plow_water_depth(0.0);
    mPlowInfo.set_plow_buried_depth(0.0);
    mPlowInfo.set_plow_left_pump(0.0);
    mPlowInfo.set_plow_right_pump(0.0);
    mPlowInfo.set_plow_speed(0.0);
    mPlowInfo.set_plow_boots_angle(0.0);
    mPlowInfo.set_plow_boots_length(0.0);
    mPlowInfo.set_plow_tow_length(0.0);
    mPlowInfo.set_plow_tow_tension(0.0);
    mPlowInfo.set_plow_pos_update_time(0);
    mPlowInfo.set_plow_kp_dis(0.0);
    //布缆数据
    mLayInfo.set_cable_update_time(0);
    mLayInfo.set_cable_payout_speed(0);
    mLayInfo.set_meter_counter_time(0);
    mLayInfo.set_meter_counter_coeff(0.0);
    mLayInfo.set_meter_counter_initlen(0.0);
    mLayInfo.set_meter_counter_length(0.0);
    mLayInfo.set_cable_length(0.0);
    mLayInfo.set_glj_cb_length(0.0);
    mLayInfo.set_glj_cb_speed(0.0);
    mLayInfo.set_glj_cb_tension(0.0);
    mLayInfo.set_ltj_cb_length(0.0);
    mLayInfo.set_ltj_cb_speed(0.0);
    mLayInfo.set_ltj_cb_tension(0.0);
    mLayInfo.set_tl_length(0.0);
    mLayInfo.set_tl_tension(0.0);
    mLayInfo.set_qd_length(0.0);
    mLayInfo.set_qd_tension(0.0);
    mLayInfo.set_light_status(0);
    mLayInfo.set_meter_source(0);
    //水面4017
    mSurface4017.set_surface_4017_time(0);
    mSurface4017.set_surface_4017_lpump(0.0);
    mSurface4017.set_surface_4017_pull1(0.0);
    mSurface4017.set_surface_4017_pull2(0.0);
    mSurface4017.set_surface_4017_pull3(0.0);
    mSurface4017.set_surface_4017_rpump(0.0);
    //水下4017
    mUnder4017.set_under_4017_time(0);
    mUnder4017.set_under_4017_boots_angle(0.0);
    mUnder4017.set_under_4017_boots_length(0.0);
    mUnder4017.set_under_4017_buried_depth(0.0);
    mUnder4017.set_under_4017_lpump(0.0);
    mUnder4017.set_under_4017_pull1(0.0);
    mUnder4017.set_under_4017_pull2(0.0);
    mUnder4017.set_under_4017_pull3(0.0);
    mUnder4017.set_under_4017_rpump(0.0);
    mUnder4017.set_under_4017_touch_down_p1(0.0);
    mUnder4017.set_under_4017_touch_down_p2(0.0);
    mUnder4017.set_under_4017_touch_down_p3(0.0);
    mUnder4017.set_under_4017_touch_down_p4(0.0);
    mUnder4017.set_under_4017_water_depth(0.0);

    //施工信息
    mConstructInfo.set_iscontructed(false);
    mConstructInfo.set_host_name("");
    mConstructInfo.set_plan_id(-1);
    mConstructInfo.set_project_id(-1);
    mConstructInfo.set_route_id(-1);

    //
    mDevInfo.set_allocated_lay_info(&mLayInfo);
    mDevInfo.set_allocated_plow_info(&mPlowInfo);
    mDevInfo.set_allocated_ship_info(&mShipInfo);
    mDevInfo.set_allocated_surface_4017(&mSurface4017);
    mDevInfo.set_allocated_under_4017(&mUnder4017);
    mDevInfo.set_allocated_construct_info(&mConstructInfo);
    mDevInfo.set_surface_4017_checked_flg(false);
    mDevInfo.set_under_4017_checked_flg(false);
    mDevInfo.set_under_hmr3000_checked_flg(false);
    mDevInfo.set_surface_hmr3000_checked_flg(false);
    mDevInfo.set_gsp_checked_flg(false);
    mDevInfo.set_dp_checked_flg(false);
    mDevInfo.set_navi_device_checked_flg(false);
    mDevInfo.set_cable_integrated_checked_flg(false);
    mDevInfo.set_metercounter_checked_flg(false);
    mDevInfo.set_usbl_checked_flg(false);

    //
}

void CollectServer::slotRecvComData(const QString& comName, const QString& topic, qint64 time, const QByteArray& recv)
{
//    qDebug()<<"recv:"<<recv;
    if(MSG_DP_UPLOAD_DEV == topic)
    {
        emit signalSendRecvedContent(time, comName, QString::fromUtf8(recv.constData()));
        return;
    }    
    bool change2HexFlag = false;
    bool ship_pos_update = false;
    //解析数据，发送给接收到的客户端
    if(MSG_CABLE_DEV == topic)
    {
        slotCableLayeReciveComData(time, recv);
        change2HexFlag = true;
    } else if(MSG_NAVI_DEV == topic)
    {
        slotNaviReciveComData(time, recv);
        change2HexFlag = true;
        ship_pos_update = true;
    } else if(MSG_DP_DEV == topic)
    {
        slotDpReciveComData(time, recv);
        ship_pos_update = true;
    } else if(MSG_METER_COUNTER == topic)
    {
        COMDEVPARAM param = mDevComParamsMap[topic];
        slotMeterCounterReciveComData(time, recv, param.mMessageNum1);
    } else if(MSG_SURFACE_4017 == topic)
    {
        COMDEVPARAM param = mDevComParamsMap[topic];
        slotSurface4017ReciveComData(time, recv, param.mMessageNum1);
    } else if(MSG_UNDER_4017 == topic)
    {
        COMDEVPARAM param = mDevComParamsMap[topic];
        slotUnderWater4017ReciveComData(time, recv, param.mMessageNum1, param.mMessageNum2);
    } else if(MSG_UNDER_HMR == topic)
    {
        slotUnderHMRReciveComData(time, recv);
    } else if(MSG_USBL == topic)
    {
        slotUSBLReciveComData(time, recv);
    } else if(MSG_GPS == topic)
    {
        slotGPSReciveComData(time, recv);
        ship_pos_update = true;
    } else if(MSG_SURFACE_HMR == topic)
    {
        slotSurfaceHMRReciveComData(time, recv);
    } else if(MSG_TENSION_DEV)
    {
        change2HexFlag = true;
        //开始解析
        double res = 0;
        if(getValue2(recv, res))
        {
            //qDebug()<<"res:"<<res << mTensionCoeff;
            res = res * mTensionCoeff;
            mLayInfo.set_ltj_cb_tension(res);
            //qDebug()<<"res:"<<res;
        }
    }
    //转发到列表显示接收到的数据日志
    QString saveContent;
    if(change2HexFlag)
    {
        saveContent = QString::fromStdString(recv.toHex().toUpper().constData());
        emit signalSendRecvedContent(time, comName, QString::fromStdString(recv.toHex().toUpper().constData()) + QString(" TENSION:%1").arg(mLayInfo.ltj_cb_tension()));
    }
    else
    {
        saveContent = QString::fromStdString(recv.constData());
        emit signalSendRecvedContent(time, comName, QString::fromStdString(recv.constData()));
    }

    LOG(topic, (char*)saveContent.toStdString().data());

    //检查是否需要手动计算犁设备的经纬度位置
    if((!mDevInfo.usbl_checked_flg()) && ship_pos_update )
    {
        //没有启动超短基线设备，需要通过船舶的经纬度进行手动计算
        if(mShipInfo.ship_mode() != 0)
        {
            //已经取得了船舶的经纬度
            double tow_wrie_out = 0.0;
            if(mPlowInfo.plow_tow_length() != 0)
            {
                tow_wrie_out = mPlowInfo.plow_tow_length();
            } else
            {
                tow_wrie_out = mTowRange;
            }
            double water_depth = mPlowInfo.plow_water_depth();

            //采用勾股定理计算犁设备的KP值
            if(tow_wrie_out > water_depth)
            {
                double sub_kp = sqrt(tow_wrie_out * tow_wrie_out - water_depth * water_depth);
                //计算犁设备的经纬度
                double plow_lat = 0.0, plow_lon = 0.0;
                double last_lon = mPlowInfo.plow_lon(), last_lat = mPlowInfo.plow_lat();
                distbear_to_latlon(mShipInfo.ship_lat(), mShipInfo.ship_lon(), sub_kp, 180-mShipInfo.ship_course(), plow_lat, plow_lon);
                mPlowInfo.set_plow_lat(plow_lat);
                mPlowInfo.set_plow_lon(plow_lon);
                mPlowInfo.set_plow_pos_mode(2);
                mPlowInfo.set_plow_flag(false);

                //计算犁设备的速度
                if(mPlowInfo.plow_pos_update_time() == 0)
                {
                    mPlowInfo.set_plow_speed(0.0);

                } else
                {
                    //计算两点之间的距离
                    double dis = getDistanceDeg(last_lat, last_lon, mPlowInfo.plow_lat(), mPlowInfo.plow_lon());
                    qint64 subtime = time - mPlowInfo.plow_pos_update_time();
                    if(subtime != 0)
                    {
                        double speed = (dis * 1000) / subtime ;
                        mPlowInfo.set_plow_speed(speed);
                    }
                    mPlowInfo.set_plow_kp_dis(dis);

                }
                mPlowInfo.set_plow_pos_update_time(time);
                mPlowInfo.set_plow_update_time(time);
            }

        }
    }

    //发送给客户端
    if(mLastSendTime == 0 || (mLastSendTime != 0 && time - mLastSendTime > 1000))
    {
        if(        mShipInfo.ship_update_time() == 0
                   && mLayInfo.cable_update_time() == 0
                   && mPlowInfo.plow_update_time() == 0
                   && mSurface4017.surface_4017_time() == 0
                   && mUnder4017.under_4017_time() == 0)
        {
            return;
        }
        QByteArray dst;
        dst.resize(mDevInfo.ByteSize());
        mDevInfo.SerializeToArray(dst.data(), dst.size());
        slotSendComData("DEVICE_INFO", dst);
        mLastSendTime = time;

    }

}


void CollectServer::slotDpReciveComData(qint64 time, QByteArray data)
{    
    mShipInfo.set_ship_mode(2);
    mShipInfo.set_ship_update_time(time);

    //开始解析数据
    QString src = QString::fromStdString(data.constData());
    //取得命令的名称
    QRegExp exp("\\$([A-Z]{1,})");
    if(exp.indexIn(src) < 0) return;
    QString cmd = exp.cap(1);
    if(cmd.length() == 0) return;
    if(cmd == "PMASNS")
    {
        parsePMASNSData(src);
    } else if(cmd == "PMAGGA")
    {
        parsePMAGGAData(src);
    } else if(cmd == "PMAVTG")
    {
        parsePMAVTGData(src);
    } else if(cmd == "PMATHA")
    {
        parsePMATHAData(src);
    } else if(cmd == "PMATHF")
    {
        parsePMATHFData(src);
    } else if(cmd == "PMAINF")
    {
        parsePMAINFData(src);
    } else if(cmd == "PMAALA")
    {
        parsePMAALAData(src);
    } else if(cmd == "PMAVDR")
    {
        parsePMAVDRData(src);
    }

}

PMAVDR CollectServer::parsePMAVDRData(const QString &src)
{
    com::zhichenhaixin::proto::PMAVDR data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 8) return data;
    int i = 1;
    data.set_active_mode(list[i++].toInt());
    data.set_azimuth_steering_limit(list[i++].toDouble());
    data.set_axis_active(list[i++].toInt());
    data.set_turn_rate(list[i++].toDouble());
    data.set_steering_mode(list[i++].toInt());
    data.set_steering_gain(list[i++].toInt());
    return data;
}

PMAALA CollectServer::parsePMAALAData(const QString &src)
{
    com::zhichenhaixin::proto::PMAALA data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 11) return data;
    int i = 1;
    data.set_alarm_status(list[i++].toInt());
    data.set_yaw_alarm_limit(list[i++].toDouble());
    data.set_yaw_alarm_onoff(list[i++].toInt());
    data.set_surge_alarm_limit(list[i++].toDouble());
    data.set_surge_alarm_onoff(list[i++].toInt());
    data.set_sway_alarm_limit(list[i++].toDouble());
    data.set_sway_alarm_onoff(list[i++].toInt());
    data.set_xtrack_alarm_limit(list[i++].toDouble());
    data.set_xtrack_alarm_onoff(list[i++].toInt());
    return data;
}

PMAINF CollectServer::parsePMAINFData(const QString &src)
{
    com::zhichenhaixin::proto::PMAINF data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 17) return data;
    int i = 1;
    data.set_refsys(list[i++].toInt());
    data.set_gyro(list[i++].toInt());
    data.set_mru(list[i++].toInt());
    data.set_wind(list[i++].toInt());
    data.set_estimate_lat(list[i++].toDouble());
    data.set_estimate_lat_dir(list[i++].toStdString());
    data.set_estimate_lon(list[i++].toDouble());
    data.set_estimate_lon_dir(list[i++].toStdString());
    data.set_demand_lat(list[i++].toDouble());
    data.set_demand_lat_dir(list[i++].toStdString());
    data.set_demand_lon(list[i++].toDouble());
    data.set_demand_lon_dir(list[i++].toStdString());
    data.set_estimate_heading(list[i++].toDouble());
    data.set_demand_heading(list[i++].toDouble());
    data.set_controller_gain(list[i++].toInt());
    return data;
}

PMATHF CollectServer::parsePMATHFData(const QString& src)
{
    com::zhichenhaixin::proto::PMATHF data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 10) return data;
    int i = 1;
    data.set_thruster_number(list[i++].toInt());
    data.set_thruster_status(list[i++].toStdString());
    data.set_rpm_pitch_demand(list[i++].toDouble());
    data.set_rpm_pitch_feedback(list[i++].toDouble());
    data.set_thrust_demand(list[i++].toDouble());
    data.set_thrust_feedback(list[i++].toDouble());
    data.set_azimuth_demand(list[i++].toDouble());
    data.set_azimuth_feedback(list[i++].toDouble());

    return data;
}

PMATHA  CollectServer::parsePMATHAData(const QString& src)
{
    com::zhichenhaixin::proto::PMATHA data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 8) return data;
    int i = 1;
    data.set_surge_demand(list[i++].toDouble());
    data.set_surge_feedback(list[i++].toDouble());
    data.set_sway_demand(list[i++].toDouble());
    data.set_sway_feedback(list[i++].toDouble());
    data.set_yaw_demand(list[i++].toDouble());
    data.set_yaw_feedback(list[i++].toDouble());    

    //发送给客户端的数据
    mShipInfo.set_surge_demand(data.surge_demand());
    mShipInfo.set_surge_feedback(data.surge_feedback());
    mShipInfo.set_sway_demand(data.sway_demand());
    mShipInfo.set_sway_feedback(data.sway_feedback());
    return data;
}


PMAVTG  CollectServer::parsePMAVTGData(const QString& src)
{
    com::zhichenhaixin::proto::PMAVTG data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 11) return data;
    int i = 1;
    data.set_gps_identifier(list[i++].toInt());
    data.set_true_course(list[i++].toDouble());
    i++;    //T sign
    i++;    // not used
    i++;    //magn course sign
    data.set_speed_knots(list[i++].toDouble());
    i++;    //konts sign
    data.set_speed_kmph(list[i++].toDouble());

    mShipInfo.set_ship_course(data.true_course());
    mShipInfo.set_ship_speed_ground(data.speed_kmph() / 3.6);
    return data;
}

void CollectServer::slotUSBLReciveComData(qint64 time, QByteArray data)
{
    //开始解析数据
    QString src = QString::fromStdString(data.constData());
    //取得命令的名称
    QRegExp exp("\\$([A-Z]{1,})");
    if(exp.indexIn(src) < 0) return;
    QString cmd = exp.cap(1);
    if(cmd.length() == 0) return;
    if(cmd == "GPGLL")
    {
        parseUSBLData(src);
    }
}


double   CollectServer::convertLonlatFromddmmdotmmmm(const QString &ValueStr, const QString &NSEWFlgStr)
{
    //先转换目标字符串到度
    double val = ValueStr.toDouble();
    int a = val/100.0;
    double b = val - a*100;
    b = b/60 + a;
    //确定符号
    if(NSEWFlgStr != "N" && NSEWFlgStr != "E")
    {
        b *= -1;
    }
    return b;
}

QString CollectServer::convertLonlatToddmmdotmmmm(double val)
{
    int deg = (int)val;
    double min = (val - deg) * 60;
    int zmin = (int)min;
    return QString("").sprintf("%02d%02d.%04d", deg, zmin, (int)((min-zmin) *10000) );
}

qint64  CollectServer::convertTimeFromhhmmssdotsss(const QString &valstr)
{
    QDateTime now = QDateTime::currentDateTime();
    if(valstr.length() >= 6)
    {
        int hour = valstr.mid(0, 2).toInt();
        int minute = valstr.mid(2, 2).toInt();
        int seconds = valstr.mid(4, 2).toInt();
        int mseconds = 0;
        int pos = valstr.indexOf(".");
        if(pos >= 0)
        {
            mseconds = valstr.right(valstr.length() - pos -1).toInt();
        }

        QTime t = now.time();
        t.setHMS(hour, minute, seconds, mseconds);
        now.setTime(t);
    }

    return now.toMSecsSinceEpoch();
}

QString CollectServer::convertTimeTohhmmssdotsss(qint64 time)
{
    QDateTime cur = QDateTime::fromMSecsSinceEpoch(time);
    return QString("").sprintf("%02d%02d%02d.%03d", cur.time().hour(), cur.time().minute(), cur.time().second(), cur.time().msec());
}

//$GPGLL,3723.2475,N,12158.3416,W,161229.487,A*2C
void CollectServer::parseUSBLData(const QString &src)
{
    mPlowInfo.set_plow_flag(false);
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 7) return ;
    double last_lon = mPlowInfo.plow_lon(), last_lat = mPlowInfo.plow_lat();
    //先检查是否定位成功
    mPlowInfo.set_plow_flag(list[6] == "A");
    mPlowInfo.set_plow_lat(convertLonlatFromddmmdotmmmm(list[1], list[2]));
    mPlowInfo.set_plow_lon(convertLonlatFromddmmdotmmmm(list[3], list[4]));
    //更新时间
    qint64 cur = convertTimeFromhhmmssdotsss(list[5]);
    mPlowInfo.set_plow_update_time(cur);
    mPlowInfo.set_plow_pos_mode(1);
    //计算犁设备的速度
    if(mPlowInfo.plow_pos_update_time() == 0)
    {
        mPlowInfo.set_plow_speed(0.0);

    } else
    {
        //计算两点之间的距离
        double dis = getDistanceDeg(last_lat, last_lon, mPlowInfo.plow_lat(), mPlowInfo.plow_lon());
        qint64 subtime = cur - mPlowInfo.plow_pos_update_time();
        if(subtime != 0)
        {
            double speed = (dis * 1000) / subtime ;
            mPlowInfo.set_plow_speed(speed);
        }
        mPlowInfo.set_plow_kp_dis(dis);

    }
    mPlowInfo.set_plow_pos_update_time(cur);
    return;
}

double CollectServer::convertDPLonLat(const QString &src)
{
    //ddmm.mmmm 或者dddmm.mmmm
    double total = src.toDouble();
    int deg = 0;
    double min = 0.0;
    deg = (int)(total / 100);
    min = total - deg * 100;
    return deg + min / 60;
}


PMAGGA  CollectServer::parsePMAGGAData(const QString& src)
{
    com::zhichenhaixin::proto::PMAGGA data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 17) return data;
    int i = 1;
    data.set_gps_identifier(list[i++].toInt());
    data.set_time(list[i++].toStdString());
    data.set_lat(convertDPLonLat(list[i++]));
    data.set_lat_dir(list[i++].toStdString());
    data.set_lon(convertDPLonLat(list[i++]));
    data.set_lon_dir(list[i++].toStdString());
    data.set_gps_qualifier(list[i++].toInt());
    data.set_satellites_number(list[i++].toInt());
    data.set_hdop(list[i++].toDouble());
    //更新到客户端的数据
    mShipInfo.set_ship_lat(data.lat() * (data.lat_dir() == "N" ? 1 : (-1)));
    mShipInfo.set_ship_lon(data.lon() * (data.lon_dir() == "E" ? 1 : (-1)));
    return data;
}


PMASNS  CollectServer::parsePMASNSData(const QString& src)
{
    com::zhichenhaixin::proto::PMASNS data;
    QStringList list = src.split(QRegExp("[,*]"));
    if(list.length() < 23) return data;
    int i = 1;
    data.set_date(list[i++].toStdString());
    data.set_time(list[i++].toStdString());
    data.set_gyro1_sts(list[i++].toInt());
    data.set_heading1(list[i++].toDouble());
    data.set_gyro2_sts(list[i++].toInt());
    data.set_heading2(list[i++].toDouble());
    data.set_gyro3_sts(list[i++].toInt());
    data.set_heading3(list[i++].toDouble());
    data.set_mru1_sts(list[i++].toInt());
    data.set_pitch1(list[i++].toDouble());
    data.set_roll1(list[i++].toDouble());
    data.set_heave1(list[i++].toDouble());
    data.set_mru2_sts(list[i++].toInt());
    data.set_pitch2(list[i++].toDouble());
    data.set_roll2(list[i++].toDouble());
    data.set_heave2(list[i++].toDouble());
    data.set_wind1_sts(list[i++].toInt());
    data.set_speed1(list[i++].toInt());
    data.set_speed_dir1(list[i++].toInt());
    data.set_wind2_sts(list[i++].toInt());
    data.set_speed2(list[i++].toInt());
    data.set_speed_dir2(list[i++].toInt());

    //赋值给实际的客户端信息
    //陀螺仪状态gyro
    if(data.gyro1_sts() == 2 || data.gyro1_sts() == 4)
    {
        mShipInfo.set_ship_head(data.heading1());
    } else if(data.gyro2_sts() == 2 || data.gyro2_sts() == 4)
    {
        mShipInfo.set_ship_head(data.heading2());
    } else if(data.gyro3_sts() == 2 || data.gyro3_sts() == 4)
    {
        mShipInfo.set_ship_head(data.heading3());
    }

    //MRU
    if(data.mru1_sts() == 2 || data.mru1_sts() == 4)
    {
        mShipInfo.set_ship_pitch(data.pitch1());
        mShipInfo.set_ship_roll(data.roll1());
        mShipInfo.set_heave(data.heave1());
    } else if(data.mru2_sts() == 2 || data.mru2_sts() == 4)
    {
        mShipInfo.set_ship_pitch(data.pitch2());
        mShipInfo.set_ship_roll(data.roll2());
        mShipInfo.set_heave(data.heave2());
    }
    //WIND
    if(data.wind1_sts() == 2 || data.wind1_sts() == 4)
    {
        mShipInfo.set_wind_speed(data.speed1());
        mShipInfo.set_wind_dir(data.speed_dir1());
    } else if(data.wind2_sts() == 2 || data.wind2_sts() == 4)
    {
        mShipInfo.set_wind_speed(data.speed2());
        mShipInfo.set_wind_dir(data.speed_dir2());
    }

    return data;
}

void CollectServer::slotNaviReciveComData(qint64 time, QByteArray data)
{
    //优先使用GPS和DP设备的数据
    if(mShipInfo.ship_mode() == 1 || mShipInfo.ship_mode() == 2)
    {
        //证明DP或者GPS已经在更新数据，不在处理导航设备的数据
        return;
    }
    mShipInfo.set_ship_mode(3);
    //系统对采集的数据需要进行位数判定，确保设备上传数据是正确的数据协议格式。
    //其中导航数据是85字节数，由于数据发送端数据不是按照协议一次发送对应85字节数，
    //需要对数据的位数进行判断，并将数据头文件进行判断将数据一次放入缓存中，
    //85字节数统一排列正确后才能对数据进行解析
    int lasti= -1; //记录下一次完整数据结束的位置。这里是不是会传过来下一次开始的数据有待检验
    if(mGpsBuf.isEmpty())
    {
        int i = 0;
        for(; i<data.size(); ++i)
        {
            //A9 AA是头文件，从这里开始接收完整的数据
            if(0xA9 == (data[i] & 0xFF))
            {
                break;
            }
        }
        //开始对数据进行赋值
        for(; i<data.size(); ++i)
        {
            mGpsBuf.append(data[i]);
            //达到85字节结束赋值，开始进行数据处理
            if(mGpsBuf.size() == 85)
            {
                lasti = i;
                break;
            }
        }
    }
    else
    {
        //开始接续数据
        for(int i=0; i< data.size(); ++i)
        {
            mGpsBuf.append(data[i]);
            if(mGpsBuf.size() == 85)
            {
                lasti = i;
                break;
            }
        }
    }

    if(mGpsBuf.size() >= 85)
    {
        //开始解析导航数据并发送
        parseNaviDevData4Com(mGpsBuf);
        mGpsBuf.clear();
    }

    //开始进行下一轮的数据
    if(lasti != -1 && lasti != data.size()-1)
    {
        //还有其他数据需要解析,这里暂且不考虑。
        for(int i=lasti; i<data.size(); i++)
        {
            mGpsBuf.append(data[i]);
        }
    }

}


void CollectServer::slotCableLayeReciveComData(qint64 time, QByteArray data)
{
    //布缆设备数据接口每次发送63字节数，数据按照正常协议格式全字节发送，
    //程序只需要判断字节数是否满足协议要求，未满足数据的不处理。
    //程序使用setCableLayerData4Com布缆设备数据接口，处理数据解析。

    if(mCableBuf.isEmpty())
    {
        int i = 0;
        for(; i<data.size(); ++i)
        {
            if(0xFF == (data[i] & 0xFF) && (i+1) <data.size()&& 0xFF == (data[i+1] & 0xFF))
            {
                break;
            }
        }
        for(; i<data.size(); ++i)
        {
            mCableBuf.append(data[i]);
            if(mCableBuf.size() == 63)
            {
                 break;
            }
        }
    }
    else
    {
        for(int i=0; i< data.size(); ++i)
        {
            mCableBuf.append(data[i]);
            if(mCableBuf.size() == 63)
            {
                break;
            }
        }
    }
    if(mCableBuf.size() >= 63)
    {
        //开始解析布缆数据并发送
        parseCableLayerData4Com(mCableBuf);
        mCableBuf.clear();
    }

}

QByteArray CollectServer::makeNaviDataFromFakeData(const FakeData &fake)
{
    QByteArray data;
    data.resize(85);

    data[0] = 0xA9;
    data[84] = 0xBF;
    data[1] = 0xAA;
    data[83] = 0x9A;

    data[2] = 0xFF;
    int lon = (int)(fake.lon / M_2PREP30 / 90);
    int lat = (int)(fake.lat / M_2PREP30 / 90);

    memcpy(data.data() + 3, &lon, sizeof(int));
    memcpy(data.data() + 7, &lat, sizeof(int));

    int head = (int)(60.0 / M_2PREP30 / 180);
    memcpy(data.data() + 11, &head, sizeof(int));

    int shipspd = (int)(fake.shipspd /0.01);
    memcpy(data.data() + 23, &shipspd, sizeof(int));
    memcpy(data.data() + 59, &shipspd, sizeof(int));

    int depth = 0;
    memcpy(data.data() + 39, &depth, sizeof(int));

    int wind_spd = 30;
    int wind_dir = (int)(30.0 / 180 / M_2PREP13);
    memcpy(data.data() + 43, &wind_spd, sizeof(int));
    memcpy(data.data() + 45, &wind_dir, sizeof(int));

    //qDebug()<<"fake segemnt:"<<fake.segmentCourse;

    int course = (int)(fake.segmentCourse / 180 / M_2PREP15);
    memcpy(data.data() + 61, &course, sizeof(int));

    QByteArray time = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss").toLatin1();
    memcpy(data.data() + 63, time.data(), 19);
    return data;
}

bool CollectServer::parseNaviDevData4Com(const QByteArray &data)
{
    if(data.size() != 85) return false;
    //check data format(start & end )
    if((0xA9 != (data[0] & 0xFF)) || (0xBF != (data[84] & 0xFF)) )
    {
        emit signalSendLogMsg(QString("parse navigation data failed with wrong format(%1---%2) != (A9--BF)")
                              .arg(data.left(1).toHex().toUpper().data())
                              .arg(data.right(1).toHex().toUpper().data()));
        return false;
    }
    QByteArray gps;//GPS四字节数组

    //检查当前的数据是否有效
    gps[0] = data[2] &0xFF;
    gps[1] = 0;
    gps[2] = 0;
    gps[3] = 0;
    bool lonlatflag = gps[0] & 0x01;
    bool ship_headflag = gps[0] & 0x02;
    bool gps_speedflag = gps[0] & 0x04;
    bool ship_speedflag = gps[0] & 0x10;
    bool wind_flag = gps[0] & 0x20;
    bool weather_flag = gps[0] & 0x40;
    bool depth_flag = gps[0] & 0x80;

    //1.计算经纬度
    gps.resize(4);
    gps[0] = data[3] &0xFF;
    gps[1] = data[4] &0xFF;
    gps[2] = data[5] &0xFF;
    gps[3] = data[6] &0xFF;
    mShipInfo.set_ship_lon(bytesToInt(gps) * 90 * M_2PREP30);

    //2.计算纬度
    gps[0] = data[7] &0xFF;
    gps[1] = data[8] &0xFF;
    gps[2] = data[9] &0xFF;
    gps[3] = data[10] &0xFF;
    mShipInfo.set_ship_lat(bytesToInt(gps) * 90 * M_2PREP30);

    //3.计算航向
    gps[0] = data[11] &0xFF;
    gps[1] = data[12] &0xFF;
    gps[2] = data[13] &0xFF;
    gps[3] = data[14] &0xFF;
    mShipInfo.set_ship_head(bytesToInt(gps) * 180 * M_2PREP30);

    //4.计算航速单位（km）
    //qDebug()<<"data[23]:"<<data[23]<<" data[24]:"<<data[24];
    gps[0] = data[23] &0xFF;
    gps[1] = data[24] &0xFF;
    gps[2] = 0;
    gps[3] = 0;
    mShipInfo.set_ship_speed(bytesToInt(gps) * 0.01 / 3.6);

    //计算对地速度
    gps[0] = data[59] &0xFF;
    gps[1] = data[60] &0xFF;
    gps[2] = 0;
    gps[3] = 0;
    mShipInfo.set_ship_speed_ground(bytesToInt(gps) * 0.01 / 3.6); //这里默认速度值都为正数

    //计算海深
    gps[0] = data[39] &0xFF;
    gps[1] = data[40] &0xFF;
    gps[2] = data[41] &0xFF;
    gps[3] = data[42] &0xFF;
    mPlowInfo.set_plow_water_depth(bytesToInt(gps) * 0.1);
    //计算风速风向
    gps[0] = data[43] &0xFF;
    gps[1] = data[44] &0xFF;
    gps[2] = 0;
    gps[3] = 0;
    mShipInfo.set_wind_speed(bytesToInt(gps) * 0.01);

    gps[0] = data[45] &0xFF;
    gps[1] = data[46] &0xFF;
    gps[2] = 0;
    gps[3] = 0;
    mShipInfo.set_wind_dir(bytesToInt(gps) * 180 * M_2PREP13);

    //计算航迹向
    gps[0] = data[61] &0xFF;
    gps[1] = data[62] &0xFF;
    gps[2] = 0;
    gps[3] = 0;
    mShipInfo.set_ship_course(bytesToInt(gps) * 180 * M_2PREP15);

    //5计算时间
    gps.resize(19);
    gps[0] = data[63];
    gps[1] = data[64];
    gps[2] = data[65];
    gps[3] = data[66];
    gps[4] = data[67];
    gps[5] = data[68];
    gps[6] = data[69];
    gps[7] = data[70];
    gps[8] = data[71];
    gps[9] = data[72];
    gps[10] = data[73];
    gps[11] = data[74];
    gps[12] = data[75];
    gps[13] = data[76];
    gps[14] = data[77];
    gps[15] = data[78];
    gps[16] = data[79];
    gps[17] = data[80];
    gps[18] = data[81];

    qint64 cur = QDateTime::fromString(QString::fromStdString(gps.toStdString()), "yyyy.MM.dd hh:mm:ss").toMSecsSinceEpoch();
    mShipInfo.set_ship_update_time(cur);
    mPlowInfo.set_plow_update_time(cur);

//    qDebug()<<"shipspd:"<<mShipInfo.ship_speed()<<" "<<mShipInfo.ship_speed_ground()<<" course:"<<mShipInfo.ship_course();
    return true;
}

bool CollectServer::parseCableLayerData4Com(const QByteArray &data)
{
    qint64 time = QDateTime::currentMSecsSinceEpoch();
//    emit signalSendLogMsg(QString("parse cable layer data for com, receive data size:%1").arg(data.size()));
    if(data.size() != 63) return false;
    //check data format(start & end )
    if((0xFF != (data[0] & 0xFF)) || (0xFF != (data[1] & 0xFF)) || (0x0D != (data[61] & 0xFF)) || (0x0A != (data[62] & 0xFF)) )
    {
        emit signalSendLogMsg(QString("parse navigation data failed with wrong format(%1---%2) != (FFFF--0D0A)")
                              .arg(data.left(2).toHex().toUpper().data())
                              .arg(data.right(2).toHex().toUpper().data()));
        return false;
    }

    //计算正负号
    bool glj_is_positive  = data[3] & 0x01;         //鼓轮机海缆长度
    bool ltj_is_positive  = data[3] & 0x02;         //轮胎机海缆长度
    bool tl_is_positive   = data[3] & 0x04;         //拖缆长度
    bool qd_is_positive   = data[3] & 0x08;         //脐带缆长度
    //add
    bool hq_is_positive   = data[3] & 0x10;         //犁横倾
    bool zq_is_positive   = data[3] & 0x20;         //犁纵倾

    QByteArray cab; //四字节数组
    cab.resize(4);


    mLayInfo.set_cable_update_time(time);
    //1.鼓轮机海缆张力（5-8）
    cab[0] = data[4] &0xFF;
    cab[1] = data[5] &0xFF;
    cab[2] = data[6] &0xFF;
    cab[3] = data[7] &0xFF;
    mLayInfo.set_glj_cb_tension(bytesToInt(cab) /10/ 100.00);
//    qDebug()<<"tension:"<<mLayInfo.glj_cb_tension();

    //1.鼓轮机海缆速度（9-12）
    cab[0] = data[8] &0xFF;
    cab[1] = data[9] &0xFF;
    cab[2] = data[10] &0xFF;
    cab[3] = data[11] &0xFF;
    mLayInfo.set_glj_cb_speed(bytesToInt(cab) / 100.00);

    //1.鼓轮机海缆长度绝对值（13-16）
    cab[0] = data[12] &0xFF;
    cab[1] = data[13] &0xFF;
    cab[2] = data[14] &0xFF;
    cab[3] = data[15] &0xFF;
    mLayInfo.set_glj_cb_length(bytesToInt(cab) / 100.00);

    //1.轮胎机海缆张力（17-20）
    cab[0] = data[16] &0xFF;
    cab[1] = data[17] &0xFF;
    cab[2] = data[18] &0xFF;
    cab[3] = data[19] &0xFF;
    mLayInfo.set_ltj_cb_tension(bytesToInt(cab) /10/ 100.00);

    //1.轮胎机海缆速度（21-24）
    cab[0] = data[20] &0xFF;
    cab[1] = data[21] &0xFF;
    cab[2] = data[22] &0xFF;
    cab[3] = data[23] &0xFF;
    mLayInfo.set_ltj_cb_speed(bytesToInt(cab) / 100.00);

    //1.轮胎机海缆长度绝对值（25-28）
    cab[0] = data[24] &0xFF;
    cab[1] = data[25] &0xFF;
    cab[2] = data[26] &0xFF;
    cab[3] = data[27] &0xFF;
    mLayInfo.set_ltj_cb_length(bytesToInt(cab) / 100.00);

    //1.拖缆张力（29-32）
    cab[0] = data[28] &0xFF;
    cab[1] = data[29] &0xFF;
    cab[2] = data[30] &0xFF;
    cab[3] = data[31] &0xFF;
    mLayInfo.set_tl_tension(bytesToInt(cab) / 100.00);
    mPlowInfo.set_plow_tow_tension(mLayInfo.tl_tension());

    //1.拖缆长度绝对值(33-36)
    cab[0] = data[32] &0xFF;
    cab[1] = data[33] &0xFF;
    cab[2] = data[34] &0xFF;
    cab[3] = data[35] &0xFF;
    mLayInfo.set_tl_length(bytesToInt(cab) / 100.00);
    mPlowInfo.set_plow_tow_length(mLayInfo.tl_length());

    //1.脐带张力KN（37-40）
    cab[0] = data[36] &0xFF;
    cab[1] = data[37] &0xFF;
    cab[2] = data[38] &0xFF;
    cab[3] = data[39] &0xFF;
    mLayInfo.set_qd_tension(bytesToInt(cab)/10 / 100.00);

    //1.脐带长度绝对值(41-44)
    cab[0] = data[40] &0xFF;
    cab[1] = data[41] &0xFF;
    cab[2] = data[42] &0xFF;
    cab[3] = data[43] &0xFF;
    mLayInfo.set_qd_length(bytesToInt(cab) / 100.00);

    mPlowInfo.set_plow_update_time(time);
    //1.海缆埋深(cm)(45-48)
    cab[0] = data[44] &0xFF;
    cab[1] = data[45] &0xFF;
    cab[2] = data[46] &0xFF;
    cab[3] = data[47] &0xFF;
    mPlowInfo.set_plow_buried_depth(bytesToInt(cab) / 100.00);

    //1.犁横倾(49-52)
    cab[0] = data[48] &0xFF;
    cab[1] = data[49] &0xFF;
    cab[2] = data[50] &0xFF;
    cab[3] = data[51] &0xFF;
    mPlowInfo.set_plow_roll(bytesToInt(cab) / 100.00 * (hq_is_positive ?  1: -1));

    //1.犁纵倾(53-56)
    cab[0] = data[52] &0xFF;
    cab[1] = data[53] &0xFF;
    cab[2] = data[54] &0xFF;
    cab[3] = data[55] &0xFF;
    mPlowInfo.set_plow_pitch(bytesToInt(cab) / 100.00 * (zq_is_positive ?  1: -1));

    //1.灯状态(57-60)
    cab[0] = data[56] &0xFF;
    cab[1] = data[57] &0xFF;
    cab[2] = data[58] &0xFF;
    cab[3] = data[59] &0xFF;
    mLayInfo.set_light_status(bytesToInt(cab));

    //更新海缆长度
    mLayInfo.set_cable_length(mLayInfo.ltj_cb_length());
    mLayInfo.set_cable_payout_speed(mLayInfo.ltj_cb_speed());
    mLayInfo.set_meter_source(2);
    //qDebug()<<"cable length:"<<mLayInfo.cable_length();
    return true;
}

QByteArray CollectServer::makeCableDataFromFakeData(const FakeData &fake)
{
    QByteArray data;
    data.resize(63);
    data[0] = 0xFF;
    data[1] = 0xFF;
    data[61] = 0x0D;
    data[62] = 0x0A;
    data[3] = 0x3F;
    int power = 1000000;
    memcpy(data.data() + 4, &power, sizeof(int));//
    int cablespd = (int)(fake.cableSpd /3.6 * 100);
    memcpy(data.data() + 8, &cablespd, sizeof(int));
    int cablelen = (int)(fake.pointCableTotaLength *1000 * 100);
    memcpy(data.data() + 12, &cablelen, sizeof(int));

    memcpy(data.data() + 16, &power, sizeof(int));//
    memcpy(data.data() + 20, &cablespd, sizeof(int));
    memcpy(data.data() + 24, &cablelen, sizeof(int));

    memcpy(data.data() + 28, &power, sizeof(int));//
    memcpy(data.data() + 32, &cablelen, sizeof(int));
    memcpy(data.data() + 36, &power, sizeof(int));//
    memcpy(data.data() + 40, &cablelen, sizeof(int));

    int depth = 2000;
    memcpy(data.data() + 44, &depth, sizeof(int));//
    int pitch = 30 * 100;
    int roll = 15 * 100;
    memcpy(data.data() + 48, &pitch, sizeof(int));//
    memcpy(data.data() + 52, &roll, sizeof(int));

    int light = 1;
    memcpy(data.data() + 56, &light, sizeof(int));//

    return data;
}

qint64  CollectServer::bytesToInt(QByteArray bytes)
{
    qint64  addr = bytes[0] & 0x000000FF;
    addr |= ((bytes[1] << 8) & 0x0000FF00);
    addr |= ((bytes[2] << 16) & 0x00FF0000);
    addr |= ((bytes[3] << 24) & 0xFF000000);
    return addr;
}


void CollectServer::slotTimeOut()
{
    return;
    //产生模拟数据
    //根据不同的状态产生不同的模拟数据。停止施工时，定位到桐梓林（104.072986,30.624211）;
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
    if(!mSocket) return;
    static int largeCmd = 0;
    if(mFakeConstructionMode != 0)
    {
        //正在施工，暂停施工的状态
        if(mFakeIndex >= mFakeDataList.size())
        {
            mFakeIndex --;
        }
        if(mFakeIndex < mFakeDataList.size())
        {
            mFakeData = mFakeDataList.at(mFakeIndex);
        }
    }

    if(largeCmd % 2 == 0)
    {
        //发送GPS命令
        if(mDevInfo.gsp_checked_flg())
        {
            //发送GPS命令
//            QString gpsstr = tr("$GPGGA,%1,%2,N,%3,E,4,07,1.5,6.571,M,8.942,M,0.7,0016*7B").arg(convertTimeTohhmmssdotsss(time))
//                    .arg(convertLonlatToddmmdotmmmm(mFakeData.lat))
//                    .arg(convertLonlatToddmmdotmmmm(mFakeData.lon));
            QString gpsstr = "$GPRMC,104325.00,A,3121.4672621,N,12136.7704286,E,0.0,,241117,5.4,E,N*18\r\n"
                             "$GPGLL,3121.4672621,N,12136.7704286,E,104325.00,A,D*61\r\n"
                             "$GPGGA,104325.00,3121.4672621,N,12136.7704286,E,2,15,0.7,30.6600,M,8.3240,M,18.0,0281*73\r\n"
                             "$GPVTG,191.3,T,185.8,M,0.0,N,0.1,K,D*29\r\n"
                             "$GPRMC,104326.00,A,3121.4672653,N,12136.7704288,E,0.0,,241117,5.4,E,N*10\r\n"
                             "$GPGLL,3121.4672653,N,12136.7704288,E,104326.00,A,D*69\r\n"
                             "$GPGGA,104326.00,3121.4672653,N,12136.7704288,E,2,15,0.7,30.6614,M,8.3240,M,19.0,0281*7F\r\n"
                             "$GPVTG,23.0,T,17.6,M,0.0,N,0.0,";
            slotRecvComData(mDevComParamsMap[MSG_GPS].mName, MSG_GPS, time, gpsstr.toLatin1());
            QString vtgstr = tr("$GPVTG,%1,T,10.02,M,10.02,N,%2,K,1234*9A").arg(mFakeData.segmentCourse).arg(mFakeData.shipspd);
            //slotRecvComData(mDevComParamsMap[MSG_GPS].mName, MSG_GPS, time, vtgstr.toLatin1());
            QString hdtStr = tr("$GPHDT,%1,T*00").arg(mFakeData.head);
            //slotRecvComData(mDevComParamsMap[MSG_GPS].mName, MSG_GPS, time, hdtStr.toLatin1());

        }

        //发送计米器
        if(mDevInfo.metercounter_checked_flg())
        {
            COMDEVPARAM param = mDevComParamsMap[MSG_METER_COUNTER];
            //QString meterStr = QString("").sprintf("#%02d\r=+%d.A", param.mMessageNum1, (int)(mFakeData.pointCableTotaLength * 1000));
            QString meterStr = QString("").sprintf("X%02d>+%010d\r", param.mMessageNum1, (int)(mFakeData.pointCableTotaLength * 1000));
            //qDebug()<<"ori length:"<<mFakeData.pointCableTotaLength<<" src:"<<meterStr;
            slotRecvComData(mDevComParamsMap[MSG_METER_COUNTER].mName, MSG_METER_COUNTER, time, meterStr.toLatin1());
        }

        //张力
        QByteArray tensionArray;
        tensionArray.resize(10);
        tensionArray[0] = 0xBB;
        tensionArray[1] = 0xBB;
        tensionArray[2] = 0xBB;
        tensionArray[3] = 0x01;
        tensionArray[4] = 0xA1;
        tensionArray[5] = 0x00;
        tensionArray[6] = 0x45;
        tensionArray[7] = 0x03;
        tensionArray[8] = 0xFF;
        tensionArray[9] = 0xA2;
        slotRecvComData(mDevComParamsMap[MSG_TENSION_DEV].mName, MSG_TENSION_DEV, time, tensionArray);


        if(mFakeConstructionMode == 1)mFakeIndex++;

        //发送水下40172数据
        if(mDevInfo.under_4017_checked_flg())
        {
            COMDEVPARAM param = mDevComParamsMap[MSG_UNDER_4017];
            QString str40172 = QString("").sprintf("#%02d\r>+02.095+01.575+00.771+02.357+02.358+02.358+02.357+02.268", param.mMessageNum2);
            slotRecvComData(mDevComParamsMap[MSG_UNDER_4017].mName, MSG_UNDER_4017, time, str40172.toLatin1());
        }
        //发送导航数据
        if(mDevInfo.navi_device_checked_flg())
        {
            slotRecvComData(mDevComParamsMap[MSG_NAVI_DEV].mName, MSG_NAVI_DEV, time, makeNaviDataFromFakeData(mFakeData));
        }


    } else
    {
        //发送布缆数据
        if(mDevInfo.cable_integrated_checked_flg())
        {
            slotRecvComData(mDevComParamsMap[MSG_CABLE_DEV].mName, MSG_CABLE_DEV, time, makeCableDataFromFakeData(mFakeData));
        }

        //发送水上4017数据，水下4017数据。水上水下HMR数据，USBL数据
        if(mDevInfo.surface_4017_checked_flg())
        {
            COMDEVPARAM param = mDevComParamsMap[MSG_SURFACE_4017];
            QString str4017 = QString("").sprintf("#%02d\r>+02.095+01.575+00.771+02.357+02.358+02.358+02.357+02.268", param.mMessageNum1);
            slotRecvComData(mDevComParamsMap[MSG_SURFACE_4017].mName, MSG_SURFACE_4017, time, str4017.toLatin1());
        }

        if(mDevInfo.under_4017_checked_flg())
        {
            COMDEVPARAM param = mDevComParamsMap[MSG_UNDER_4017];
            QString str40171 = QString("").sprintf("#%02d\r>+02.095+01.575+00.771+02.357+02.358+02.358+02.357+02.268", param.mMessageNum1);
            slotRecvComData(mDevComParamsMap[MSG_UNDER_4017].mName, MSG_UNDER_4017, time, str40171.toLatin1());
        }

        if(mDevInfo.surface_hmr3000_checked_flg())
        {
            COMDEVPARAM param = mDevComParamsMap[MSG_SURFACE_HMR];
            QString str3000("$PTNTHPR,130.7,N,-9.1,N,1.2,N*17");
            slotRecvComData(mDevComParamsMap[MSG_SURFACE_HMR].mName, MSG_SURFACE_HMR, time, str3000.toLatin1());
        }
    }

    largeCmd++;
}

void CollectServer::slotSendComData(const QByteArray& pTopic, const QByteArray& pData)
{
    if(!mSocket) return;
    QByteArray time = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
    zmq_send(mSocket, pTopic.data(), pTopic.size(), ZMQ_SNDMORE);
    zmq_send(mSocket, time.data(), time.size(), ZMQ_SNDMORE);
    zmq_send(mSocket, pData.data(), pData.size(), 0);
    //emit signalSendLogMsg(QString::fromUtf8(pData));
}

void CollectServer::slotRecvConstructionCommand(const QByteArray &pTopic, const QByteArray &pData)
{
    //解析命令
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(pData, &error);
    if(error.error != QJsonParseError::NoError)
    {
        qDebug()<<"parse config update info failed for wrong json format string.";
        return;
    }
    if(!doc.isObject())
    {
        qDebug()<<"parse config update info failed for wrong json format string.";
        return;
    }
    QJsonObject obj = doc.object();
    //开始具体的解析
    QString host_name = obj.value(HOST_KEY).toString();
    mConstructInfo.set_host_name(host_name.toStdString());
    QJsonValue val = obj.value(CLIENT_UPDATE_KEY);
    QJsonArray arr;
    if(val.isObject())
    {
        //只是更新了单个命令
        arr.append(val);
    } else if(val.isArray())
    {
        //更新了多个命令
        arr = val.toArray();
    }
    //开始解析
    for(int i=0; i<arr.size(); i++)
    {
        if(!arr[i].isObject()) continue;
        QJsonObject obj = arr[i].toObject();
        int cmd = obj.value(CLIENT_UPDATE_CMD).toInt();
        if(cmd == JSON_PARSE_CMD_CONSTRUCTION_START)
        {
            //先保存是否正在施工的情况
            int oldplanID = mConstructInfo.plan_id();
            //外部客户端发起了施工开始
            QJsonObject wkobj = obj.value(CLIENT_UPDATE_VAL).toObject();
            mConstructInfo.set_iscontructed(true);
            mConstructInfo.set_plan_id(wkobj.value("plan").toInt());
            mConstructInfo.set_route_id(wkobj.value("route").toInt());
            mConstructInfo.set_project_id(wkobj.value("project").toInt());
            if(oldplanID != mConstructInfo.plan_id())
            {
                startFakeServer();
            } else
            {
                startFakeServer(mFakeIndex);
            }
        } else if(cmd == JSON_PARSE_CMD_CONSTRUCTION_END)
        {
            //外部客户端发起了施工结束
            QJsonObject wkobj = obj.value(CLIENT_UPDATE_VAL).toObject();
            mConstructInfo.set_iscontructed(false);
            mConstructInfo.set_plan_id(wkobj.value("plan").toInt());
            mConstructInfo.set_route_id(wkobj.value("route").toInt());
            mConstructInfo.set_project_id(wkobj.value("project").toInt());
            stopFakeServer();
        }
    }
    emit signalSendLogMsg(QString::fromUtf8(pData));


    //发送给客户端
    slotSendComData(pTopic, pData);
}

double CollectServer::degToRad(double d)
{
    return ((d * GLOB_PI) / 180.0);
}

double CollectServer::radToDeg(double r)
{
    return ((r * 180.0) / GLOB_PI);
}

void CollectServer::distbear_to_latlon(double lat1, double lon1, double dist, double brng, double &lat_out, double &lon_out)
{
    brng = degToRad(brng);

    lat1=degToRad(lat1);
    lon1=degToRad(lon1);
    double R=6378000.0; //Earth radius 6372795.0
    double lat2 = asin( sin(lat1)*cos(dist/R) + cos(lat1)*sin(dist/R)*cos(brng) );
    double lon2 = lon1 + atan2(sin(brng)*sin(dist/R)*cos(lat1),cos(dist/R)-sin(lat1)*sin(lat2));

    lat_out=radToDeg(lat2);
    lon_out=radToDeg(lon2);
}

double CollectServer::getDistanceDeg(double lat1, double lon1, double lat2, double lon2)
{

    double* course1 = NULL;
    double* course2 = NULL;

    QPointF p1(lon1,lat1);
    QPointF p2(lon2,lat2);
    if ( p1.x() == p2.x() && p1.y() == p2.y() )
    {
        return 0;
    }

    // ellipsoid
    //double a = mSemiMajor;
    //double b = mSemiMinor;
    //double f = 1 / mInvFlattening;
    double a = 6378137.0;
    double b = 6356752.314245;
    double f = 1.0 / 298.257223563;

    double p1_lat = degToRad( p1.y() ), p1_lon = degToRad( p1.x() );
    double p2_lat = degToRad( p2.y() ), p2_lon = degToRad( p2.x() );

    double L = p2_lon - p1_lon;
    double U1 = atan(( 1 - f ) * tan( p1_lat ) );
    double U2 = atan(( 1 - f ) * tan( p2_lat ) );
    double sinU1 = sin( U1 ), cosU1 = cos( U1 );
    double sinU2 = sin( U2 ), cosU2 = cos( U2 );
    double lambda = L;
    double lambdaP = 2 * GLOB_PI;

    double sinLambda = 0;
    double cosLambda = 0;
    double sinSigma = 0;
    double cosSigma = 0;
    double sigma = 0;
    double alpha = 0;
    double cosSqAlpha = 0;
    double cos2SigmaM = 0;
    double C = 0;
    double tu1 = 0;
    double tu2 = 0;

    int iterLimit = 20;
    while ( qAbs( lambda - lambdaP ) > 1e-12 && --iterLimit > 0 )
    {
        sinLambda = sin( lambda );
        cosLambda = cos( lambda );
        tu1 = ( cosU2 * sinLambda );
        tu2 = ( cosU1 * sinU2 - sinU1 * cosU2 * cosLambda );
        sinSigma = sqrt( tu1 * tu1 + tu2 * tu2 );
        cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
        sigma = atan2( sinSigma, cosSigma );
        alpha = asin( cosU1 * cosU2 * sinLambda / sinSigma );
        cosSqAlpha = cos( alpha ) * cos( alpha );
        cos2SigmaM = cosSigma - 2 * sinU1 * sinU2 / cosSqAlpha;
        C = f / 16 * cosSqAlpha * ( 4 + f * ( 4 - 3 * cosSqAlpha ) );
        lambdaP = lambda;
        lambda = L + ( 1 - C ) * f * sin( alpha ) *
                ( sigma + C * sinSigma * ( cos2SigmaM + C * cosSigma * ( -1 + 2 * cos2SigmaM * cos2SigmaM ) ) );
    }

    if ( iterLimit == 0 )
        return -1;  // formula failed to converge

    double uSq = cosSqAlpha * ( a * a - b * b ) / ( b * b );
    double A = 1 + uSq / 16384 * ( 4096 + uSq * ( -768 + uSq * ( 320 - 175 * uSq ) ) );
    double B = uSq / 1024 * ( 256 + uSq * ( -128 + uSq * ( 74 - 47 * uSq ) ) );
    double deltaSigma = B * sinSigma * ( cos2SigmaM + B / 4 * ( cosSigma * ( -1 + 2 * cos2SigmaM * cos2SigmaM ) -
                                                                B / 6 * cos2SigmaM * ( -3 + 4 * sinSigma * sinSigma ) * ( -3 + 4 * cos2SigmaM * cos2SigmaM ) ) );
    double s = b * A * ( sigma - deltaSigma );

    if ( course1 )
    {
        *course1 = atan2( tu1, tu2 );
    }
    if ( course2 )
    {
        // PI is added to return azimuth from P2 to P1
        *course2 = atan2( cosU1 * sinLambda, - sinU1 * cosU2 + cosU1 * sinU2 * cosLambda ) + GLOB_PI;
    }

    return s;

    //return get_distance_rad(degToRad(lat1), degToRad(lon1), degToRad(lat2), degToRad(lon2));
}

void CollectServer::slotSetupSimulationData()
{
    mFakeDataList.clear();
    if(!readFile())
    {
        emit signalSendLogMsg("initialize ship fake data failed.");
        return;
    }
    qDebug()<<"setup simulationdata finished";
    if(mFakeDataList.length() && mFakeConstructionMode == 0)
    {
        mFakeData = mFakeDataList[0];
        emit signalSendLogMsg("simulation initialized, ship move to start point.");
    }
}

/*-------------------------------------------
    *
    * 已知两点坐标，求两点连线的方位角
    *
    ---------------------------------------------*/
double CollectServer::calcAzimuth(double lon1, double lat1, double lon2, double lat2)
{
    double arvLat,antPos;
    double pi= GLOB_PI;
    arvLat=(lat1+lat2)/2;
    if((lat1-lat2)==0){
        if(lon1 == lon2)
        {
            antPos = 0;
        } else if(lon2 > lon1)
        {
            antPos=90;
        } else
        {
            antPos = 270;
        }
    }else{
        antPos=atan((lon1-lon2)*cos(ang2Cir(arvLat))/(lat1-lat2))*180/pi;
    }

    if(lat1>lat2){
        antPos+=180;
    }

    if(antPos<0){
        antPos+=360;
    }
    antPos*=10;
    antPos=int(antPos+0.5);
    antPos/=10;

    return antPos;
}


//Sample: X00>+0000000123\r 一共16字节，使用时
//00X00>+000000012;
//去掉\r一共15字节
void CollectServer::slotMeterCounterReciveComData(qint64 time, QByteArray data, const QString& msgnum)
{
    //计米器不需要解析地址，如果符合格式长度值就进行解析
    QString src = QString::fromStdString(data.constData());
    if(src.contains("\r")) src.remove("\r");
    mMeterCounterCmdStr += src;
    QList<int> cmdIndexList;
    int index = 0;
    while ((index = mMeterCounterCmdStr.indexOf("X", index)) >= 0) {
        cmdIndexList.append(index);
        index += 10;
    }
    if(cmdIndexList.length() == 0) return;
    //这里每次取第一笔据作为有效数据
    qint64 curNum = 0;
    double used_cable_length = mSavedCableLength + mLayInfo.meter_counter_initlen();
    for(int i=0; i<cmdIndexList.length(); i++)
    {
        //取得命令对应的子字符串
        int startIndex = cmdIndexList[i];
        if(startIndex + 15 > mMeterCounterCmdStr.length()) break;
        src = mMeterCounterCmdStr.mid(startIndex, 15);
        //取得编号
        //QString num = src.mid(1, 2);
        //if(num != msgnum) return;
        QRegExp exp("([\\+\\-]{1}\\d{1,})");
        if(exp.indexIn(src) < 0) continue;
        curNum = exp.cap(1).toLongLong();
        break;
    }
    if(mFirstRecvMeterCounterFlag)
    {
        //第一次接收到数据，不做处理，直接输出起始数据
        mFirstRecvMeterCounterFlag = false;
    } else
    {
        //计算前后两次读数的差值，差值作为实际的海缆长度
        qint64 sub = curNum - mLastMeterCounterNum;
        if(fabs(sub)<mMeterConterMax)
        {
            double sub_cable_len = sub * mLayInfo.meter_counter_coeff();
            mSavedCableLength += sub_cable_len;
            used_cable_length += sub_cable_len;
        } else
        {
            //超出了设定的最大值范围，计数器开始从头计数的情况，不处理
        }
    }
    mLayInfo.set_cable_update_time(time);
    qint64 lastTime = mLayInfo.meter_counter_time();
    double speed = mLayInfo.cable_payout_speed();
    if(lastTime != 0)
    {

//            //已经开始计数了
//            double sublen = used_cable_length - mLayInfo.cable_length();
//            qint64 subtime = (time - lastTime);
//            if(subtime > 0)
//            {
//                double cal_speed = sublen *1000 / subtime;
//                if(fabs(cal_speed) > 0)
//                {
//                    speed = fabs(cal_speed);
//                }
//            }
        //计算当前时间和上一次速度的时间间隔
        int subtime = time - lastTime;
        if(subtime >= mSpeedTimeGap * 1000)
        {
            double sublen = used_cable_length - mLastCalCableLength;
            double cal_speed = sublen *1000 / subtime;
            if(fabs(cal_speed) > 0)
            {
                speed = fabs(cal_speed);
            }
            qDebug()<<"cable_speed:"<<speed<<" timegap:"<<mSpeedTimeGap<<" subtime:"<<subtime / 1000.0<<QDateTime::fromMSecsSinceEpoch(time);
            mLayInfo.set_meter_counter_time(time);
            mLastCalCableLength = used_cable_length;

        }
    } else
    {
        mLayInfo.set_meter_counter_time(time);
        mLastCalCableLength = used_cable_length;
    }
    mLayInfo.set_cable_length(used_cable_length);
    mLayInfo.set_cable_payout_speed(speed);
    mLayInfo.set_meter_source(1);
    mLastMeterCounterNum = curNum;
    //检查是否有未处理的不完整的命令
    int lastIndex = cmdIndexList.last();
    if(lastIndex + 15 > mMeterCounterCmdStr.length())
    {
        mMeterCounterCmdStr = mMeterCounterCmdStr.mid(lastIndex, mMeterCounterCmdStr.length() - lastIndex);
    }
    //保存实时施工的数据
    Utils::Profiles::instance()->setValue("SaveData", "Cable_Length", mSavedCableLength);
}

void CollectServer::slotUnderHMRReciveComData(qint64 time, QByteArray data)
{
    mPlowInfo.set_plow_update_time(time);
    HMR3000 hmr;
    if(parseHMR3000Data(QString::fromStdString(data.toStdString()), hmr))
    {
        mPlowInfo.set_plow_head(hmr.head);
        mPlowInfo.set_plow_pitch(hmr.pitch);
        mPlowInfo.set_plow_roll(hmr.roll);
    }
}

void CollectServer::slotUnderWater4017ReciveComData(qint64 time, QByteArray data, const QString& msgnum1, const QString& msgnum2)
{
    mPlowInfo.set_plow_update_time(time);
    QString src = QString::fromStdString(data.constData());
    if(src.length() < 3) return;
    //取得编号
    QString msgnum = src.mid(2, 1);

    if(src.length() < 7* m40171ChannelNum+5) return;
    QList<double> valist;
    for(int i=0; i<m40171ChannelNum; i++)
    {
        valist.append(src.mid(5+i*7, 7).toDouble());
    }
    if(msgnum == msgnum1)
    {
        //更新各个参数的值
        mPlowInfo.set_plow_water_depth(get4017DetailValue(valist, m40171WaterDepthParam));
        mPlowInfo.set_plow_boots_angle(get4017DetailValue(valist, m40171BootsAngleParam));
        mPlowInfo.set_plow_buried_depth(mPlowInfo.plow_boots_length() * sin(mPlowInfo.plow_boots_angle()));
        mUnder4017.set_under_4017_boots_angle(get4017DetailValue(valist, m40171BootsAngleParam));
        mUnder4017.set_under_4017_water_depth(get4017DetailValue(valist, m40171WaterDepthParam));
        mUnder4017.set_under_4017_touch_down_p1(get4017DetailValue(valist, m40171TouchDown1Param));
        mUnder4017.set_under_4017_touch_down_p2(get4017DetailValue(valist, m40171TouchDown2Param));
        mUnder4017.set_under_4017_touch_down_p3(get4017DetailValue(valist, m40171TouchDown3Param));
        mUnder4017.set_under_4017_touch_down_p4(get4017DetailValue(valist, m40171TouchDown4Param));
    } else if(msgnum == msgnum2)
    {
        mUnder4017.set_under_4017_pull1(get4017DetailValue(valist, m40172Pull1Param));
        mUnder4017.set_under_4017_pull2(get4017DetailValue(valist, m40172Pull2Param));
        mUnder4017.set_under_4017_pull3(get4017DetailValue(valist, m40172Pull3Param));
        mUnder4017.set_under_4017_lpump(get4017DetailValue(valist, m40172LeftPumpParam));
        mUnder4017.set_under_4017_rpump(get4017DetailValue(valist, m40172RightPumpParam));
        mPlowInfo.set_plow_left_pump(get4017DetailValue(valist, m40172LeftPumpParam));
        mPlowInfo.set_plow_right_pump(get4017DetailValue(valist, m40172RightPumpParam));
    }

}

//***********************************************************************
//Description: AD4017模块串口格式解码
//Inputs: string str 串口接收到的字符串,CR
//Outputs:double value[] 解析串口字符串到double数组
//Sample: >+5.2111+4.3856+2.9867+5.6795+4.2965+3.6489+2.6491+4.2367(Cr)
//***********************************************************************
void CollectServer::slotSurface4017ReciveComData(qint64 time, QByteArray data, const QString& msgnum)
{
    QString src = QString::fromStdString(data.constData());
    if(src.length() < 3) return;
    //取得编号
    QString num = src.mid(2, 1);

    if(src.length() < 7* mSurfaceChannelNum+5) return;
    QList<double> valist;
    for(int i=0; i<mSurfaceChannelNum; i++)
    {
        valist.append(src.mid(5+i*7, 7).toDouble());
    }
    if(num == msgnum)
    {
        //更新各个参数的值
        mLayInfo.set_cable_update_time(time);
        mLayInfo.set_tl_tension(get4017DetailValue(valist, mSurfacePull3Param));
        mPlowInfo.set_plow_tow_tension(mLayInfo.tl_tension());
        mSurface4017.set_surface_4017_pull1(get4017DetailValue(valist, mSurfacePull1Param));
        mSurface4017.set_surface_4017_pull2(get4017DetailValue(valist, mSurfacePull2Param));
        mSurface4017.set_surface_4017_pull3(get4017DetailValue(valist, mSurfacePull3Param));
        mSurface4017.set_surface_4017_lpump(get4017DetailValue(valist, mSurfaceLeftPumpParam));
        mSurface4017.set_surface_4017_rpump(get4017DetailValue(valist, mSurfaceRightPumpParam));
    }
}

double CollectServer::get4017DetailValue(const QList<double>& vals, const DEVANALYPARAM &param)
{
    return get4017DetailValue(vals, param.channel, param.deviation, param.deviationCoefficient);
}

double CollectServer::get4017DetailValue(const QList<double>& vals, const QString& channel_name, double offset, double coeff)
{
    if(channel_name.length() < 3) return 0.0;
    int ch = channel_name.right(channel_name.length()- 2).toInt();
    if (ch >= vals.size()) return 0.0;

    return (vals[ch] - offset) * coeff;
}

void CollectServer::slotSurfaceHMRReciveComData(qint64 time, QByteArray data)
{
    HMR3000 hmr;
    if(parseHMR3000Data(QString::fromStdString(data.toStdString()), hmr))
    {        
        mShipInfo.set_ship_update_time(time);
        mShipInfo.set_ship_head(hmr.head);
        mShipInfo.set_ship_pitch(hmr.pitch);
        mShipInfo.set_ship_roll(hmr.roll);
    }

}

//$GPGGA,014434.70,3817.13334637,N,12139.72994196,E,4,07,1.5,6.571,M,8.942,M,0.7,0016*7B
void CollectServer::slotGPSReciveComData(qint64 time, QByteArray data)
{
    //开始解析数据
    mGpsCmdStr += QString::fromStdString(data.constData());
    //取得命令的名称
    QRegExp exp_end("\\r\\n");
    //test
    QList<int> cmdIndexList;
    int index = 0;
    while((index = exp_end.indexIn(mGpsCmdStr, index)) >= 0)
    {
        //qDebug()<<"cmd:"<<exp_end.cap()<<index;
        cmdIndexList.append(index);
        index += exp_end.cap().length();
    }
    if(cmdIndexList.length() == 0) return;
    //已经存在一个完整的命令，开始解析
    int cmd_start = 0;
    for(int i=0; i<cmdIndexList.length(); i++)
    {
        //qDebug()<<"cmdindex:"<<cmdIndexList[i];
        QString src = mGpsCmdStr.mid(cmd_start, cmdIndexList[i] - cmd_start);
        //qDebug()<<" src:"<<src<<" cmd_start:"<<cmd_start;
        cmd_start = cmdIndexList[i]+2;
        QRegExp exp("\\$([A-Z]{1,})");
        if(exp.indexIn(src) < 0) continue;
        QString cmd = exp.cap(1);
        if(cmd.length() == 0) continue;
        //qDebug()<<" cmd = "<<cmd;
        if(cmd == "GPGGA")
        {
            QStringList list = src.split(QRegExp("[,*]"));
            if(list.length() < 16) return ;
            if(mLastPosFlag)
            {
                mLastLat = mShipInfo.ship_lat();
                mLastLon = mShipInfo.ship_lon();
            }

            //进行校正GPS值算出船舶中心位置的经纬度  然后再加上修正值
            double old_lon = convertLonlatFromddmmdotmmmm(list[4],list[5]);
            double old_lat = convertLonlatFromddmmdotmmmm(list[2], list[3]);
            double new_lon = old_lon, new_lat = old_lat;
            if(mShipHeadAhead)
            {
                distbear_to_latlon(old_lat, old_lon, 0.5*mGpsShipLength, mShipInfo.ship_course() + 180, new_lat, new_lon);
            } else
            {
                distbear_to_latlon(old_lat, old_lon, 0.5*mGpsShipLength, mShipInfo.ship_course(), new_lat, new_lon);
            }
            new_lat += mGpsLatDeviation;
            new_lon += mGpsLonDeviation;

            mShipInfo.set_ship_update_time(convertTimeFromhhmmssdotsss(list[1]));
            mShipInfo.set_ship_lat(new_lat);
            mShipInfo.set_ship_lon(new_lon);
            //qDebug()<<"src:"<<list.length()<<"lon:"<<QString::number(mShipInfo.ship_lon(), 'f', 6)<<" lat:"<<QString::number(mShipInfo.ship_lat(), 'f', 6);
            if(mLastPosFlag && mCalCourseFlag)
            {
                //计算方位角
                double angle = calcAzimuth(mLastLon, mLastLat, mShipInfo.ship_lon(), mShipInfo.ship_lat());
                mShipInfo.set_ship_course(angle);
            }


            mLastPosFlag = true;
            //开始计算船舶速度
            if(mLastCalShipSpeedTime == 0)
            {
                mLastCalShipMoveLength = 0.0;
                mLastCalShipSpeedTime = time;
            } else
            {
                mLastCalShipMoveLength += getDistanceDeg(mLastSpeedLat, mLastSpeedLon, mShipInfo.ship_lat(), mShipInfo.ship_lon());
                qint64 subtime = time - mLastCalShipSpeedTime;
                if( subtime >= mSpeedTimeGap * 1000)
                {
                    double speed = mLastCalShipMoveLength * 1000 / subtime;
                    if(speed > 0.0)
                    {
                        mShipInfo.set_ship_speed_ground(speed);
                        mShipInfo.set_ship_speed(speed);
                    }
                    mLastCalShipMoveLength = 0.0;
                    mLastCalShipSpeedTime = time;
                    qDebug()<<"ship_speed:"<<speed<<" timegap:"<<mSpeedTimeGap<<" subtime:"<<subtime / 1000.0<<QDateTime::fromMSecsSinceEpoch(time);
                }
            }
            mLastSpeedLat = mShipInfo.ship_lat();
            mLastSpeedLon = mShipInfo.ship_lon();

        } else if(cmd == "GPVTG")
        {
            //地面速度信息
            //$GPVTG,(1),T,(2),M,(3),N,(4),K,(5)*hh(CR)(LF)
            QStringList list = src.split(QRegExp("[,*]"));
            if(list.length() < 10) return ;
            if(!mCalCourseFlag) mShipInfo.set_ship_course(list[1].toDouble());
            //mShipInfo.set_ship_speed_ground(list[7].toDouble() /3.6);
            //mShipInfo.set_ship_speed(list[7].toDouble() /3.6);
            mShipInfo.set_ship_update_time(time);
            //qDebug()<<"course:"<<mShipInfo.ship_course()<<" speed:"<<mShipInfo.ship_speed();
        } else if(cmd == "GPHDT")
        {
            //$GPHDT,123.456,T*00
            QStringList list = src.split(QRegExp("[,*]"));
            if(list.length() < 4) return ;
            mShipInfo.set_ship_head(list[1].toDouble());
            mShipInfo.set_ship_update_time(time);
        }
    }

    mShipInfo.set_ship_mode(2);
    int total = mGpsCmdStr.length();
    mGpsCmdStr = mGpsCmdStr.right(total - cmdIndexList.last() - 2);
    //qDebug()<<"gspcmd str:"<<mGpsCmdStr;
}

////$GPGGA,014434.70,3817.13334637,N,12139.72994196,E,4,07,1.5,6.571,M,8.942,M,0.7,0016*7B
//void CollectServer::slotGPSReciveComData(qint64 time, QByteArray data)
//{
//    //开始解析数据
//    QString src = QString::fromStdString(data.constData());
//    mGpsCmdStr += src;
//    //取得命令的名称
//    QRegExp exp("\\$([A-Z]{1,})");
//    //test
//    QList<int> cmdIndexList;
//    int index = 0;
//    while((index = exp.indexIn(mGpsCmdStr, index)) >= 0)
//    {
//        qDebug()<<"cmd:"<<exp.cap(1)<<index;
//        cmdIndexList.append(index);
//        index += exp.cap(1).length();
//    }
//    if
//    if(exp.indexIn(src) < 0) return;
//    QString cmd = exp.cap(1);
//    if(cmd.length() == 0) return;
//    if(cmd == "GPGGA")
//    {
//        QStringList list = src.split(QRegExp("[,*]"));
//        if(list.length() < 16) return ;
//        mShipInfo.set_ship_update_time(convertTimeFromhhmmssdotsss(list[1]));
//        mShipInfo.set_ship_lat(convertLonlatFromddmmdotmmmm(list[2], list[3]));
//        mShipInfo.set_ship_lon(convertLonlatFromddmmdotmmmm(list[4],list[5]));
//        qDebug()<<"src:"<<list.length()<<"lon:"<<mShipInfo.ship_lon()<<" lat:"<<mShipInfo.ship_lat();
//    } else if(cmd == "GPVTG")
//    {
//        //地面速度信息
//        //$GPVTG,(1),T,(2),M,(3),N,(4),K,(5)*hh(CR)(LF)
//        QStringList list = src.split(QRegExp("[,*]"));
//        if(list.length() < 10) return ;
//        mShipInfo.set_ship_course(list[1].toDouble());
//        mShipInfo.set_ship_speed_ground(list[7].toDouble() /3.6);
//        mShipInfo.set_ship_speed(list[7].toDouble() /3.6);
//        mShipInfo.set_ship_update_time(time);
//    } else if(cmd == "GPHDT")
//    {
//        //$GPHDT,123.456,T*00
//        QStringList list = src.split(QRegExp("[,*]"));
//        if(list.length() < 4) return ;
//        mShipInfo.set_ship_head(list[1].toDouble());
//        mShipInfo.set_ship_update_time(time);
//    }
//    mShipInfo.set_ship_mode(2);
//}

//***********************************************************************
//Description: HMR3000串口格式解码
//Inputs: string str 串口接收到的字符串,CR LF结尾
//Outputs:compass_info& info 解析串口字符串到compass_info结构体
//Sample: $PTNTHPR,85.9,N,-0.9,N,0.8,N*2C
//Note: 应答模式，需主动发送命令："$PTNT,HPR*78" <Cr><Lf>
/*
下面将叙述专用的句子：  HPR     Heading, Pitch, & Roll
航向，俯仰和横滚  $PTNTHPR,x.x,a,x.x,a,x.x,a*hh<cr><lf>
这个句子把HMR3000的三个重要的测量结果和有用的状态信息结合在一起
,数据依次代表：航向，磁场状态，俯仰，俯仰状态，横滚，横滚状态。
航向、俯仰和横滚的单位可以是度或mils,由在EEPROM中的设定来决定。
如果偏向角和磁偏角写入EEPROM中，航向的测量将会被修正。
例： 以角度的模式
$PTNTHPR,85.9,N,-0.9,N,0.8,N*2C $
PTNTHPR,7.4,N,4.2,N,2.0,N*33
$PTNTHPR,354.9,N,5.2,N,0.2,N*3A
以Mil的模式  $PTNTHPR,90,N,29,N,15,N*1C
状态信息的部分包括六个字母的指示：
L = low alarm,  （低级报警）
M = low warning,  （低级警告）
N = normal,  （正常）
O = high warning  （高级警告）
P = high alarm,  （高级报警）
C = tuning analog circuit  （调节模拟电路）
如果这三个状态指示中的任一个报警，航向部分将为空白，相应的磁场部分也如此。报警和警告的阈值可在 EEPROM中更改。
*/
//$PTNTHPR,85.9,N,-0.9,N,0.8,N*2C
bool CollectServer::parseHMR3000Data(const QString &src, HMR3000 &data)
{
    //取得命令的名称
    QRegExp exp("\\$([A-Z]{1,})");
    if(exp.indexIn(src) < 0) return false;
    QString cmd = exp.cap(1);
    if(cmd.length() == 0) return false;
    if(cmd != "PTNTHPR") return false;

    QStringList list = src.split(QRegExp("[,*]"));
    if(list.size() < 7) return false;
    data.head = list[1].toDouble();
    data.head_status = abs(getHMR3000Status(list[2]));
    data.pitch = list[3].toDouble();
    data.pitch_status = abs(getHMR3000Status(list[4]));
    data.roll = list[5].toDouble();
    data.roll_status = abs(getHMR3000Status(list[6]));
    return true;
}

int CollectServer::getHMR3000Status(const QString &str)
{
    if(str == "L") return -2;
    if(str == "M") return -1;
    if(str == "N") return 0;
    if(str == "O") return 1;
    if(str == "P") return 2;
    return 0;
}

void CollectServer::setSurface4017Params(const QMap<QString, DEVANALYPARAM> params)
{
    mSurfacePull1Param = params.value(PULL_4017_1);
    mSurfacePull2Param = params.value(PULL_4017_2);
    mSurfacePull3Param = params.value(PULL_4017_3);
    mSurfaceLeftPumpParam = params.value(LEFT_PUMP_4017);
    mSurfaceRightPumpParam = params.value(RIGHT_PUMP_4017);
}

void CollectServer::setUnderWater40172Params(const QMap<QString, DEVANALYPARAM> params)
{
    m40172Pull1Param = params.value(PULL_4017_1);
    m40172Pull2Param = params.value(PULL_4017_2);
    m40172Pull3Param = params.value(PULL_4017_3);
    m40172LeftPumpParam = params.value(LEFT_PUMP_4017);
    m40172RightPumpParam = params.value(RIGHT_PUMP_4017);
}

void CollectServer::setUnderWaterThresholdParam(const QMap<QString, DEVANALYPARAM> params)
{
    mUnderWaterMaxPitch = params.value(UNDER_WATER_MAX_PITCH).deviation;
    mUnderWaterMaxRoll = params.value(UNDER_WATER_MAX_ROLL).deviation;
    mUnderWaterMaxDepth = params.value(UNDER_WATER_MAX_DEPTH).deviation;
    mUnderWaterMinHeight = params.value(UNDER_WATER_MIN_HEIGHT).deviation;
    mUnderWaterMaxForce1 = params.value(UNDER_WATER_MAX_POWER1).deviation;
    mUnderWaterMaxForce2 = params.value(UNDER_WATER_MAX_POWER2).deviation;
    mUnderWaterMaxForce3 = params.value(UNDER_WATER_MAX_POWER3).deviation;
}
void CollectServer::setSurfaceThresholdParam(const QMap<QString, DEVANALYPARAM> params)
{
    mSurfaceMaxPitch = params.value(SURFACE_WATER_MAX_DEPTH).deviation;
    mSurfaceMaxRoll = params.value(SURFACE_WATER_MAX_ROLL).deviation;
    mSurfaceMaxDepth= params.value(SURFACE_WATER_MAX_DEPTH).deviation;
    mSurfaceMaxForce1 = params.value(SURFACE_WATER_MAX_POWER1).deviation;
    mSurfaceMaxForce2 = params.value(SURFACE_WATER_MAX_POWER2).deviation;
    mSurfaceMaxForce3 = params.value(SURFACE_WATER_MAX_POWER3).deviation;

}

void CollectServer::setTowBodyParam(const QMap<QString, DEVANALYPARAM> params)
{
    mTowForwardLength = params.value(FORWARD_LENGTH).deviation;
    mTowBackwardLength = params.value(BACKWARD_LENGTH).deviation;
    mTowBootsLength = params.value(BOOTS_LENGTH).deviation;
    mTowRange = params.value(TOW_BODY_RANGE).deviation;
    mTowBootsHeight = params.value(BOOTS_HEIGHT).deviation;
    mPlowInfo.set_plow_boots_length(mTowBootsLength);
}

void CollectServer::setOtherDevParam(const QMap<QString, DEVANALYPARAM> params)
{
    mReportStep = params.value(REPORT_STEP).deviation;
    mStorageTime = params.value(STORAGE_TIME).deviation;
    mGpsLonDeviation = params.value(GPS_LONOFFSET).deviation;
    mGpsLatDeviation = params.value(GPS_LATOFFSET).deviation;
    mGpsShipLength = params.value(GPS_SHIP_LENGTH).deviation;
    if(params.value(GPS_SHIP_AHEAD).deviation > 0)
    {
        mShipHeadAhead = true;
    } else
    {
        mShipHeadAhead = false;
    }
    mDisplayTime = params.value(DISPLAY_TIME).deviation;
    mSurfaceCompassDeviation = params.value(SURFACE_HMR_OFFSET).deviation;
    mMeterCounterStepMeter = params.value(METER_COUNTER_COEFF).deviation;
    mMeterCounterInitLength = params.value(METER_COUNTER_INIT).deviation;
    mUnderWaterCompassDeviation = params.value(UNDER_HMR_OFFSET).deviation;
    mLayInfo.set_meter_counter_coeff(mMeterCounterStepMeter);
    mLayInfo.set_meter_counter_initlen(mMeterCounterInitLength);
    mTensionCoeff = params.value(TENSION_COEFF).deviation;
    if(mTensionCoeff == 0.0)
    {
        mTensionCoeff = 1.0;
    }
    mSpeedTimeGap = params.value(SPEED_TIME_GAP).deviation;

    qDebug()<<__FUNCTION__<<"tension coeff"<<mTensionCoeff;
}

void CollectServer::setUnderWater40171Params(const QMap<QString, DEVANALYPARAM> params)
{
    m40171WaterDepthParam = params.value(DEPTH_4017);
    m40171BootsAngleParam = params.value(BOOTS_ANGLE_4017);
    m40171TouchDown1Param = params.value(TOUCHDOWN_4017_1);
    m40171TouchDown2Param = params.value(TOUCHDOWN_4017_2);
    m40171TouchDown3Param = params.value(TOUCHDOWN_4017_3);
    m40171TouchDown4Param = params.value(TOUCHDOWN_4017_4);
}

void CollectServer::slotRecvDpUploadMsg(const QByteArray &msg)
{
    //开启串口，读取
    ReciveComData *recv = mStartedComList.value(MSG_DP_UPLOAD_DEV, 0);
    if(recv)
    {
        recv->writeData(msg);
    }
}

QByteArray CollectServer::makeTensionCmd()
{
    QByteArray cmd;
    cmd.append(0x02);
    cmd.append(0x03);
    cmd.append(short2Byte(1));
    cmd.append(short2Byte(1));
    cmd.append(short2Byte(CRC16(cmd)));
    return QByteArray();
}




