#include "zchxRadarReportWorker.h"
#include "common.h"

zchxRadarReportWorker::zchxRadarReportWorker(RadarConfig* cfg, QThread* thread, QObject *parent) :
    QObject(parent),
    mSocket(Q_NULLPTR),
    mRadarCfg(cfg),
    mWorkThread(thread),
    mInit(false)
{
   init();
   if(mInit && mWorkThread)
   {
       this->moveToThread(mWorkThread);
   }
}

void zchxRadarReportWorker::init()
{
    if(!mRadarCfg) return;
    if(mRadarCfg->getReportIP().length() == 0 || mRadarCfg->getReportPort() == 0 || mRadarCfg->getReportOpen() == false) return;

    mSocket = new QUdpSocket();

    if(!mSocket->bind(QHostAddress::AnyIPv4, mRadarCfg->getReportPort(),QAbstractSocket::ShareAddress))
    {
        LOG_FUNC_DBG<<"bind report failed--";
        return;
    }

    mSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);//禁止本机接收
    if(!mSocket->joinMulticastGroup(QHostAddress(mRadarCfg->getReportIP())))
    {
         LOG_FUNC_DBG<<"joinMuticastGroup report failed--";
         return;
    }

    connect(mSocket, SIGNAL(readyRead()),this, SLOT(slotRecvReportData()));
    connect(mSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpReportError(QAbstractSocket::SocketError)));

    mInit = true;

    LOG_FUNC_DBG<<"init radar "<<mRadarCfg->getName() << " heart work succeed.";
    return;
}


void zchxRadarReportWorker::slotRecvReportData()
{
    if(!mSocket) return;
    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    datagram.resize(mSocket->pendingDatagramSize());//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    mSocket->readDatagram(datagram.data(), datagram.size());//readDatagram将不大于指定长度的数据保存到datagram.data()
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar report data,size = %1").arg(datagram.size());
    emit signalSendRecvedContent(utc,"REPORT_RECEIVE",sContent);
//    cout<<"接收到雷达状态数据啦"<<datagram;
//    cout<<"----------------";
    ProcessReport(datagram, datagram.size());
}

void zchxRadarReportWorker::displayUdpReportError(QAbstractSocket::SocketError e)
{
    LOG_FUNC_DBG<<mSocket->errorString()<<e;
}

void zchxRadarReportWorker::processReport(const QByteArray& bytes, size_t len)
{
    //cout<<"状态-------------"<<QString::fromStdString(bytes.toHex().data());

    emit signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "RadarRpoert", QString::fromStdString(bytes.toHex().data()));
    if(len < 3 ) return;
    unsigned char val = bytes[1];
    //cout<<val;
    if (val == 0xC4)
    {
        //cout<<"解析雷达数据-------------------------------------------";
        switch ((len << 8) + bytes[0])
        {
        case (18 << 8) + 0x01:
        {
            RadarReport_01C4_18 *s = (RadarReport_01C4_18 *)bytes.data();
            if (mRadarPowerStatus != s->radar_status)
            {
                mRadarPowerStatus = s->radar_status;
                switch (bytes[2])
                {
                case 0x01:
                    cout<<"待机状态";
                    //ZCHXLOG_DEBUG("reports status RADAR_STANDBY");
                    updateValue(INFOTYPE::POWER,0);
                    break;
                case 0x02:
                    cout<<"传输状态";
                    //ZCHXLOG_DEBUG("reports status TRANSMIT");
                    updateValue(INFOTYPE::POWER,1);
                    break;
                case 0x05:
                    cout<<"唤醒状态";
                    //ZCHXLOG_DEBUG("reports status WAKING UP");
                    break;
                default:
                    break;
                }
            }
            break;
        }
        case (99 << 8) + 0x02: // length 99, 02 C4,contains gain,rain,interference rejection,sea
            //target_boost, target_expansion,range
        {
            //cout<<"进来了_2 02，C4，包含增益，雨，干扰抑制，海洋,target_boost, target_expansion,range";
            RadarReport_02C4_99 *s = (RadarReport_02C4_99 *)bytes.data();
            //gain
            if (s->field8 == 1)        // 1 for auto
                updateValue(INFOTYPE::GAIN,-1);
            else
                updateValue(INFOTYPE::GAIN, s->gain * 100 / 255);
            //sea
            if (s->field13 == 0x01)
                updateValue(INFOTYPE::SEA_CLUTTER,-1);  // auto sea
            else
                updateValue(INFOTYPE::SEA_CLUTTER,s->sea * 100 / 255);
            //rain
            updateValue(INFOTYPE::RAIN_CLUTTER, s->rain * 100 / 255);
            //target boost
            updateValue(INFOTYPE::TARGET_BOOST, s->target_boost);
            //s->interference rejection
            updateValue(INFOTYPE::INTERFERENCE_REJECTION, s->interference_rejection);
            //target expansion
            updateValue(INFOTYPE::TARGET_EXPANSION, s->target_expansion);
            //range
            updateValue(INFOTYPE::RANG, s->range / 10);
            break;
        }
        case (129 << 8) + 0x03: // 129 bytes starting with 03 C4
        {
            //cout<<"进来了_3 129 bytes starting with 03 C4";
            RadarReport_03C4_129 *s = (RadarReport_03C4_129 *)bytes.data();
            switch (s->radar_type) {
            case 0x0f:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_BR24");
                break;
            case 0x08:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_3G");
                break;
            case 0x01:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_4G");
                break;
            default:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: " << s->radar_type);
                return ;
            }
            break;
        }
        case (66 << 8) + 0x04: // 66 bytes starting with 04 C4,contains bearing alignment,antenna height
        {
            //cout<<"进来了_4 从04 C4开始的66个字节，包含轴承对齐，天线高度";
            RadarReport_04C4_66 *data = (RadarReport_04C4_66 *)bytes.data();
            // bearing alignment
            int ba = (int)data->bearing_alignment / 10;
            if (ba > 180) ba = ba - 360;
            updateValue(INFOTYPE::BEARING_ALIGNMENT, ba);
            // antenna height
            updateValue(INFOTYPE::ANTENNA_HEIGHT, data->antenna_height / 1000);
            break;
        }
        case (564 << 8) + 0x05:
        {
            //cout<<"进来了_5 内容未知，但我们知道BR24雷达发送这个";
            // Content unknown, but we know that BR24 radomes send this
            //ZCHXLOG_DEBUG("Navico BR24: msg");
            break;
        }
        case (18 << 8) + 0x08: // length 18, 08 C4,
            //contains scan speed, noise rejection and target_separation and sidelobe suppression,local_interference_rejection
        {
            //cout<<"进来了_6 包含扫描速度，噪声抑制和目标分离和侧面抑制，局部干扰抑制";
            RadarReport_08C4_18 *s08 = (RadarReport_08C4_18 *)bytes.data();
            updateValue(INFOTYPE::SCAN_SPEED, s08->scan_speed);
            updateValue(INFOTYPE::NOISE_REJECTION, s08->noise_rejection);
            updateValue(INFOTYPE::TARGET_SEPARATION, s08->target_sep);
            if (s08->sls_auto == 1)
                updateValue(INFOTYPE::SIDE_LOBE_SUPPRESSION,-1);
            else
                updateValue(INFOTYPE::SIDE_LOBE_SUPPRESSION, s08->side_lobe_suppression * 100 / 255);
            updateValue(INFOTYPE::LOCAL_INTERFERENCE_REJECTION, s08->local_interference_rejection);
            break;
        }
        default:
            break;
        }
    }
    else if(bytes[1] == 0xF5){
        cout<<"不知道是什么东西";
        //ZCHXLOG_DEBUG("unknown: buf[1]=0xF5");
    }
    return ;
}

void ZCHXRadarDataServer::updateValue(INFOTYPE controlType, int value)
{
    //检查值的范围
    if(controlType <= INFOTYPE::UNKNOWN ||  controlType > INFOTYPE::RESVERED)
    {
        return;
    }
    //cout<<"mRadarStatusMap容器大小--------------"<<mRadarStatusMap.size();
    if(!mRadarStatusMap.contains(controlType))
    {
        mRadarStatusMap[controlType] = RadarStatus(controlType);
    }
    RadarStatus &sts = mRadarStatusMap[controlType];
    //cout<<"sts.value"<<sts.value<<"value"<<value;
    if(sts.value != value)
    {

        sts.value = value;
        emit signalRadarStatusChanged(mRadarStatusMap.values(), m_uSourceID);
    }

}


