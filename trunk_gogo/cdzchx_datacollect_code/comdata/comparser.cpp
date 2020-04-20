#include "comparser.h"
#include "profiles.h"
#include "Log.h"
#include <windows.h>
#include "globlefun/glogfunction.h"
#include "protobuf/protobufdataprocessor.h"
#include "common.h"
#include "comdata/comdefines.h"

const double M_2PREP30 =  0.000000000931322574615478515625;
const double M_2PREP15 =  0.000030517578125;
const double M_2PREP13 =  0.0001220703125;
#define GLOB_PI  (3.14159265358979323846)

ComParser::ComParser(QObject *parent) : QObject(parent)
{
    connect(this, SIGNAL(signalRecvComStatusChange(QString,bool)), this, SLOT(slotRecvComStatusChange(QString,bool)));
    connect(this, SIGNAL(signalRecvComData(QByteArray,int,QString,qint64,QByteArray)), \
            this, SLOT(slotRecvComData(QByteArray,int,QString,qint64,QByteArray)));
    this->moveToThread(&mWorkThread);
    mWorkThread.start();
}

ComParser::~ComParser()
{
}

void ComParser::slotRecvComStatusChange(const QString& type, bool sts)
{
    QMutexLocker locker(PROTOBUF_DATA->getMutex());
    PROTOBUF_DATA->devInfo().set_cur_utc_time(QDateTime::currentMSecsSinceEpoch());
    //解析数据，发送给接收到的客户端
    if(COM_ZS_DEV == type)
    {
        PROTOBUF_DATA->zs().set_sts(sts);
    } else if(COM_NHN_DEV == type)
    {
        PROTOBUF_DATA->nhn().set_sts(sts);
    } else if(COM_RDO_DEV == type)
    {
        PROTOBUF_DATA->rdo().set_sts(sts);
    } else if(COM_GPS_DEV == type)
    {
        PROTOBUF_DATA->gps().set_sts(sts);
    } else if(COM_ORP_DEV == type)
    {
        PROTOBUF_DATA->orp().set_sts(sts);
    } else if(COM_DDM_DEV == type)
    {
        PROTOBUF_DATA->ddm().set_sts(sts);
    } else if(COM_WATER_DEV == type)
    {
        PROTOBUF_DATA->wl().set_sts(sts);
    }
}

void ComParser::slotRecvComData(const QByteArray& head, int cmd_len,const QString& topic, qint64 time, const QByteArray& recv)
{
    QMutexLocker locker(PROTOBUF_DATA->getMutex());
    PROTOBUF_DATA->devInfo().set_cur_utc_time(time);
    //解析数据，发送给接收到的客户端
    if(COM_ZS_DEV == topic)
    {
        slotZSReciveComData(head, cmd_len, time, recv);
    } else if(COM_NHN_DEV == topic)
    {
        slotNHNReciveComData(head, cmd_len,time, recv);
    } else if(COM_RDO_DEV == topic)
    {
        slotRDOReciveComData(head, cmd_len,time, recv);
    } else if(COM_GPS_DEV == topic)
    {
        slotGPSReciveComData(head, cmd_len,time, recv);
    } else if(COM_ORP_DEV == topic)
    {
        slotORPReciveComData(head, cmd_len,time, recv);
    } else if(COM_DDM_DEV == topic)
    {
        slotDDMReciveComData(head, cmd_len,time, recv);
    } else if(COM_WATER_DEV == topic)
    {
        slotWLReciveComData(head, cmd_len,time, recv);
    }
    return;
}

double   ComParser::convertLonlatFromddmmdotmmmm(const QString &ValueStr, const QString &NSEWFlgStr)
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


qint64  ComParser::convertTimeFromhhmmssdotsss(const QString &valstr)
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

//$GPGGA,014434.70,3817.13334637,N,12139.72994196,E,4,07,1.5,6.571,M,8.942,M,0.7,0016*7B
void ComParser::slotGPSReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data)
{
    //qDebug()<<"开始解析GPS数据";
    Q_UNUSED(head);
    Q_UNUSED(cmd_len);
    SOCDS_DEBUG;
    //开始解析数据
    //PROTOBUF_DATA->gps().set_ship_id(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_ID").toString().toStdString().data());
    QString recv = QString::fromStdString(data.constData()).remove(QRegularExpression("[\\r\\n]"));
    mGpsCmdStr += recv;
    //取得命令的名称
    QRegExp exp_start("\\$([A-Z]{1,})");
    QList<int> cmdIndexList;
    int index = 0;
    while((index = exp_start.indexIn(mGpsCmdStr, index)) >= 0)
    {
        //qDebug()<<"cmd:"<<exp_end.cap()<<index;
        cmdIndexList.append(index);
        index += exp_start.cap().length();
    }
    if(cmdIndexList.length() == 0) return;
    //将命令字符串解析到命令列表
    QStringList cmdList;
    for(int i=0; i<cmdIndexList.length()-1; i++)
    {
        int k = i+1;
        cmdList.append(mGpsCmdStr.mid(cmdIndexList[i], cmdIndexList[k] - cmdIndexList[i]));
    }
    //添加最后一个命令
    int last = cmdIndexList.last();
    cmdList.append(mGpsCmdStr.right(mGpsCmdStr.length() - last));
    //开始命令处理
    mGpsCmdStr.clear();
    for(int i=0; i<cmdList.size(); i++)
    {
        QString src = cmdList[i];
        //检查是否是一个完整命令
         if(src.indexOf("*") == src.size() -3)
         {
             if(exp_start.indexIn(src) < 0) continue;
             QString cmd = exp_start.cap(1);
             if(cmd.length() == 0) continue;
             //qDebug()<<" cmd = "<<src<<" type:"<<cmd;
             if(cmd == "GPGGA")
             {
                 QStringList list = src.split(QRegExp("[,*]"));
                 if(list.length() < 16) continue ;
                 PROTOBUF_DATA->gps().set_ship_update_time(convertTimeFromhhmmssdotsss(list[1]));
                 PROTOBUF_DATA->gps().set_ship_lat(convertLonlatFromddmmdotmmmm(list[2], list[3]));
                 PROTOBUF_DATA->gps().set_ship_lon(convertLonlatFromddmmdotmmmm(list[4],list[5]));
             } else if(cmd == "GPVTG")
             {
                 //地面速度信息
                 //$GPVTG,(1),T,(2),M,(3),N,(4),K,(5)*hh(CR)(LF)
                 QStringList list = src.split(QRegExp("[,*]"));
                 if(list.length() < 10) continue ;
                 PROTOBUF_DATA->gps().set_ship_course(list[1].toDouble());
                 PROTOBUF_DATA->gps().set_ship_speed(list[7].toDouble() /3.6);
                 PROTOBUF_DATA->gps().set_ship_update_time(time);
             } else if(cmd == "GPHDT")
             {
                 //$GPHDT,123.456,T*00
                 QStringList list = src.split(QRegExp("[,*]"));
                 if(list.length() < 4) continue ;
                 PROTOBUF_DATA->gps().set_ship_head(list[1].toDouble());
                 PROTOBUF_DATA->gps().set_ship_update_time(time);
             }


         } else if(i == cmdList.size() - 1)
         {
             mGpsCmdStr.append(src);
         }
    }
    int total = mGpsCmdStr.length();
    mGpsCmdStr = mGpsCmdStr.right(total - cmdIndexList.last() - 2);
    SOCDS_DEBUG;
}

void ComParser::slotZSReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data)
{
    QList<double> list;
    if(parseRecvByteData(head, cmd_len,data, COM_ZS_DEV, list))
    {
        PROTOBUF_DATA->zs().set_time(time);
        PROTOBUF_DATA->zs().set_temp(list[1]);
        PROTOBUF_DATA->zs().set_zs(list[0]);
    }
}

void ComParser::slotNHNReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data)
{
    QList<double> list;
    if(parseRecvByteData(head, cmd_len,data, COM_NHN_DEV, list))
    {
        PROTOBUF_DATA->nhn().set_time(time);
        PROTOBUF_DATA->nhn().set_nhn(list[0]);
        PROTOBUF_DATA->nhn().set_temp(list[1]);
    }
}

void ComParser::slotRDOReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data)
{
    QList<double> list;
    if(parseRecvByteData(head, cmd_len,data, COM_RDO_DEV, list))
    {
        PROTOBUF_DATA->rdo().set_time(time);
        PROTOBUF_DATA->rdo().set_rdo(list[0]);
        PROTOBUF_DATA->rdo().set_temp(list[1]);
    }
}

void ComParser::slotDDMReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data)
{
    QList<double> list;
    if(parseRecvByteData(head, cmd_len,data, COM_DDM_DEV, list))
    {
        PROTOBUF_DATA->ddm().set_time(time);
        PROTOBUF_DATA->ddm().set_ddm(list[0]);
        PROTOBUF_DATA->ddm().set_temp(list[1]);
    }
}

void ComParser::slotORPReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data)
{
    QList<double> list;
    if(parseRecvByteData(head, cmd_len, data, COM_ORP_DEV, list))
    {
        PROTOBUF_DATA->orp().set_time(time);
        PROTOBUF_DATA->orp().set_orp(list[0]);
    }
}

void ComParser::slotWLReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data)
{
    QList<double> list;
    if(parseRecvByteData(head, cmd_len,data, COM_WATER_DEV, list))
    {
        PROTOBUF_DATA->wl().set_time(time);
        PROTOBUF_DATA->wl().set_press(list[0]);
        PROTOBUF_DATA->wl().set_temp(list[1]);
        PROTOBUF_DATA->wl().set_lvl(list[2]);
    }
}

bool ComParser::parseRecvByteData(const QByteArray& head, int cmd_len, const QByteArray &data, const QString& type, QList<double>& list)
{
    std::string srcStr = data.toHex().toUpper().toStdString();
    //解析错误响应
    if(data.size() < cmd_len) {
        LOG(LOG_RTM, "receive cmddata not enough(%d, %d),parse error occured:%s", data.size(), cmd_len, srcStr.data());
        return false;
    }
    //取得需要解析的数据
    //查找开始位置的地址码，功能码和有效数据长度码
    int index = -1;
    for(int i=0; i<data.size(); i++)
    {
        QByteArray checkBytes = data.mid(i, head.size());
        if(checkBytes.size() == head.size())
        {
            bool same = true;
            for(int k=0; k<head.size(); k++)
            {
                if(checkBytes[k]^head[k])
                {
                    same = false;
                    break;
                }
            }
            if(same)
            {
                index = i;
                break;
            }
        }
    }
    if(index == -1)
    {
        LOG(LOG_RTM, "head data not found yet(%s),parse error occured:%s", head.toHex().toUpper(), srcStr.data());
        return false;
    }
    //CRC校验检查
    QByteArray full_cmd_bytes = data.mid(index, cmd_len);
    if(full_cmd_bytes.size() != cmd_len)
    {
        LOG(LOG_RTM, "one full cmd not found yet(%s),parse error occured:%s", full_cmd_bytes.toHex().toUpper(), srcStr.data());
        return false;
    }
    QByteArray dataBytes = full_cmd_bytes.mid(0, cmd_len - 2);
    QByteArray crcBytes = full_cmd_bytes.right(2);
    short calcCrc = GlogFunction::instance()->CRCModbus16((uchar*)dataBytes.data(), dataBytes.size());
    short recvCrc = GlogFunction::instance()->bytes2Short(crcBytes, LOW_FIRST);;
//    if(type == COM_WATER_DEV)
//    {
//        recvCrc = GlogFunction::instance()->bytes2Short(crcBytes, HIGH_FIRST);
//    } else
//    {
//        recvCrc = GlogFunction::instance()->bytes2Short(crcBytes, LOW_FIRST);
//    }
    if(calcCrc != recvCrc)
    {
        LOG(LOG_RTM, "crc check error.calc_crc = %u, recv_crc = %u. data:%s ignored", calcCrc, recvCrc, srcStr.data());
        return false;
    }

    if(type == COM_ZS_DEV || type == COM_NHN_DEV || type == COM_RDO_DEV || type == COM_DDM_DEV)
    {
        list.append(GlogFunction::instance()->byte2ShortDouble(data.mid(3,4), HIGH_FIRST));
        list.append(GlogFunction::instance()->byte2ShortDouble(data.mid(7,4), HIGH_FIRST));
    } else if(type == COM_ORP_DEV)
    {
        list.append(GlogFunction::instance()->byte2ShortDouble(data.mid(3,4), HIGH_FIRST));
    } else if(type == COM_WATER_DEV)
    {
        list.append(GlogFunction::instance()->byte2Int(data.mid(3,4), HIGH_FIRST));
        list.append(GlogFunction::instance()->byte2ShortDouble(data.mid(7,2), HIGH_FIRST));
        list.append(GlogFunction::instance()->byte2Int(data.mid(9,4), HIGH_FIRST));
    }


    return true;
}




