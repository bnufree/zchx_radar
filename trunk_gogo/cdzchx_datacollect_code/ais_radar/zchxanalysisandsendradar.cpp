#include "zchxanalysisandsendradar.h"
#include "BR24.hpp"
//#include <QDebug>
#include <QtCore/qmath.h>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
//#include <zlib.h>
#include <QDir>
#include <QBuffer>
#include "../profiles.h"
#include "Log.h"
#include "dataout/zchxdataoutputservermgr.h"

#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

static uint8_t BR24MARK[] = {0x00, 0x44, 0x0d, 0x0e};
int g_lastRange = 0;

#define         PROCESS_CYCLE_DATA
#define         DOPPLER_USED

#ifndef NAVICO_SPOKES
#define NAVICO_SPOKES 2048
#endif

#ifndef NAVICO_SPOKE_LEN
#define NAVICO_SPOKE_LEN 1024
#endif

#if SPOKES_MAX < NAVICO_SPOKES
#undef SPOKES_MAX
#define SPOKES_MAX NAVICO_SPOKES
#endif
#if SPOKE_LEN_MAX < NAVICO_SPOKE_LEN
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX NAVICO_SPOKE_LEN
#endif


enum LookupSpokeEnum {
  LOOKUP_SPOKE_LOW_NORMAL,
  LOOKUP_SPOKE_LOW_BOTH,
  LOOKUP_SPOKE_LOW_APPROACHING,
  LOOKUP_SPOKE_HIGH_NORMAL,
  LOOKUP_SPOKE_HIGH_BOTH,
  LOOKUP_SPOKE_HIGH_APPROACHING
};

static uint8_t lookupData[6][256];

void ZCHXAnalysisAndSendRadar::InitializeLookupData()
{
  if (lookupData[5][255] == 0) {
    for (int j = 0; j <= UINT8_MAX; j++) {
      uint8_t low = (j & 0x0f) << 4;
      uint8_t high = (j & 0xf0);

      lookupData[LOOKUP_SPOKE_LOW_NORMAL][j] = low;
      lookupData[LOOKUP_SPOKE_HIGH_NORMAL][j] = high;

      switch (low) {
        case 0xf0:
          lookupData[LOOKUP_SPOKE_LOW_BOTH][j] = 0xff;
          lookupData[LOOKUP_SPOKE_LOW_APPROACHING][j] = 0xff;
          break;

        case 0xe0:
          lookupData[LOOKUP_SPOKE_LOW_BOTH][j] = 0xfe;
          lookupData[LOOKUP_SPOKE_LOW_APPROACHING][j] = low;
          break;

        default:
          lookupData[LOOKUP_SPOKE_LOW_BOTH][j] = low;
          lookupData[LOOKUP_SPOKE_LOW_APPROACHING][j] = low;
      }

      switch (high) {
        case 0xf0:
          lookupData[LOOKUP_SPOKE_HIGH_BOTH][j] = 0xff;
          lookupData[LOOKUP_SPOKE_HIGH_APPROACHING][j] = 0xff;
          break;

        case 0xe0:
          lookupData[LOOKUP_SPOKE_HIGH_BOTH][j] = 0xfe;
          lookupData[LOOKUP_SPOKE_HIGH_APPROACHING][j] = high;
          break;

        default:
          lookupData[LOOKUP_SPOKE_HIGH_BOTH][j] = high;
          lookupData[LOOKUP_SPOKE_HIGH_APPROACHING][j] = high;
      }
    }

//    {
//        for(int i=0; i<6; i++)
//        {
//            QList<uint8_t> data;
//            for (int j = 0; j <= UINT8_MAX; j++) {
//                data.append(lookupData[i][j]);
//            }
//            qDebug()<<"lookupData["<<i<<"]:"<<data;
//        }
//    }
  }
}

ZCHXAnalysisAndSendRadar::ZCHXAnalysisAndSendRadar(int id, QObject *parent)
    : QObject(parent),
      mRangeFactor(13.0),
      m_uSourceID(id),
      mStartAzimuth(-1),
      mRadarOutMgr(0),
      mRadarType(RADAR_UNKNOWN),
      mUseNativeRadius(false),
      mDopplerVal(0)
{
    qRegisterMetaType<QMap< int,QList<QPointF> >>("QMap< int,QList<QPointF> >");
    qRegisterMetaType<zchxRadarRectList>("const zchxRadarRectList&");
    qRegisterMetaType<zchxRadarRectDefList>("const zchxRadarRectDefList&");
    qRegisterMetaType<zchxRadarRectMap>("const zchxRadarRectMap&");
    qRegisterMetaType<QMap<int,QList<TrackNode>>>("QMap<int,QList<TrackNode>>");
    qRegisterMetaType<zchxRadarRouteNodes>("const zchxRadarRouteNodes&");

    InitializeLookupData();


    m_ri.broken_packets = 0;
    m_ri.broken_spokes = 0;
    m_ri.missing_spokes = 0;
    m_ri.packets = 0;
    m_ri.spokes = 0;
    str_radar = QString("Radar_%1").arg(m_uSourceID);

    //设定预推的默认参数
    Utils::Profiles::instance()->setDefault(str_radar,"PushTimes", 6);
    Utils::Profiles::instance()->setDefault(str_radar,"PushInterval", 5);
    sRadarType = Utils::Profiles::instance()->value(str_radar,"Radar_Type").toString();
    m_dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble();
    m_uLoopNum = Utils::Profiles::instance()->value(str_radar,"Loop_Num").toInt();
    mRadarVideoPort = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toInt();
    mRadarVideoTopic = Utils::Profiles::instance()->value(str_radar,"video_Topic").toString();
    mRadarTrackPort = Utils::Profiles::instance()->value(str_radar,"Track_Send_Port").toInt();
    mRadarTrackTopic = Utils::Profiles::instance()->value(str_radar,"Track_Topic").toString();
    cell_num = Utils::Profiles::instance()->value(str_radar,"Cell_Num").toInt();
    double merge_dis = Utils::Profiles::instance()->value(str_radar,"Radius").toInt();
    int clear_track_time = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();
    radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toInt();//雷达编号
    hIniNum = Utils::Profiles::instance()->value(str_radar,"historyNum").toDouble(); //自定义保存多少笔
    double dir_invert = Utils::Profiles::instance()->value(str_radar,"Direction_Invert").toDouble();
    bool cog_adjust = Utils::Profiles::instance()->value(str_radar,"azimuth_adjustment").toBool();

    mRadarRectPort =  Utils::Profiles::instance()->value(str_radar,"Yuhui_Send_Port").toInt();
    mRadarRectTopic = Utils::Profiles::instance()->value(str_radar,"Yuhui_Topic").toString();
    mUseNativeRadius = Utils::Profiles::instance()->value(str_radar, "native_radius", false).toBool();


    //回波块解析
    bool process_sync = true;
    m_VideoProcessor =  new ZCHXRadarVideoProcessor(m_uSourceID, this);
    double prediction_width = Utils::Profiles::instance()->value(str_radar, "prediction_width", 20).toDouble();

    m_targetTrack = new zchxRadarTargetTrack(radar_num, Latlon(m_dCentreLat, m_dCentreLon), clear_track_time, prediction_width, true, this);
    m_targetTrack->setAdjustCogEnabled(cog_adjust);
    m_targetTrack->setTargetMergeDis(merge_dis);
    m_targetTrack->setDirectionInvertVal(dir_invert);

    connect(m_VideoProcessor,SIGNAL(signalSendVideoPixmap(QPixmap)), this,SLOT(slotRecvVideoImg(QPixmap)));
    //回波颜色设置
    connect(this, SIGNAL(colorSetSignal(int,int,int,int,int,int)),
            m_VideoProcessor,SLOT(slotSetColor(int,int,int,int,int,int)));
    //处理矩形回波块    
    connect(m_targetTrack, SIGNAL(signalSendTracks(zchxRadarSurfaceTrack)),
            this, SLOT(slotSendComTracks(zchxRadarSurfaceTrack)));
    connect(m_targetTrack, SIGNAL(signalSendRectData(zchxRadarRectMap)),
            this, SLOT(sendRadarRectPixmap(zchxRadarRectMap)));
    connect(m_targetTrack, SIGNAL(signalSendRoutePath(zchxRadarRouteNodes)),
            this, SLOT(sendRadarNodeRoute(zchxRadarRouteNodes)));
    m_VideoProcessor->start();
    if(process_sync)
    {
        m_VideoProcessor->setTracker(m_targetTrack);
    } else
    {
        connect(m_VideoProcessor,SIGNAL(signalSendRects(zchxRadarRectDefList)),
                m_targetTrack,SLOT(appendTask(zchxRadarRectDefList)));
        m_targetTrack->start();
    }


    fMap.clear();//浮标模块
    for(int i =0; PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).isNull() != 1; i++)
    {
        QStringList mStrList = PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).toStringList();
        fMap[i] = mStrList;
    }
    floatRange = PROFILES_INSTANCE->value("FloatSet","range").toInt();
    cout<<"更新浮标配置-fMap"<<fMap.size()<<floatRange;


    //发送雷达目标的zmq
    mRadarOutMgr = new zchxRadarDataOutputMgr(this);


    connect(this, SIGNAL(analysisLowranceRadarSignal(QByteArray,int,int,int)),
            this, SLOT(analysisLowranceRadarSlot(QByteArray,int,int,int)));
    connect(this, SIGNAL(analysisCatRadarSignal(QByteArray,int,int,int,QString)),
            this, SLOT(analysisCatRadarSlot(QByteArray,int,int,int,QString)));
    connect(this,SIGNAL(showTrackNumSignal(bool)),this,SLOT(showTrackNumSlot(bool)));
    m_workThread.setStackSize(64000000);
    moveToThread(&m_workThread);
    m_workThread.start();

    m_pTimer = new QTimer();
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    m_pTimer->start(2000);
    m_pTimer_1 = new QTimer();
    connect(m_pTimer_1, SIGNAL(timeout()), this, SLOT(handleTimeout_1()));
    m_pTimer_1->start(100);

    QTimer::singleShot(1000, this, SLOT(slotReadLimitData()));

}

void ZCHXAnalysisAndSendRadar::slotReadLimitData()
{
    bool limit_enable = Utils::Profiles::instance()->value(str_radar,"Limit").toBool();
    if(limit_enable)
    {
        QString file = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();
        if(file.trimmed().isEmpty()) return;
        QFile fp(file);
        if(fp.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            if(mRadarOutMgr) mRadarOutMgr->appendLimitData(fp.readAll());
            fp.close();
        } else
        {
            qDebug()<<"open limit file failed....";
        }

    }
}

ZCHXAnalysisAndSendRadar::~ZCHXAnalysisAndSendRadar()
{
      cout<<"~ZCHXAnalysisAndSendRadar()0";
      if(m_pTimer)
      {
          m_pTimer->stop();
          m_pTimer->deleteLater();
      }
      if(m_pTimer_1)
      {
          m_pTimer_1->stop();
          m_pTimer_1->deleteLater();
      }
      if(m_workThread.isRunning())
      {
          m_workThread.quit();
      }
      m_workThread.terminate();

      if(m_VideoProcessor)
      {
          delete m_VideoProcessor;
          m_VideoProcessor = NULL;
      }
      if(m_targetTrack)
      {
          delete m_targetTrack;
          m_targetTrack = 0;
      }

      if(mRadarOutMgr) delete mRadarOutMgr;
}


void ZCHXAnalysisAndSendRadar::handleTimeout()//1_超时处理函数,总是重置计数器，所以在显示变化之后它们不会显示大量的数字
{
    m_ri.broken_packets = 0;
    m_ri.broken_spokes = 0;
    m_ri.missing_spokes = 0;
    m_ri.packets = 0;
    m_ri.spokes = 0;
}

void ZCHXAnalysisAndSendRadar::handleTimeout_1()//300ms发送一次数据包信息
{
    emit show_statistics(m_ri.packets,m_ri.broken_packets,m_ri.spokes,m_ri.broken_spokes,m_ri.missing_spokes);
}

//解析小雷达目标
//void ZCHXAnalysisAndSendRadar::analysisLowranceRadarSlot(const QByteArray &sRadarData, int uLineNum, int uCellNum, int uHeading)
//{
//    if(mRadarType == RADAR_UNKNOWN) return;
//    static QTime time_stas;
//    quint64 num = QDateTime::currentMSecsSinceEpoch();
////    if(finishiProcess == 0)
////    {
////        cout<<"deal not finishi !";
////        return;
////    }
//    if(upRad == 0)
//        upRad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
//    const char *buf = sRadarData.constData();
//    int len = sRadarData.size();
////    qDebug()<<"analysisLowranceRadarSlot len:"<<len;
//    BR24::Constants::radar_frame_pkt *packet = (BR24::Constants::radar_frame_pkt *)buf;//正常大小是17160

//    //cout<<sizeof(BR24::Constants::radar_frame_pkt);//结构体大小是固定的64328 定的120个spoke(536*120)+frame_hdr(8)
//    //qDebug()<<" packet len:"<<(int)sizeof(packet->frame_hdr);

//    m_ri.packets++;//1_一共收到的包
//    if (len < (int)sizeof(packet->frame_hdr)) {
//        // The packet is so small it contains no scan_lines, quit!
//        qDebug()<<"此包长度不吻合，丢包！";
//        m_ri.broken_packets++;//1_不完整的包
//        return;
//    }
//    //cout<<len<<sizeof(packet->frame_hdr)<<sizeof(BR24::Constants::radar_line);//第 178 行 (17160 - 8) / 536 = 32
//    int scanlines_in_packet = (len - sizeof(packet->frame_hdr)) / sizeof(BR24::Constants::radar_line);
//    if (scanlines_in_packet != 32) {
//        cout<<"此包没有32条扫描线，丢包！";
//        m_ri.broken_packets++;//1_不完整的包
//        return;
//    }

//    //qDebug()<<"scanlines_in_packet:"<<scanlines_in_packet;
//    //int systemAreaCode = config.SAC ;
//    //int systemIdentificationCode = config.SIC;

//    QDateTime curDateTime = QDateTime::currentDateTime();
//    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
//                                  curDateTime.date().day()),QTime(0, 0));
//    int time_of_day = startDateTime.secsTo(curDateTime);

//    QList<int> AmplitudeList;
//    QList<int> pIndexList;
////    struct SAzmData sAzmData;
//    //std::list<TrackInfo> trackList;
//    zchxVideoFrameList frameList;
//    double range_factor;
//    for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
//        QDateTime curDateTime_1 = QDateTime::currentDateTime();
//        QDateTime startDateTime_1(QDate(curDateTime_1.date().year(),curDateTime_1.date().month(),
//                                        curDateTime_1.date().day()),QTime(0, 0));
//        int time_of_day_1 = startDateTime_1.secsTo(curDateTime_1);//当前时间

//        BR24::Constants::radar_line *line = &packet->line[scanline];

//        // Validate the spoke
//        int spoke = line->common.scan_number[0] | (line->common.scan_number[1] << 8);

//        m_ri.spokes++;
//        //cout<<spoke;

//        if (line->common.headerLen != 0x18) {
//            qDebug()<<"strange header length:" << line->common.headerLen;
//            // Do not draw something with this...
//            qDebug()<<"该"<< scanline << "扫描线头长度不是24字节，丢包！";
//            m_ri.missing_spokes++;//1_没有扫描线
//            m_next_spoke = (spoke + 1) % SPOKES;

//            cout<<"打印missing_spokes";
//            continue;
//        }

//        //probably status: 02 valid data; 18 spin up
//        if (line->common.status != 0x02 && line->common.status != 0x12) {
//            qDebug()<<"strange status:" << line->common.status;
//            qDebug()<<"该"<< scanline << "扫描线状态不对，无效！";
//            m_ri.broken_spokes++;//1_不完整扫描线
//        }
//        if (m_next_spoke >= 0 && spoke != m_next_spoke) {
//            if (spoke > m_next_spoke) {
//                m_ri.missing_spokes += spoke - m_next_spoke;

//            } else {
//                m_ri.missing_spokes += SPOKES + spoke - m_next_spoke;
//                cout << "m_ri.missing_spokes_2:"<<m_ri.missing_spokes;
//                cout<<"打印missing_spokes";
//            }
//            qDebug()<<"error spoke:"<<spoke<<m_next_spoke<<m_uSourceID;
//        }
//        m_next_spoke = (spoke + 1) % SPOKES;
//        //qDebug()<<"current spoke:"<<spoke<<" next:"<<m_next_spoke;
//        int range_raw = 0;
//        int angle_raw = 0;
//        short int heading_raw = 0;
//        double range_meters = 0;

//        heading_raw = (line->common.heading[1] << 8) | line->common.heading[0];

//        if (memcmp(line->br24.mark, BR24MARK, sizeof(BR24MARK)) == 0) {
//            // BR24 and 3G mode
//            range_raw = ((line->br24.range[3] & 0xff) << 24 | (line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
//            angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
//            range_meters = (int)((double)range_raw * 10.0 / sqrt(2.0));
//            cout<<"半径_1 range_meters"<<range_meters;
//            //range_meters = 7249.92;//1_写死距离因子,和半径

//            qDebug()<<"br24";

//            // test code:
//            if (g_lastRange != range_meters) {
//                LOG(LOG_RTM, "%s %s %d, BR24 and 3G mode: range_raw = %d, range_meters = %f",
//                    __FILE__, __FUNCTION__,__LINE__,
//                    range_raw, range_meters);

//                g_lastRange = range_meters;
//            }
//        } else {
//            {
//                uint16_t large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
//                uint16_t small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
//                int angle_raw_test = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
//                if (large_range == 0x80) {
//                    if (small_range == 0xffff) {
//                        range_meters = 0;  // Invalid range received
//                    } else {
//                        range_meters = small_range / 4;
//                    }
//                } else {
//                    range_meters = large_range * small_range / 512;
//                }
//            }
//            //cout<<"br4g";
//            // 4G mode
//            short int large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
//            short int small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
//            //cout<<"large_range=" << (line->br4g.largerange[1] << 8) <<(line->br4g.largerange[0]) ;
//            //cout<<"small_range=" << (line->br4g.smallrange[1] << 8) <<(line->br4g.smallrange[0]);
//            angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
//            if (large_range == 0x80 && (line->br4g.smallrange[1] << 8)<32768) {
//            //if (large_range == 0x80  ) {
//                if (small_range == -1) {
//                    range_raw = 0;  // Invalid range received
//                } else {
//                    //cout<<"small_range"<<small_range<<"large_range"<<large_range;
//                    range_raw = small_range;
//                    range_meters = range_raw / 4;
//                    //if(range_meters < 2600) range_meters =2600;
//                }
//            }
//            else {
//                if(sRadarType == "6G")
//                {
//                    //6G
//                    int Rad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
//                    range_meters = (Rad*7.0312)/4;
//                    singalShowRadiusCoefficient(range_meters,1.7578);
//                    double k = Utils::Profiles::instance()->value(str_radar,"RadiusCoefficient").toDouble();
//                    if(k <= 0)
//                    {
//                        k = 1.7578;
//                    }
//                    range_meters = Rad * k;
//                }
//                else
//                {
//                    //4G
//                    //cout<<"small_range"<<small_range<<"large_range"<<large_range;
//                    /*range_raw = large_range * 256;
//                    range_meters = range_raw / 4;*/
//                    int Rad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
//                    range_meters = Rad*(1.3847);
//                    singalShowRadiusCoefficient(range_meters,1.3847);
//                    double k = Utils::Profiles::instance()->value(str_radar,"RadiusCoefficient").toDouble();
//                    if(k <= 0)
//                    {
//                        k = 1.3847;
//                    }
//                    range_meters = Rad * k;
//                }
//            }
//            //只有通道一打印
//            int nowRad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt() ;
//            if( upRad != nowRad && m_uSourceID == 1)
//            {
//                cout<<"打印2"<<upRad<<nowRad<<m_uSourceID;
//                upRad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
//                LOG(LOG_RTM, "上传半径:%d,  small_range:%d, large_range:%d, 返回半径:= %f", upRad, small_range,large_range,range_meters);
//            }

//        }
//        int azimuth_cell = uLineNum;
//        bool radar_heading_valid = (((heading_raw) & ~(HEADING_TRUE_FLAG | (azimuth_cell - 1))) == 0);
//        double heading;
//        if (radar_heading_valid) {
//            double heading_rotation = (((heading_raw) + 2 * azimuth_cell) % azimuth_cell);
//            double heading_degrees = ((heading_rotation) * (double)DEGREES_PER_ROTATION / azimuth_cell);
//            heading = (fmod(heading_degrees + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION));
//        } else {
//            //ZCHXLOG_DEBUG("radar_heading_valid=" << radar_heading_valid );
//        }
//        //角度值强制变成偶数
//        int origin_angle_raw = angle_raw;
//        if(angle_raw % 2)
//        {
//            angle_raw += 1;
//        }
//        if(angle_raw == 4096) angle_raw = 0;
//        qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw<<(line->br24.angle[1] << 8)<< line->br24.angle[0]<<" origion:"<<origin_angle_raw;
//        angle_raw = MOD_ROTATION2048(angle_raw / 2);  //让方向和一圈的扫描线个数保持一致(2048)
//        //qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw;
//        double start_range = 0.0 ;
//        range_factor = range_meters/uCellNum;

//        //qDebug()<<"range_meter:"<<range_meters<<" cellNum:"<<uCellNum<<" range_factor"<<range_factor;

//        AmplitudeList.clear();
//        pIndexList.clear();
//        //double dAzimuth = angle_raw * (360.0 / (uLineNum / 2)) + uHeading;
//        //cout<<"dAzimuth:"<<dAzimuth<<"angle_raw"<<angle_raw<<"uHeading"<<uHeading; //1_扫描方位,angle_raw(0-2047),uHeading(180)

//        //赋值振幅
//        for (int range = 0; range < uCellNum; range++) {
//            int value = (int)(line->data[range]);
////            sAzmData.iRawData[range] = 0;
//            if (value > 0) {
//                AmplitudeList.append(value);
//                pIndexList.append(range);
//            }
//        }
//        //cout<<"spoke:"<<spoke<<" angle:"<<angle_raw<<heading_raw;
//        //检查是否是经过了一次扫描周期,如果是,发出数据开始解析
//        if(mStartAzimuth >= 0 && mStartAzimuth == angle_raw)
//        {
//            qDebug()<<"video data elapsed:"<<time_stas.elapsed();
//            //一个扫描周期完成
////            cout<<"一个扫描周期完成angle_raw"<<angle_raw<<" data size:"<<m_radarVideoMap.size();
////            qDebug()<<"cycle data end:"<<QDateTime::currentDateTime().toString("hh:mm:ss zzz");
////            processVideoData(true);
//            m_radarVideoMap.clear();
//            mStartAzimuth = -1;
//        }

//        //cout<<"angle_raw"<<angle_raw;
//        RADAR_VIDEO_DATA objVideoData;
//        radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toInt();//雷达编号
//        objVideoData.m_uSourceID = m_uSourceID; //1 接入雷达编号
//        objVideoData.m_uSystemAreaCode = 1; //系统区域代码
//        objVideoData.m_uSystemIdentificationCode = 1; //系统识别代码
//        objVideoData.m_uMsgIndex = spoke; //消息唯一序列号
//        objVideoData.m_uAzimuth = angle_raw; //扫描方位
//        objVideoData.m_dStartRange = start_range;//扫描起始距离
//        objVideoData.m_dRangeFactor = range_factor;//1_距离因子
//        objVideoData.m_uTotalNum = uCellNum;//1_一条线上有多少个点
//        objVideoData.m_dCentreLon = m_dCentreLon; //中心经度
//        objVideoData.m_dCentreLat = m_dCentreLat; //中心纬度
//        objVideoData.m_uLineNum = uLineNum/2; //1_总共线的个数
//        objVideoData.m_uHeading = uHeading;//雷达方位

//        objVideoData.m_pAmplitude = AmplitudeList;
//        objVideoData.m_pIndex = pIndexList;
//        objVideoData.m_timeofDay = time_of_day_1;
//        //半径
//        m_dRadius = range_meters;
//        //m_dDiameter = m_dRadius*2;
////        qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw<<(line->br24.angle[1] << 8)<< line->br24.angle[0]<<m_dRadius;
////        qDebug()<<"radius:"<<m_dRadius;
//        //有可能多次扫描线有同一个角度,这里可能将角度的值进行合并,先暂且不处理,直接覆盖
//#if 0
//        int key = objVideoData.m_uAzimuth % 2048;
//        if(m_radarVideoMap.contains(key))
//        {
//            //将两个的振幅值进行合并
//            RADAR_VIDEO_DATA &old_data = m_radarVideoMap[key];
//            for(int i=0; i<pIndexList.size(); i++)
//            {
//                int index = old_data.m_pIndex.indexOf(pIndexList[i]);
//                if(index >= 0)
//                {
//                    //已经有值
//                    if(old_data.m_pAmplitude[index] < AmplitudeList[i])
//                    {
//                        old_data.m_pAmplitude[index] = AmplitudeList[i];
//                    }
//                } else
//                {
//                    //值不存在
//                    old_data.m_pIndex.append(pIndexList[i]);
//                    old_data.m_pAmplitude.append(AmplitudeList[i]);
//                }

//            }
//        } else
//        {
//            m_radarVideoMap[key] = objVideoData;
//        }
//#endif

//        m_radarVideoMap[objVideoData.m_uMsgIndex % 2048] = objVideoData;
//#if 0
//        if(start_azimuth == -1)
//        {
//            start_azimuth = angle_raw;
//           //cout<<"angle_raw"<<angle_raw;
//        }
//        else if(start_azimuth == angle_raw)
//        {
//            //一个扫描周期完成
//            //cout<<"一个扫描周期完成angle_raw"<<angle_raw;
//            Process1CycleData();
//        }
//#else
//        if(mStartAzimuth == -1)
//        {
//            qDebug()<<"new cycle lowrance data now:"<<QDateTime::currentDateTime().toString("hh:mm:ss zzz");
//            mStartAzimuth = angle_raw;
//            time_stas.start();
//        }
//#endif
//    }

//    emit signalRadiusFactorUpdated(m_dRadius, m_dRadius / (uCellNum - 1));
////    processVideoData(true);

////    finishiProcess = 1;
//}

void ZCHXAnalysisAndSendRadar::analysisLowranceRadarSlot(const QByteArray &sRadarData, int uLineNum, int uCellNum, int uHeading)
{
    if(mRadarType == RADAR_UNKNOWN) return;
    static QTime time_stas;
    quint64 num = QDateTime::currentMSecsSinceEpoch();
//    if(finishiProcess == 0)
//    {
//        cout<<"deal not finishi !";
//        return;
//    }
    if(upRad == 0)
        upRad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
    const char *buf = sRadarData.constData();
    int len = sRadarData.size();
//    qDebug()<<"analysisLowranceRadarSlot len:"<<len;
    BR24::Constants::radar_frame_pkt *packet = (BR24::Constants::radar_frame_pkt *)buf;//正常大小是17160

    //cout<<sizeof(BR24::Constants::radar_frame_pkt);//结构体大小是固定的64328 定的120个spoke(536*120)+frame_hdr(8)
    //qDebug()<<" packet len:"<<(int)sizeof(packet->frame_hdr);

    m_ri.packets++;//1_一共收到的包
    if (len < (int)sizeof(packet->frame_hdr)) {
        // The packet is so small it contains no scan_lines, quit!
        qDebug()<<"此包长度不吻合，丢包！";
        m_ri.broken_packets++;//1_不完整的包
        return;
    }
    //cout<<len<<sizeof(packet->frame_hdr)<<sizeof(BR24::Constants::radar_line);//第 178 行 (17160 - 8) / 536 = 32
    int scanlines_in_packet = (len - sizeof(packet->frame_hdr)) / sizeof(BR24::Constants::radar_line);
    if (scanlines_in_packet != 32) {
        cout<<"此包没有32条扫描线，丢包！";
        m_ri.broken_packets++;//1_不完整的包
        return;
    }

    //qDebug()<<"scanlines_in_packet:"<<scanlines_in_packet;
    //int systemAreaCode = config.SAC ;
    //int systemIdentificationCode = config.SIC;

    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);

    QList<int> lineData;
//    QList<int> pIndexList;
//    struct SAzmData sAzmData;
    //std::list<TrackInfo> trackList;
    zchxVideoFrameList frameList;
    double range_factor;
    for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
        QDateTime curDateTime_1 = QDateTime::currentDateTime();
        QDateTime startDateTime_1(QDate(curDateTime_1.date().year(),curDateTime_1.date().month(),
                                        curDateTime_1.date().day()),QTime(0, 0));
        int time_of_day_1 = startDateTime_1.secsTo(curDateTime_1);//当前时间

        BR24::Constants::radar_line *line = &packet->line[scanline];

        // Validate the spoke
        int spoke = (line->common.scan_number[0] | (line->common.scan_number[1] << 8)) % SPOKES;

        m_ri.spokes++;
        //cout<<spoke;

        if (line->common.headerLen != 0x18) {
            qDebug()<<"strange header length:" << line->common.headerLen;
            // Do not draw something with this...
            qDebug()<<"该"<< scanline << "扫描线头长度不是24字节，丢包！";
            m_ri.missing_spokes++;//1_没有扫描线
            m_next_spoke = (spoke + 1) % SPOKES;

            cout<<"打印missing_spokes";
            continue;
        }

        //probably status: 02 valid data; 18 spin up
        if (line->common.status != 0x02 && line->common.status != 0x12) {
            qDebug()<<"strange status:" << line->common.status;
            qDebug()<<"该"<< scanline << "扫描线状态不对，无效！";
            m_ri.broken_spokes++;//1_不完整扫描线
        }
        if (m_next_spoke >= 0 && spoke != m_next_spoke) {
            if (spoke > m_next_spoke) {
                m_ri.missing_spokes += spoke - m_next_spoke;

            } else {
                m_ri.missing_spokes += SPOKES + spoke - m_next_spoke;
                cout << "m_ri.missing_spokes_2:"<<m_ri.missing_spokes;
                cout<<"打印missing_spokes";
            }
            qDebug()<<"error spoke:"<<spoke<<m_next_spoke<<m_uSourceID;
        }
        m_next_spoke = (spoke + 1) % SPOKES;
        //qDebug()<<"current spoke:"<<spoke<<" next:"<<m_next_spoke;
        int range_raw = 0;
        int angle_raw = 0;
        short int heading_raw = 0;
        double range_meters = 0;

        heading_raw = (line->common.heading[1] << 8) | line->common.heading[0];
        if(mUseNativeRadius)
        {
            switch (mRadarType) {
            case RADAR_BR24:
            {
                range_raw = ((line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
                angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
                range_meters = (int)((double)range_raw * 10.0 / sqrt(2.0));
                break;
            }

            case RADAR_3G:
            case RADAR_4G: {
                short int large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
                short int small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
                angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
                if (large_range == 0x80) {
                    if (small_range == -1) {
                        range_meters = 0;  // Invalid range received
                    } else {
                        range_meters = small_range / 4;
                    }
                } else {
                    range_meters = large_range * 64;
                }
                break;
            }

            case RADAR_6G:
            {
                uint16_t large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
                uint16_t small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
                angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
                if (large_range == 0x80) {
                    if (small_range == 0xffff) {
                        range_meters = 0;  // Invalid range received
                    } else {
                        range_meters = small_range / 4;
                    }
                } else {
                    range_meters = large_range * small_range / 512;
                }
                break;
            }

            default:
                break;
            }
        } else
        {
            if (memcmp(line->br24.mark, BR24MARK, sizeof(BR24MARK)) == 0) {
                // BR24 and 3G mode
                range_raw = ((line->br24.range[3] & 0xff) << 24 | (line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
                angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
                range_meters = (int)((double)range_raw * 10.0 / sqrt(2.0));
                cout<<"半径_1 range_meters"<<range_meters;
                //range_meters = 7249.92;//1_写死距离因子,和半径

                qDebug()<<"br24";

                // test code:
                if (g_lastRange != range_meters) {
                    LOG(LOG_RTM, "%s %s %d, BR24 and 3G mode: range_raw = %d, range_meters = %f",
                        __FILE__, __FUNCTION__,__LINE__,
                        range_raw, range_meters);

                    g_lastRange = range_meters;
                }
            } else {
                {
                    uint16_t large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
                    uint16_t small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
                    if (large_range == 0x80) {
                        if (small_range == 0xffff) {
                            range_meters = 0;  // Invalid range received
                        } else {
                            range_meters = small_range / 4;
                        }
                    } else {
                        range_meters = large_range * small_range / 512;
                    }
                }
                //cout<<"br4g";
                // 4G mode
                short int large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
                short int small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
                //cout<<"large_range=" << (line->br4g.largerange[1] << 8) <<(line->br4g.largerange[0]) ;
                //cout<<"small_range=" << (line->br4g.smallrange[1] << 8) <<(line->br4g.smallrange[0]);
                angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
                if (large_range == 0x80 && (line->br4g.smallrange[1] << 8)<32768) {
                    //if (large_range == 0x80  ) {
                    if (small_range == -1) {
                        range_raw = 0;  // Invalid range received
                    } else {
                        //cout<<"small_range"<<small_range<<"large_range"<<large_range;
                        range_raw = small_range;
                        range_meters = range_raw / 4;
                        //if(range_meters < 2600) range_meters =2600;
                    }
                }
                else {
                    if(sRadarType == "6G")
                    {
                        //6G
                        int Rad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
                        range_meters = (Rad*7.0312)/4;
                        singalShowRadiusCoefficient(range_meters,1.7578);
                        double k = Utils::Profiles::instance()->value(str_radar,"RadiusCoefficient").toDouble();
                        if(k <= 0)
                        {
                            k = 1.7578;
                        }
                        range_meters = Rad * k;
                    }
                    else
                    {
                        //4G
                        //cout<<"small_range"<<small_range<<"large_range"<<large_range;
                        /*range_raw = large_range * 256;
                    range_meters = range_raw / 4;*/
                        int Rad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
                        range_meters = Rad*(1.3847);
                        singalShowRadiusCoefficient(range_meters,1.3847);
                        double k = Utils::Profiles::instance()->value(str_radar,"RadiusCoefficient").toDouble();
                        if(k <= 0)
                        {
                            k = 1.3847;
                        }
                        range_meters = Rad * k;
                    }
                }
                //只有通道一打印
                int nowRad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt() ;
                if( upRad != nowRad && m_uSourceID == 1)
                {
                    cout<<"打印2"<<upRad<<nowRad<<m_uSourceID;
                    upRad = Utils::Profiles::instance()->value(str_radar,"Radar_up_radius").toInt();
                    LOG(LOG_RTM, "上传半径:%d,  small_range:%d, large_range:%d, 返回半径:= %f", upRad, small_range,large_range,range_meters);
                }

            }
        }

        int azimuth_cell = uLineNum;
        bool radar_heading_valid = (((heading_raw) & ~(HEADING_TRUE_FLAG | (azimuth_cell - 1))) == 0);
        double heading;
        if (radar_heading_valid) {
            double heading_rotation = (((heading_raw) + 2 * azimuth_cell) % azimuth_cell);
            double heading_degrees = ((heading_rotation) * (double)DEGREES_PER_ROTATION / azimuth_cell);
            heading = (fmod(heading_degrees + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION));
        } else {
            //ZCHXLOG_DEBUG("radar_heading_valid=" << radar_heading_valid );
        }
        //角度值强制变成偶数
        int origin_angle_raw = angle_raw;
        if(0){
            //雷达扫描线检测
            static QMap<int, int> counterMap;
            if(counterMap.contains(spoke) && counterMap[spoke] != origin_angle_raw)
            {
                qDebug()<<"error spoke angle find now:"<<spoke<<origin_angle_raw<<counterMap[spoke];
            } else
            {
                counterMap[spoke] = origin_angle_raw;
            }

        }
        if(angle_raw % 2)
        {
            angle_raw += 1;
        }
        if(angle_raw == 4096) angle_raw = 0;
        angle_raw = MOD_ROTATION2048(angle_raw / 2);  //让方向和一圈的扫描线个数保持一致(2048)
        //qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw;
        double start_range = 0.0 ;        

        //qDebug()<<"range_meter:"<<range_meters<<" cellNum:"<<uCellNum<<" range_factor"<<range_factor;

        lineData.clear();
        //double dAzimuth = angle_raw * (360.0 / (uLineNum / 2)) + uHeading;
        //cout<<"dAzimuth:"<<dAzimuth<<"angle_raw"<<angle_raw<<"uHeading"<<uHeading; //1_扫描方位,angle_raw(0-2047),uHeading(180)

        //赋值振幅
#ifndef DOPPLER_USED
        for (int range = 0; range < uCellNum; range++) {
            lineData.append((int)(line->data[range]));
        }
#else
        int doppler = mDopplerVal;
        if (doppler < 0 || doppler > 2) {
            doppler = 0;
        }
        uint8_t *lookup_low = lookupData[LOOKUP_SPOKE_LOW_NORMAL + doppler];
        uint8_t *lookup_high = lookupData[LOOKUP_SPOKE_HIGH_NORMAL + doppler];
        for (int i = 0; i < uCellNum; i++) {
            int temp = line->data[i];
            lineData.append(lookup_low[temp]);
            lineData.append(lookup_high[temp]);
        }
#endif
        //cout<<"spoke:"<<spoke<<" angle:"<<angle_raw<<heading_raw;
        range_factor = range_meters/lineData.size();
        if(m_VideoProcessor) m_VideoProcessor->setRangeFactor(range_factor);
        if(m_targetTrack) m_targetTrack->setRangefactor(range_factor);
        //检查是否是经过了一次扫描周期,如果是,发出数据开始解析
        if(mStartAzimuth >= 0 && mStartAzimuth == angle_raw)
        {
            int msec = time_stas.elapsed();
            if(m_VideoProcessor) m_VideoProcessor->setRadarSpr(msec / 1000.0);
            //一个扫描周期完成
//            cout<<"一个扫描周期完成angle_raw"<<angle_raw<<" data size:"<<m_radarVideoMap.size();
            qDebug()<<"cycle data end:"<<QDateTime::currentDateTime().toString("hh:mm:ss zzz")<<msec;
            processVideoData(true);
            m_radarVideoMap.clear();
            mStartAzimuth = -1;
        }

        //cout<<"angle_raw"<<angle_raw;
        RADAR_VIDEO_DATA objVideoData;
        radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toInt();//雷达编号
        objVideoData.m_uSourceID = m_uSourceID; //1 接入雷达编号
        objVideoData.m_uSystemAreaCode = 1; //系统区域代码
        objVideoData.m_uSystemIdentificationCode = 1; //系统识别代码
        objVideoData.m_uMsgIndex = spoke; //消息唯一序列号
        objVideoData.m_uAzimuth = angle_raw; //扫描方位
        objVideoData.m_dStartRange = start_range;//扫描起始距离
        objVideoData.m_dRangeFactor = range_factor;//1_距离因子
        objVideoData.m_uTotalCellNum = lineData.size();//1_一条线上有多少个点
        objVideoData.m_dCentreLon = m_dCentreLon; //中心经度
        objVideoData.m_dCentreLat = m_dCentreLat; //中心纬度
        objVideoData.m_uLineNum = uLineNum/2; //1_总共线的个数
        objVideoData.m_uHeading = uHeading;//雷达方位

        objVideoData.mLineData = lineData;
        objVideoData.m_timeofDay = time_of_day_1;
        //半径
        m_dRadius = range_meters;
        //m_dDiameter = m_dRadius*2;
//        qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw<<(line->br24.angle[1] << 8)<< line->br24.angle[0]<<m_dRadius;
//        qDebug()<<"radius:"<<m_dRadius;
        m_radarVideoMap[objVideoData.m_uMsgIndex % 2048] = objVideoData;
        if(mStartAzimuth == -1)
        {
            qDebug()<<"new cycle lowrance data now:"<<QDateTime::currentDateTime().toString("hh:mm:ss zzz");
            mStartAzimuth = angle_raw;
            time_stas.start();
        }
    }

    emit signalRadiusFactorUpdated(m_dRadius, m_dRadius / (uCellNum - 1));
//    processVideoData(true);

//    finishiProcess = 1;
}

//发送雷达回波余辉图片
void ZCHXAnalysisAndSendRadar::setRadarAfterglowPixmap(const int uIndex, const QPixmap &videoPixmap, const QPixmap &objPixmap, const QPixmap &prePixmap)
{
    if(!mRadarOutMgr) return;
    //回波图片转二进制
    QByteArray videoArray;
    QBuffer videoBuffer(&videoArray);
    videoBuffer.open(QIODevice::WriteOnly);
    videoPixmap.save(&videoBuffer ,"PNG");

    //当前余辉图片转二进制
    QByteArray pixArray;
    QBuffer buffer(&pixArray);
    buffer.open(QIODevice::WriteOnly);
    objPixmap.save(&buffer ,"PNG");

    //前一张余辉图片转二进制
    QByteArray preArray;
    QBuffer preBuffer(&preArray);
    preBuffer.open(QIODevice::WriteOnly);
    if(!m_prePixmap.isNull())
        m_prePixmap.save(&preBuffer ,"PNG");

    com::zhichenhaixin::proto::RadarVideo *objRadarVideo = new com::zhichenhaixin::proto::RadarVideo;

    //封装proto
    radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toInt();//雷达编号
    objRadarVideo->set_radarid(radar_num);
    objRadarVideo->set_radarname("雷达回波余辉");
    objRadarVideo->set_latitude(m_dCentreLat);
    objRadarVideo->set_longitude(m_dCentreLon);
    objRadarVideo->set_utc(QDateTime::currentMSecsSinceEpoch());
    objRadarVideo->set_height(videoPixmap.height());
    objRadarVideo->set_width(videoPixmap.width());
    objRadarVideo->set_radius(m_dRadius);
    objRadarVideo->set_imagedata(videoArray.data(),videoArray.size());

//    cout<<"图片纬度m_dCentreLat"<<m_dCentreLat;
//    cout<<"图片经度m_dCentreLat"<<m_dCentreLon;
    //以下是余辉要用的
    objRadarVideo->set_curimagedata(pixArray.data(),pixArray.size());
    if(!m_prePixmap.isNull())
        objRadarVideo->set_preimagedata(preArray.data(),preArray.size());
    else
        objRadarVideo->set_preimagedata(NULL,0);
    objRadarVideo->set_loopnum(m_uLoopNum);
    objRadarVideo->set_curindex(uIndex);

    //通过zmq发送
//    QByteArray sendData;
//    sendData.resize(objRadarVideo.ByteSize());
//    objRadarVideo.SerializePartialToArray(sendData.data(),sendData.size());

    mRadarOutMgr->appendData(zchxRadarUtils::protoBufMsg2ByteArray(objRadarVideo), mRadarVideoTopic, mRadarVideoPort);
    delete objRadarVideo;
}


//zmq发送雷达回波矩形图片
void ZCHXAnalysisAndSendRadar::sendRadarRectPixmap(const zchxRadarRectMap& map)
{
    if((!mRadarOutMgr) || map.size() == 0) return;
    zchxRadarRects *totalRadar_Rects = new zchxRadarRects;
    foreach (zchxRadarRect rect, map) {
        zchxRadarRect *mRadarRect = totalRadar_Rects->add_rects();
        mRadarRect->CopyFrom(rect);
        //没有确定的点不绘制历史
        if(!mRadarRect->dirconfirmed())
        {
            mRadarRect->clear_historyrects();
        }
    }
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    totalRadar_Rects->set_length(map.size());
    totalRadar_Rects->set_utc(utc);
    //通过zmq发送
    mRadarOutMgr->appendData(zchxRadarUtils::protoBufMsg2ByteArray(totalRadar_Rects), mRadarRectTopic, mRadarRectPort);
    delete totalRadar_Rects;
//    QByteArray totalData;//整体发送
//    totalData.resize(totalRadar_Rects.ByteSize());
//    totalRadar_Rects.SerializeToArray(totalData.data(),totalData.size());
//    mRadarRectPubThred->slotRecvContents(totalData);
}

//zmq发送雷达回波矩形图片
void ZCHXAnalysisAndSendRadar::sendRadarNodeRoute(const zchxRadarRouteNodes& list)
{
    if((!mRadarOutMgr) || list.nodes_size() == 0) return;

    zchxRadarRouteNodes* nodes = new zchxRadarRouteNodes(list);
    qDebug()<<"send data node size:"<<list.nodes_size();
    //通过zmq发送
    mRadarOutMgr->appendData(zchxRadarUtils::protoBufMsg2ByteArray(nodes), "RadarRoute", mRadarTrackPort);
    delete nodes;
}


void ZCHXAnalysisAndSendRadar::processVideoData(bool rotate)
{
    if(m_VideoProcessor == 0) return;
    if (m_radarVideoMap.isEmpty()) {
        return;
    }


    zchxRadarVideoTask task;
    task.m_RadarVideo = m_radarVideoMap;
    task.m_Range = m_dRadius;
    task.m_Rotate = rotate;
    task.m_TimeStamp = QDateTime::currentMSecsSinceEpoch();

    m_VideoProcessor->appendSrcData(task);
}

//发送合并的雷达目标
void ZCHXAnalysisAndSendRadar::slotSendComTracks(const zchxRadarSurfaceTrack& tracks)
{
   if(mRadarOutMgr)
   {
//       qDebug()<<"data outto mgr send time:"<<QDateTime::currentDateTime();
       zchxRadarSurfaceTrack* track = new zchxRadarSurfaceTrack(tracks);
       mRadarOutMgr->appendData(zchxRadarUtils::protoBufMsg2ByteArray(track), mRadarTrackTopic, mRadarTrackPort);
       delete track;
   }
}

void ZCHXAnalysisAndSendRadar::setRangeFactor(double factor)
{
    mRangeFactor = factor;
    cout<<"setRangeFactor距离因子改变了";
}

void ZCHXAnalysisAndSendRadar::slotGetGpsData(double lat, double lon)
{
    //cout<<"页面显示GPS经纬,配置文件更新经纬度";
    m_dCentreLat = lat;
    m_dCentreLon = lon;
    Utils::Profiles::instance()->setValue(str_radar,"Centre_Lat",m_dCentreLat);
    Utils::Profiles::instance()->setValue(str_radar,"Centre_Lon",m_dCentreLon);
}

void ZCHXAnalysisAndSendRadar::showTrackNumSlot(bool flag)
{
//    cout<<"显示编号"<<flag;
//    if(m_DrawRadarVideo != NULL)
//    m_DrawRadarVideo->SignalShowTrackNum(flag);
}

void ZCHXAnalysisAndSendRadar::updateFloatSlot()//更新浮标配置
{
    fMap.clear();
    for(int i =0; PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).isNull() != 1; i++)
    {
        QStringList mStrList = PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).toStringList();
        fMap[i] = mStrList;
    }
    floatRange = PROFILES_INSTANCE->value("FloatSet","range").toInt();
    cout<<"更新浮标配置-fMap"<<fMap.size()<<floatRange;
}

float ZCHXAnalysisAndSendRadar::getAngle(float x1,float y1,float x2,float y2)
{
    float angle = 0.0;
    int len_x = x1 - x2;
    int len_y = y1 - y2;
    double aby = std::abs(len_y);
    double abx = std::abs(len_x);
    double tan_yx = aby/abx;
    if(len_y > 0 && len_x < 0)
    {
        cout<<"情况1:"<<len_x<<len_y;
        angle =270 + atan(tan_yx)*180/M_PI;
    }
    else if (len_y > 0 && len_x > 0)
    {
        cout<<"情况2:"<<len_x<<len_y;
        angle =180+90- atan(tan_yx)*180/M_PI;
    }
    else if(len_y < 0 && len_x < 0)
    {
        cout<<"情况3:"<<len_x<<len_y;
        angle =90- atan(tan_yx)*180/M_PI;
    }
    else if(len_y < 0 && len_x > 0)
    {
        cout<<"情况4:"<<len_x<<len_y;
        angle = 90 + atan(tan_yx)*180/M_PI;
    }
    else if(len_x == 0 && len_y > 0)
    {
        angle = 0;
    }
    else if(len_x == 0 && len_y < 0)
    {
        angle = 180;
    }
    else if(len_y == 0 && len_x > 0)
    {
        angle = 90;
    }
    else if(len_y == 0 && len_x < 0)
    {
        angle = 270;
    }
    return angle;
}

void ZCHXAnalysisAndSendRadar::slotTrackMap(QMap<int,QList<TrackNode>> map)
{
//    if (!m_DrawRadarVideo) {
//        return;
//    }
//    m_DrawRadarVideo->slotTrackMap(map,m_uSourceID);
    //signalCombineVideo(map,m_uSourceID);
}
//绘制2个通道的回波
void ZCHXAnalysisAndSendRadar::slotDrawCombinVideo(QList<TrackNode> mList)
{
//     if (m_DrawRadarVideo == NULL) {
//        return;
//    }
//    m_DrawRadarVideo->drawCombineVideo(mList);
}

void ZCHXAnalysisAndSendRadar::slotSetRadarType(int type)
{
    mRadarType = type;
}
