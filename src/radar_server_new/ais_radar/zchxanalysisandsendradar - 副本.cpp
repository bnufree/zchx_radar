#include "zchxanalysisandsendradar.h"
#include "BR24.hpp"
#include <QDebug>
#include <QtCore/qmath.h>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <zlib.h>
#include <QDir>
#include <QBuffer>
#include "../profiles.h"
#include "Log.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
extern "C"

{

#include "ctrl.h"

}
static uint8_t BR24MARK[] = {0x00, 0x44, 0x0d, 0x0e};
int g_lastRange = 0;

#define         PROCESS_CYCLE_DATA

ZCHXAnalysisAndSendRadar::ZCHXAnalysisAndSendRadar(RadarConfig* cfg, QObject *parent)
    : QObject(parent),
      mRangeFactor(13.0),
      mRadarConfig(cfg),
      m_uSourceID(cfg == 0 ? 0 : cfg->getID()),
      mStartAzimuth(-1)
{
    //qRegisterMetaType<QMap<QList<Radar_Rect>>("QList<Radar_Rect>");
    //flag = 1;
    finish_flag = 1;
    qRegisterMetaType<QMap< int,QList<QPointF> >>("QMap< int,QList<QPointF> >");
    qRegisterMetaType<zchxRadarRectList>("const zchxRadarRectList&");
    qRegisterMetaType<zchxRadarRectDefList>("const zchxRadarRectDefList&");
    qRegisterMetaType<zchxRadarRectMap>("const zchxRadarRectMap&");
    qRegisterMetaType<QMap<int,QList<TrackNode>>>("QMap<int,QList<TrackNode>>");


    d_32 = 0;//初始化每次推给周老师库的个数
    send_finish = true;
    first = true;
    rect_finish = true;
    track_finish = true;
    m_ri.broken_packets = 0;
    m_ri.broken_spokes = 0;
    m_ri.missing_spokes = 0;
    m_ri.packets = 0;
    m_ri.spokes = 0;
    str_radar = QString("Radar_%1").arg(m_uSourceID);

    //设定预推的默认参数
    Utils::Profiles::instance()->setDefault(str_radar,"PushTimes", 6);
    Utils::Profiles::instance()->setDefault(str_radar,"PushInterval", 5);
    //F5FC74  F6E140
    //1165E34  1175E00
    m_jupmdis = Utils::Profiles::instance()->value(str_radar,"jump_distance").toInt();
    sRadarType = Utils::Profiles::instance()->value(str_radar,"Radar_Type").toString();
    m_limit_file = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();//读取限制区域文件
    m_dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble();
    m_uLoopNum = Utils::Profiles::instance()->value(str_radar,"Loop_Num").toInt();
    m_bLimit = Utils::Profiles::instance()->value(str_radar,"Limit").toBool();
    m_uVideoSendPort = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toInt();
    m_sVideoTopic = Utils::Profiles::instance()->value(str_radar,"video_Topic").toString();
    m_uTrackSendPort = Utils::Profiles::instance()->value(str_radar,"Track_Send_Port").toInt();
    m_sTrackTopic = Utils::Profiles::instance()->value(str_radar,"Track_Topic").toString();
    cell_num = Utils::Profiles::instance()->value(str_radar,"Cell_Num").toInt();
    m_jupmdis = Utils::Profiles::instance()->value(str_radar,"jump_distance").toInt();
    mTargetMergeRadius = Utils::Profiles::instance()->value(str_radar,"Radius").toInt();
    m_distance = Utils::Profiles::instance()->value(str_radar,"Distance").toDouble();
    m_clearRadarTrackTime = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();
    radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toInt();//雷达编号
    hIniNum = Utils::Profiles::instance()->value(str_radar,"historyNum").toDouble(); //自定义保存多少笔
    mDirectionInvertHoldValue = Utils::Profiles::instance()->value(str_radar,"Direction_Invert").toDouble();
    mAdjustCogEnabled = Utils::Profiles::instance()->value(str_radar,"azimuth_adjustment").toBool();

    m_uRectSendPort =  Utils::Profiles::instance()->value(str_radar,"Yuhui_Send_Port").toInt();
    m_sRectTopic = Utils::Profiles::instance()->value(str_radar,"Yuhui_Topic").toString();

    rectNum = 1+(radar_num)*10000;
    cout<<"初始化编号"<<rectNum;
    objNum = 1+(radar_num)*10000;
    maxNum = objNum + 9998;

    m_DrawRadarVideo = NULL;
    fMap.clear();//浮标模块
    for(int i =0; PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).isNull() != 1; i++)
    {
        QStringList mStrList = PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).toStringList();
        fMap[i] = mStrList;
    }
    floatRange = PROFILES_INSTANCE->value("FloatSet","range").toInt();
    cout<<"更新浮标配置-fMap"<<fMap.size()<<floatRange;

    m_pGetTrackProcess = new ZCHXGetTrackProcess(mRadarConfig);

    connect(m_pGetTrackProcess, SIGNAL(sendTrack(zchxTrackPointList)),
            this, SLOT(sendTrackSlot2(zchxTrackPointList)));

    connect(m_pGetTrackProcess,SIGNAL(signalShowTheLastPot(QList<QPointF>,QList<QPointF>)),
            this,SIGNAL(signalShowTheLastPot(QList<QPointF>,QList<QPointF>)));

    connect(this, SIGNAL(startTrackProcessSignal(zchxVideoFrameList)),
            m_pGetTrackProcess, SIGNAL(getTrackProcessSignal(zchxVideoFrameList)));


    //发送雷达目标的zmq
    //创建context，zmq的socket 需要在context上进行创建
    m_pTrackContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pTrackLisher= zmq_socket(m_pTrackContext, ZMQ_PUB);
    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uTrackSendPort);
    zmq_bind(m_pTrackLisher, sIPport.toLatin1().data());//
    //监听雷达目标zmq
    QString monitorTrackUrl = "inproc://monitor.radarTrackclient";
    zmq_socket_monitor (m_pTrackLisher, monitorTrackUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pTrackMonitorThread = new ZmqMonitorThread(m_pTrackContext, monitorTrackUrl, 0);
    connect(m_pTrackMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pTrackMonitorThread, SIGNAL(finished()), m_pTrackMonitorThread, SLOT(deleteLater()));
    m_pTrackMonitorThread->start();

    //余辉ZMQ
    //创建context，zmq的socket 需要在context上进行创建
    m_pRectContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pRectLisher= zmq_socket(m_pRectContext, ZMQ_PUB);
    QString sRectport = "tcp://*:";
    //int rectPort = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toInt();
    sRectport += QString::number(m_uRectSendPort);
    int sts = zmq_bind(m_pRectLisher, sRectport.toLatin1().data());//
    qDebug()<<"bind zmq rect returened:"<<sts;
    //监听矩形回波zmq
    QString monitorRectUrl = "inproc://monitor.radarRectclient";
    zmq_socket_monitor (m_pRectLisher, monitorRectUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pRectMonitorThread = new ZmqMonitorThread(m_pRectContext, monitorRectUrl, 0);
    connect(m_pRectMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pRectMonitorThread, SIGNAL(finished()), m_pRectMonitorThread, SLOT(deleteLater()));
    m_pRectMonitorThread->start();

    //回波ZMQ
    //创建context，zmq的socket 需要在context上进行创建
    m_pVideoContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pVideoLisher= zmq_socket(m_pVideoContext, ZMQ_PUB);
    QString sVideoport = "tcp://*:";
    //int videoPort = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toInt();
    sVideoport += QString::number(m_uVideoSendPort);
    zmq_bind(m_pVideoLisher, sVideoport.toLatin1().data());//
    //监听矩形回波zmq
    QString monitorVideoUrl = "inproc://monitor.radarVideoclient";
    zmq_socket_monitor (m_pVideoLisher, monitorVideoUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pVideoMonitorThread = new ZmqMonitorThread(m_pVideoContext, monitorVideoUrl, 0);
    connect(m_pVideoMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pVideoMonitorThread, SIGNAL(finished()), m_pVideoMonitorThread, SLOT(deleteLater()));
    m_pVideoMonitorThread->start();


    readRadarLimitFormat();//读取限制区域

    //初始新科雷达解析库的调用
    QString sAppPath = QApplication::applicationDirPath();
    m_sPath = sAppPath+"/AsterixSpecification/asterix.ini";
    std::string str = m_sPath.toStdString();
    const char* asterixDefinitionsFile = str.data();
    //qDebug()<<"path"<<asterixDefinitionsFile;
    m_pCAsterixFormat = new CAsterixFormat(asterixDefinitionsFile);
    m_pCAsterixFormatDescriptor = dynamic_cast<CAsterixFormatDescriptor*>(m_pCAsterixFormat->CreateFormatDescriptor(0, ""));

    //回波块识别
//    mVideoRects = new zchxVideoRects(m_uSourceID, true);
//    connect(mVideoRects,SIGNAL(signalSendTrackNodes(QMap<int,QList<TrackNode> >)),
//            this,SIGNAL(signalTrackList(QMap<int,QList<TrackNode> >)));
//    connect(mVideoRects,SIGNAL(signalSendTrackNodes(QMap<int,QList<TrackNode> >)),
//            this,SLOT(slotTrackMap(QMap<int,QList<TrackNode> >)));

//    connect(this,SIGNAL(signalAnalysisVideoPiece(QMap<int,RADAR_VIDEO_DATA>,double)),
//            mVideoRects,SLOT(analysisVideoPieceSlot(QMap<int,RADAR_VIDEO_DATA>,double)));

    connect(this, SIGNAL(analysisLowranceRadarSignal(QByteArray,int,int,int)),
            this, SLOT(analysisLowranceRadarSlot(QByteArray,int,int,int)));
    connect(this, SIGNAL(analysisCatRadarSignal(QByteArray,int,int,int,QString)),
            this, SLOT(analysisCatRadarSlot(QByteArray,int,int,int,QString)));
    connect(this,SIGNAL(showTrackNumSignal(bool)),this,SLOT(showTrackNumSlot(bool)));
    //回波块矩形目标识别
//    mRectExtractionThread = new zchxRectExtractionThread(this);
//    connect(mRectExtractionThread, SIGNAL(signalSendRects(zchxRadarRectDefList)),
//            this, SLOT(sendVideoRects(zchxRadarRectDefList)));
//    mRectExtractionThread->start();
    m_workThread.setStackSize(64000000);
    moveToThread(&m_workThread);
    m_workThread.start();

    m_pTimer = new QTimer();
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    m_pTimer->start(2000);
    m_pTimer_1 = new QTimer();
    connect(m_pTimer_1, SIGNAL(timeout()), this, SLOT(handleTimeout_1()));
    m_pTimer_1->start(100);

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
      zmq_close(m_pVideoLisher);
      zmq_ctx_destroy(m_pVideoContext);

      zmq_close(m_pTrackLisher);
      zmq_ctx_destroy(m_pTrackContext);

      zmq_close(m_pRectLisher);
      zmq_ctx_destroy(m_pRectContext);
      if(m_DrawRadarVideo)
      {
          delete m_DrawRadarVideo;
          m_DrawRadarVideo = NULL;
      }
      if(m_pGetTrackProcess)
      {
          delete m_pGetTrackProcess;
          m_pGetTrackProcess = NULL;
      }
      if(m_pCAsterixFormat)
      {
          delete m_pCAsterixFormat;
          m_pCAsterixFormat = NULL;
      }
      if(m_pCAsterixFormatDescriptor)
      {
          delete m_pCAsterixFormatDescriptor;
          m_pCAsterixFormatDescriptor = NULL;
      }
      if(m_pTrackMonitorThread->isRunning())
      {
          m_pTrackMonitorThread->quit();
      }
      m_pTrackMonitorThread->terminate();
      if(m_pVideoMonitorThread->isRunning())
      {
          m_pVideoMonitorThread->quit();
      }
      m_pVideoMonitorThread->terminate();
      if(m_pRectMonitorThread->isRunning())
      {
          m_pRectMonitorThread->quit();
      }
      m_pRectMonitorThread->terminate();
     cout<<"~ZCHXAnalysisAndSendRadar()8";
}

ZCHXAnalysisAndSendRadar::closeTT()
{
    fclose(m_pFile);
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

void ZCHXAnalysisAndSendRadar::Process1CycleData()
{
    QMap<int, zchxVideoFrame> frameMap;
    QMap<int, RADAR_VIDEO_DATA>::iterator it = m_radarVideoMap.begin();
    QTime t;
    t.start();
    for(; it != m_radarVideoMap.end(); it++)
    {
        //从0到到4095开始扫描.其实到2047就是一圈,雷达每次扫描两圈
        int spoke = it.key() % 2048;
        if(frameMap.contains(spoke))
        {
            //重新确认cell单元的数据
            zchxVideoFrame& video = frameMap[spoke];
            for (int i = 0; i < it.value().m_pIndex.size(); i++) {
                int subIndex = it.value().m_pIndex[i];
                if (subIndex < DATA_NUM_IN_AZM) {
                    video.set_amplitude(subIndex, it.value().m_pAmplitude[i]);
                }
            }
        } else
        {
            zchxVideoFrame video;
            video.set_systemareacode(1);
            video.set_systemidentificationcode(1);
            video.set_msgindex(it.value().m_uMsgIndex);
            video.set_azimuth(it.value().m_uAzimuth);
            //qDebug()<<"video spoke:"<<video.msgindex()<<video.azimuth();
            video.set_heading(it.value().m_uHeading);
            video.set_startrange(it.value().m_dStartRange);
            video.set_rangefactor(it.value().m_dRangeFactor);
            video.set_bitresolution(::com::zhichenhaixin::proto::RES(4));
            video.set_timeofday(it.value().m_timeofDay);
            for(int i=0; i<DATA_NUM_IN_AZM; i++)
            {
                video.add_amplitude(0);
            }
            for (int i = 0; i < it.value().m_pIndex.size(); i++) {
                int subIndex = it.value().m_pIndex[i];
                if (subIndex < DATA_NUM_IN_AZM) {
                    video.set_amplitude(subIndex, it.value().m_pAmplitude[i]);
                }
            }
            frameMap[spoke] = video;
        }
    }
//    if(m_pGetTrackProcess->getFinish())
//        emit startTrackProcessSignal(frameMap.values());
    //m_radarVideoMap.clear();
//    if(mVideoRects->isFinishProcess())
//    {
//        signalAnalysisVideoPiece(m_radarVideoMap,m_dRadius);//发送一圈回波块进行解析
//        //LOG(LOG_RTM, "%s %s %d,发送解析回波", __FILE__, __FUNCTION__,__LINE__);
//    } else
//    {
//        qDebug()<<"Rect thread is running. not process video data";
//    }
}

//解析小雷达目标
void ZCHXAnalysisAndSendRadar::analysisLowranceRadarSlot(const QByteArray &sRadarData, int uLineNum, int uCellNum, int uHeading)
{
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

    QList<int> AmplitudeList;
    QList<int> pIndexList;
    struct SAzmData sAzmData;
    //std::list<TrackInfo> trackList;
    SAzmDataList srcDataList;
    zchxVideoFrameList frameList;
    double range_factor;
    for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
        QDateTime curDateTime_1 = QDateTime::currentDateTime();
        QDateTime startDateTime_1(QDate(curDateTime_1.date().year(),curDateTime_1.date().month(),
                                        curDateTime_1.date().day()),QTime(0, 0));
        int time_of_day_1 = startDateTime_1.secsTo(curDateTime_1);//当前时间

        BR24::Constants::radar_line *line = &packet->line[scanline];

        // Validate the spoke
        int spoke = line->common.scan_number[0] | (line->common.scan_number[1] << 8);

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
        if(angle_raw % 2)
        {
            angle_raw += 1;
        }
        if(angle_raw == 4096) angle_raw = 0;
//        qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw<<(line->br24.angle[1] << 8)<< line->br24.angle[0];
        angle_raw = MOD_ROTATION2048(angle_raw / 2);  //让方向和一圈的扫描线个数保持一致(2048)
        //qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw;
        double start_range = 0.0 ;
        range_factor = range_meters/uCellNum;

        //qDebug()<<"range_meter:"<<range_meters<<" cellNum:"<<uCellNum<<" range_factor"<<range_factor;
        mRadarConfig->setRangeMin(0);
        mRadarConfig->setRangeMax(range_meters);

        AmplitudeList.clear();
        pIndexList.clear();
        //double dAzimuth = angle_raw * (360.0 / (uLineNum / 2)) + uHeading;
        //cout<<"dAzimuth:"<<dAzimuth<<"angle_raw"<<angle_raw<<"uHeading"<<uHeading; //1_扫描方位,angle_raw(0-2047),uHeading(180)

        //赋值振幅
        for (int range = 0; range < uCellNum; range++) {
            int value = (int)(line->data[range]);
            sAzmData.iRawData[range] = 0;
            if (value > 0) {
                AmplitudeList.append(value);
                pIndexList.append(range);
            }
        }
        //cout<<"spoke:"<<spoke<<" angle:"<<angle_raw<<heading_raw;
        //检查是否是经过了一次扫描周期,如果是,发出数据开始解析
        if(mStartAzimuth >= 0 && mStartAzimuth == angle_raw)
        {
            qDebug()<<"video data elapsed:"<<time_stas.elapsed();
            //一个扫描周期完成
//            cout<<"一个扫描周期完成angle_raw"<<angle_raw<<" data size:"<<m_radarVideoMap.size();
//            qDebug()<<"cycle data end:"<<QDateTime::currentDateTime().toString("hh:mm:ss zzz");
            processVideoData(true);
//            Process1CycleData();
//            m_radarVideoMap.clear();
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
        objVideoData.m_uTotalNum = uCellNum;//1_一条线上有多少个点
        objVideoData.m_dCentreLon = m_dCentreLon; //中心经度
        objVideoData.m_dCentreLat = m_dCentreLat; //中心纬度
        objVideoData.m_uLineNum = uLineNum/2; //1_总共线的个数
        objVideoData.m_uHeading = uHeading;//雷达方位

        objVideoData.m_pAmplitude = AmplitudeList;
        objVideoData.m_pIndex = pIndexList;
        objVideoData.m_timeofDay = time_of_day_1;
        //半径
        m_dRadius = range_meters;
        //m_dDiameter = m_dRadius*2;
//        qDebug()<<"spoke:"<<spoke<<" angle:"<<angle_raw<<(line->br24.angle[1] << 8)<< line->br24.angle[0]<<m_dRadius;
//        qDebug()<<"radius:"<<m_dRadius;
        //有可能多次扫描线有同一个角度,这里可能将角度的值进行合并,先暂且不处理,直接覆盖
        m_radarVideoMap[objVideoData.m_uAzimuth % 2048] = objVideoData;
//        m_radarVideoMap[objVideoData.m_uMsgIndex % 2048] = objVideoData;
#if 0
        if(start_azimuth == -1)
        {
            start_azimuth = angle_raw;
           //cout<<"angle_raw"<<angle_raw;
        }
        else if(start_azimuth == angle_raw)
        {
            //一个扫描周期完成
            //cout<<"一个扫描周期完成angle_raw"<<angle_raw;
            Process1CycleData();
        }
#else
        if(mStartAzimuth == -1)
        {
            qDebug()<<"new cycle lowrance data now:"<<QDateTime::currentDateTime().toString("hh:mm:ss zzz");
            mStartAzimuth = angle_raw;
            time_stas.start();
        }
#endif
    }

    emit signalRadiusFactorUpdated(m_dRadius, m_dRadius / (uCellNum - 1));
//    processVideoData(true);

//    finishiProcess = 1;
}

void ZCHXAnalysisAndSendRadar::analysisCatRadarSlot(const QByteArray &sRadarData, int uLineNum, int uCellNum, int uHeading,const QString &sRadarType)
{
    cout<<"m_uSourceID"<<m_uSourceID<<sRadarType.size();
    //使用解析库进行解析cat010,cat240协议雷达
    //cout<<"原始数据大小"<<sRadarData.size()<<"原始数据"<<sRadarData;
    //cout<<"sRadarType"<<sRadarType;

    if(m_pCAsterixFormatDescriptor == NULL) {
        qDebug()<<"m_pCAsterixFormatDescriptor is NULL";
        return;
    }

    if(m_pCAsterixFormatDescriptor->m_pAsterixData) {
        delete m_pCAsterixFormatDescriptor->m_pAsterixData;
        m_pCAsterixFormatDescriptor->m_pAsterixData = NULL;
    }

    m_pCAsterixFormatDescriptor->m_pAsterixData = m_pCAsterixFormatDescriptor->m_InputParser.parsePacket((const unsigned char*)(sRadarData.constData()), sRadarData.size());
    DataBlock* pDB = m_pCAsterixFormatDescriptor->m_pAsterixData->m_lDataBlocks.front();
    qint64 utc = QDateTime::currentMSecsSinceEpoch();

    if (sRadarType == "cat010")
    {
        if(track_finish == false)
        {
            cout<<"目标发送失败track_finish"<<track_finish;
            return;
        }
        QString sContent = tr("process cat010 radar ");
        emit signalSendRecvedContent(utc,"CAT010",sContent);
        analysisCat010Radar(pDB);
    }
    else if (sRadarType == "cat240") {
        QString sContent = tr("process cat240 radar ");
        emit signalSendRecvedContent(utc,"CAT240",sContent);
        analysisCat240Radar(pDB,uLineNum,uCellNum,uHeading);
    }
    else if(sRadarType == "cat020")
    {
        QString sContent = tr("process cat020 radar ");
        emit signalSendRecvedContent(utc,"CAT020",sContent);
        analysisCat020Radar(sRadarData);
    }
    else if(sRadarType == "cat253")
    {
        QString sContent = tr("process cat253 radar ");
        emit signalSendRecvedContent(utc,"CAT253",sContent);
        analysisCat253Radar(sRadarData);
    }
}

//发送雷达回波余辉图片
void ZCHXAnalysisAndSendRadar::setRadarAfterglowPixmap(const int uIndex, const QPixmap &videoPixmap, const QPixmap &objPixmap, const QPixmap &prePixmap)
{
#if 1
    if(m_uSourceID != 1)return;//只有通道1发送
    qint64 utc0 = QDateTime::currentMSecsSinceEpoch();
    //cout<<"utc_发送-------------------------------------"<<utc0;
    //qDebug()<<"save---AfterglowPixmap-----------------<<"<<uIndex;
    QString path = QCoreApplication::applicationDirPath();
    QString str1 = QString("/video.png");
    QString path1 = path+str1;
    videoPixmap.save(path1);

    QString str2 = QString("/Afterglow.png");
    QString path2 = path+str2;
    objPixmap.save(path2);

    QString str3 = QString("/pre_Afterglow.png");
    QString path3 = path+str3;
    prePixmap.save(path3);

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

    com::zhichenhaixin::proto::RadarVideo objRadarVideo;

    //封装proto
    radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toInt();//雷达编号
    objRadarVideo.set_radarid(radar_num);
    objRadarVideo.set_radarname("雷达回波余辉");
    objRadarVideo.set_latitude(m_dCentreLat);
    objRadarVideo.set_longitude(m_dCentreLon);
    objRadarVideo.set_utc(QDateTime::currentMSecsSinceEpoch());
    objRadarVideo.set_height(videoPixmap.height());
    objRadarVideo.set_width(videoPixmap.width());
    objRadarVideo.set_radius(m_dRadius);
    objRadarVideo.set_imagedata(videoArray.data(),videoArray.size());

//    cout<<"图片纬度m_dCentreLat"<<m_dCentreLat;
//    cout<<"图片经度m_dCentreLat"<<m_dCentreLon;
    //以下是余辉要用的
    objRadarVideo.set_curimagedata(pixArray.data(),pixArray.size());
    if(!m_prePixmap.isNull())
        objRadarVideo.set_preimagedata(preArray.data(),preArray.size());
    else
        objRadarVideo.set_preimagedata(NULL,0);
    objRadarVideo.set_loopnum(m_uLoopNum);
    objRadarVideo.set_curindex(uIndex);

    //通过zmq发送
    QByteArray sendData;
    sendData.resize(objRadarVideo.ByteSize());
    objRadarVideo.SerializePartialToArray(sendData.data(),sendData.size());

    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uVideoSendPort);

    QString sTopic = m_sVideoTopic;//"RadarVideo";
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();

    //   zmq_bind(m_pVideoLisher, sIPport.toLatin1().data());//
    zmq_send(m_pVideoLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sendData.data(), sendData.size(), 0);
    m_prePixmap = prePixmap;

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("send analysis radar video data,size = %1").arg(sendData.size());
    emit signalSendRecvedContent(utc,"VIDEO________SEND",sContent);
    //cout<<"发送回波图片了"<<utc;
    send_finish = true;
#endif
}

void ZCHXAnalysisAndSendRadar::sendTrackSlot(const zchxTrackPoint& radarPoint)
{
    m_radarPointMap[radarPoint.tracknumber()] = radarPoint;
    qDebug()<<"size before clear:"<<m_radarPointMap.size();
    cout<<"发送_1";
    clearRadarTrack();
    sendRadarTrack();
    qDebug()<<"size after clear:"<<m_radarPointMap.size();
}

//zmq发送雷达回波矩形图片
void ZCHXAnalysisAndSendRadar::sendRadarRectPixmap()
{
    //rect_finish = false;
    //cout<<"zmq发送雷达回波矩形图片";
    if(m_uSourceID != 1)return;//只有通道1发送
    //cout<<"开始-----------------------------------------------------------";
    if(m_radarRectMap.size()<=0)
    {
        cout<<"m_radarRectMap.size()<=0";
        return;
    }
    zchxRadarRects totalRadar_Rects;
    foreach (zchxRadarRect rect, m_radarRectMap) {
        zchxRadarRect *mRadarRect = totalRadar_Rects.add_rects();
        mRadarRect->CopyFrom(rect);
        //没有确定的点不绘制历史
        if(!mRadarRect->dirconfirmed())
        {
            mRadarRect->clear_historyrects();
        }
    }
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    totalRadar_Rects.set_length(m_radarRectMap.size());
    totalRadar_Rects.set_utc(utc);
    //通过zmq发送
    QByteArray totalData;//整体发送
    totalData.resize(totalRadar_Rects.ByteSize());
    totalRadar_Rects.SerializeToArray(totalData.data(),totalData.size());

    QString sTopic = m_sRectTopic;//"RadarRect";
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();

    zmq_send(m_pRectLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pRectLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pRectLisher, totalData.data(), totalData.size(), 0);

    //rect_finish = true;
    //cout<<"rect_finish"<<rect_finish;
    QString sContent = tr("send analysis radar video data,size = %1").arg(m_radarRectMap.size());
    emit signalSendRecvedContent(utc,"Rect________SEND",sContent);
    qDebug()<<sContent;
}

//查找最近的矩形回波
zchxRadarRectList ZCHXAnalysisAndSendRadar::findRectSameObj(zchxRadarRectDef &obj, bool self)
{
    zchxRadarRectList list;
    double min_dis = ULONG_MAX;
    zchxRadarRectMap::iterator it = m_radarRectMap.begin();
    for(; it != m_radarRectMap.end(); it++)
    {
        zchxRadarRect exist_obj = it.value();
        //这里可能存在两种情况,两个新目标比较接近,前一次已经更新了,这次只需要用后一个的数据更新他,不在计算预估.
        //将位置对应的经纬度点
        double est_lat = 0.0, est_lon = 0.0;
        if(exist_obj.currentrect().timeofday() >= obj.timeofday())
        {
            est_lat = exist_obj.currentrect().centerlatitude();
            est_lon = exist_obj.currentrect().centerlongitude();
        } else
        {
            //计算当前的时间间隔
            float delta_time = obj.timeofday() - exist_obj.currentrect().timeofday();
            if(delta_time < 0) delta_time += (3600* 24);
            //        qDebug()<<"delte time:"<<delta_time;
            //检查已有点可能的位置
            float est_distance = exist_obj.currentrect().sog()* delta_time;

            distbearTolatlon1(exist_obj.currentrect().centerlatitude(),
                              exist_obj.currentrect().centerlongitude(),
                              est_distance,
                              exist_obj.currentrect().cog(), &est_lat, &est_lon);
        }
        double dis = getDisDeg(obj.centerlatitude(),obj.centerlongitude(), est_lat, est_lon);
        if(dis <= /*distance*/100)
        {
            if(dis < min_dis)
            {
                list.prepend(exist_obj);
                min_dis = dis;
            }
            else if(dis > min_dis)
            {
                continue;
            }
        }
    }

    return list;
}

Latlon ZCHXAnalysisAndSendRadar::getMergeTargetLL(const zchxRadarRectDefList &list)
{
    double sum_x = 0, sum_y = 0;
    foreach (zchxRadarRectDef temp, list) {
        sum_x += temp.centerlongitude();
        sum_y += temp.centerlatitude();
    }
    if(list.size() > 0)
    {
        sum_x = sum_x / list.size();
        sum_y = sum_y / list.size();
    }

    return Latlon(sum_y, sum_x);
}

void ZCHXAnalysisAndSendRadar::changeTargetLL(const Latlon &ll, zchxRadarRectDef &cur)
{
    double dlon = ll.lon - cur.centerlongitude();
    double dlat = ll.lat - cur.centerlatitude();
    cur.set_centerlongitude(ll.lon);
    cur.set_centerlatitude(ll.lat);
    //将目标的所有经纬度坐标进行同步的变换
    cur.set_topleftlatitude(cur.topleftlatitude() + dlat);
    cur.set_topleftlongitude(cur.topleftlongitude() + dlon);
    cur.set_bottomrightlatitude(cur.bottomrightlatitude() + dlat);
    cur.set_bottomrightlongitude(cur.bottomrightlongitude() + dlon);
    for(int i=0; i<cur.blocks_size(); i++)
    {
        singleVideoBlock *block = cur.mutable_blocks(i);
        block->set_latitude(block->latitude() + dlat);
        block->set_longitude(block->longitude() + dlon);
    }
    if(cur.has_startlatitude())
    {
        cur.set_startlatitude(cur.startlatitude() + dlat);
    }
    if(cur.has_startlongitude())
    {
        cur.set_startlongitude(cur.startlongitude() + dlon);
    }

    if(cur.has_endlatitude())
    {
        cur.set_endlatitude(cur.endlatitude() + dlat);
    }
    if(cur.has_endlongitude())
    {
        cur.set_endlongitude(cur.endlongitude() + dlon);
    }
}

bool ZCHXAnalysisAndSendRadar::isDirectionChange(double src, double target)
{
    double delta_a = mDirectionInvertHoldValue;
    //计算原角度对应的相反角度的范围,反映到0-360的范围
    int min = int(src - delta_a);
    int max = int(src + delta_a);
    if(min < 0)
    {
        //原方向指向右上方向,则有效值的范围是min_+360 ~ 360, 0~max
        if((target >= 0  && target < max) || (target > min+360)) return false;
        return true;
    } else if(max > 360)
    {
        //原方向指向左上方向,则有效值的范围是min ~ 360, 0~max-360
        if((target >= 0  && target < max-360) || (target > min)) return false;
        return true;
    } else
    {
        //最大或者最小都在0-360的范围内
        if(target > min && target < max) return false;
        return true;
    }
    return false;
}

//这里的矩形块数据的时间越靠前就排在最后, 也就是说起点在最后
double ZCHXAnalysisAndSendRadar::calAvgCog(const zchxRadarRectDefList &list)
{
    double angle  = 0.0;
    QList<double>    cogList;
#if 1
    if(list.size() > 0)
    {
        angle = list[0].cog();
        cogList.append(angle);
        for(int i=1; i<=list.size()-1; i++)
        {
            if(i == 3) break;
            cogList.append(list[i].cog());
            double cur_angle = list[i].cog();
            double min_angle = cur_angle < angle ? cur_angle : angle;
            double max_angle = cur_angle > angle ? cur_angle : angle;
            double sub_angle = fabs(cur_angle - angle);
            //计算这两个角的角平分线对应的角度作为下一次合成的角度
            if(sub_angle > 180.0)
            {
                sub_angle = 360 - sub_angle;  //取夹角
                angle = max_angle + 0.5 * sub_angle;
            } else
            {
                angle = min_angle + 0.5 * sub_angle;
            }

        }
        qDebug()<<"src cog list:"<<cogList<<" res cog:"<<angle;
    }

#else
    //从起点为原点,计算各个轨迹点对应的直角坐标系的坐标
    QList<Mercator>  pntsList;    
    QList<double>    calCogList;

    QGeoCoordinate p1(list.last().centerlatitude(), list.last().centerlongitude());
    for(int i=list.size()-1; i>=0; i--)
    {
        double cur_lat = list[i].centerlatitude();
        double cur_lon = list[i].centerlongitude();
        //转换为直角坐标
        Mercator cat = latlonToMercator(Latlon(cur_lat, cur_lon));
        pntsList.append(cat);
        cogList.append(list[i].cog());
        if(i == list.size() - 1) continue;
        QGeoCoordinate p2(list[i].centerlatitude(), list[i].centerlongitude());
        calCogList.append(p1.azimuthTo(p2));
    }
    //按起点进行归化处理, 起点就是坐标原点(0, 0), 所有向量合成以后的方向就是目标的平均方向
    Mercator start_pos = pntsList.first();
    double sum_x = 0, sum_y = 0;
    for(int i=0; i<pntsList.size(); i++)
    {
        Mercator& cur = pntsList[i];
        cur.mX = cur.mX + start_pos.mX * (-1);
        cur.mY = cur.mY + start_pos.mY * (-1);
        sum_x += cur.mX;
        sum_y += cur.mY;
    }
    //转换成角度顺时针
    angle = atan2(sum_y * (-1), sum_x) * 180 / GLOB_PI ;
    //转换为正北方向和顺时针
    angle -= 270.0;
    while(angle >= 360 ) angle -= 360;
    while(angle < 0) angle += 360;

    qDebug()<<"src cog list:"<<cogList<<"cal cog list:"<<calCogList<<" res cog:"<<angle;
#endif
    return angle;

}

//通过使用5个点进行判断
int ZCHXAnalysisAndSendRadar::isTargetDirectStable(const zchxRadarRect& rect, int check_point_num, double *avg_cog)
{
    zchxRadarRectDefList list;
    list.append(rect.currentrect());
    for(int i=0; i<rect.historyrects_size()-1; i++) //不取第一个点,因为第一个点的方向未知
    {
        list.append(rect.historyrects(i));
        if(list.size() == check_point_num) break;
    }
    if(list.size() < check_point_num) return TARGET_DIRECTION_UNDEF;
    double pre_cog = list.last().cog();
    int same_num = 0, diff_num = 0;
    QList<double> coglist;
    coglist.append(pre_cog);
    for(int i=list.size()-2; i>=0; i--)
    {
        double cur_cog = list[i].cog();
        if(isDirectionChange(pre_cog, cur_cog))
        {
            diff_num++;
            break;

        } else
        {
            //在同一个方向
            same_num++;
        }
        pre_cog = cur_cog;
    }
    if(diff_num)
    {
        return TARGET_DIRECTION_UNSTABLE;
    }
    if(avg_cog)
    {
        *avg_cog = calAvgCog(list);
    }
    return TARGET_DIRECTION_STABLE;

}

//判断目标当前是否是在一定范围内跳动,方法
//1)目标的点数超过3个点
//2)连续点的方向不统一,每一个目标距离判定的起始点的距离都在合并的目标距离范围之内.
bool ZCHXAnalysisAndSendRadar::isTargetJumping(const zchxRadarRect& rect, double merge_dis, int jump_target_num)
{
//    //目标已经连续,且方向统一,肯定不再跳跃
//    if(isTargetDirectStable(rect, jump_target_num, NULL)) return false;
    //目标的方向不在同一方向,计算目标到起始点的距离
    zchxRadarRectDefList list;
    list.append(rect.currentrect());
    for(int i=0; i<rect.historyrects_size(); i++)
    {
        list.append(rect.historyrects(i));
        if(list.size() == jump_target_num) break;
    }
    if(list.size() < jump_target_num) return false;
    //存在不一致的情况
    //检查距离是不是距离起点都很近,很近就认为目标在乱跳
    double start_lat = list.last().centerlatitude();
    double start_lon = list.last().centerlongitude();
    bool same_target = true;
    for(int i=list.size()-2; i>= 0; i--)
    {
        double cur_lat = list[i].centerlatitude();
        double cur_lon = list[i].centerlongitude();
        double distance = getDisDeg(start_lat, start_lon, cur_lat, cur_lon);
        if(distance > merge_dis)
        {
            same_target = false;
            break;
        }
    }
    if(same_target) return true;
    return false;
}

//发送回波矩形-------------------------目标构造--------------------------------------------------------------------------------------------------
//目标出现连续3次的方向相同,则认为目标的方向和速度确定.目标确定之前,目标的速度和方向值都是0
//这个时候通过距离阈值将目标可能点保存,构造路径
void ZCHXAnalysisAndSendRadar::sendVideoRects(const zchxRadarRectDefList& src_list)
{
   signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "TargetExtraction", "sendVideoRects");
   if(src_list.size() == 0) return;
   qDebug()<<"start process video rects...............";
   zchxRadarRectDefList temp_list(src_list);
   m_radarPointMap_1.clear();
   double list_time = src_list.first().timeofday();

   double target_merge_distance = mTargetMergeRadius;
   double target_min_speed = 3.0;
   double target_max_speed = 14.0;
   //首先合并距离太近的目标, 使之成为一个目标
   mergeRectTargetInDistance(temp_list,target_merge_distance);

   if(m_radarRectMap.size() > 0)
   {
       dumpTargetDistance("old target before update", target_merge_distance);
   }

   //开始确定雷达目标点
   int target_check_num = 4;
   bool cog_avg_adjust = mAdjustCogEnabled;
   if(m_radarRectMap.size() != 0)
   {
       //雷达目标已经存在了,现在开始找可能的下一个目标点
       zchxRadarRectMap::iterator it = m_radarRectMap.begin();
       for(; it != m_radarRectMap.end(); it++)
       {
           int    rect_num = it->currentrect().rectnumber();
           double old_time = it.value().currentrect().timeofday();
           double old_sog = it.value().currentrect().sog();
           double old_lat = it.value().currentrect().centerlatitude();
           double old_lon = it.value().currentrect().centerlongitude();

           //检查目标的态势是否已经确定.目标态势确定的方法就是最近出现了连续三笔的数据的方向大体一致  目标没有回头的情况
           //目标态势确定的情况,已经存在的目标的方向角就是已经经过平均估计的方向角
           //否则就是目标的实际方向角
           double est_cog = it->currentrect().cog();
           //计算当前的时间间隔, 根据时间间隔算预估位置点
           float delta_time = list_time - old_time;

           zchxRadarRectDefList merge_list;
           int cur_est_cnt = it->estcount();
           //这里开始进行位置的预估判断,
           //目标的态势已经确定,则根据的预推位置确定目标,目标没有找到,则将预推位置估计为目标位置
           //目标的态势未确定,则根据距离来判断目标的可能位置
           if(it->dirconfirmed())
           {
               qDebug()<<"current target dir is confirmed."<<rect_num<<" cog:"<<est_cog;
               //计算当前目标可能的预估位置
               double est_lat = 0.0, est_lon = 0.0;
               if(delta_time < 0) delta_time += (3600* 24);
               float est_distance = old_sog * delta_time;
               distbearTolatlon1(old_lat, old_lon, est_distance, est_cog, &est_lat, &est_lon);
               QGeoCoordinate last_pos(old_lat, old_lon);
               //从最新的目标矩形框中寻找预估位置附件的点列
               for(int k = 0; k<temp_list.size();)
               {
                   zchxRadarRectDef next = temp_list[k];                   
                   //计算两者的距离
                   double distance = getDisDeg(est_lat, est_lon, next.centerlatitude(),next.centerlongitude());
                   if(distance < target_merge_distance)
                   {
                       //计算二者的方向角,待选取的点必须在这个点的前方
                       QGeoCoordinate cur_pos(next.centerlatitude(), next.centerlongitude());
                       double cal_cog = last_pos.azimuthTo(cur_pos);
                       if(isDirectionChange(est_cog, cal_cog))
                       {
                           //点跑到当前点的后方去了,与原来的运动方向相反, 不处理.
                           qDebug()<<"found near target but direction not allowed. skip...";
                           k++;
                       } else
                       {
                           //目标存在,从原来的队列里边删除
                           merge_list.append(next);
                           temp_list.removeAt(k);
                       }
                       continue;
                   }
                   k++;
               }
               //目标没有找到的情况,直接构造一个目标
               if(merge_list.size() == 0)
               {
                   qDebug()<<"next target not found, now fake one...."<<rect_num;
                   zchxRadarRectDef fakeData;
                   fakeData.CopyFrom(it->currentrect());
                   fakeData.set_timeofday(list_time);
                   //将目标移动到现在的预推位置
                   changeTargetLL(Latlon(est_lat, est_lon), fakeData);
                   merge_list.append(fakeData);
                   cur_est_cnt++;
               } else
               {
                   qDebug()<<"next target found, with size:"<<merge_list.size();
                   cur_est_cnt = 0;  //目标重新回到了实际的环境中
               }
           } else
           {
               qDebug()<<"current target dir isn't confirmed. find target near old pos."<<rect_num<<" cog:"<<est_cog;
               //这里不计算目标的预估位置,直接计算目标范围之类的点
               //从最新的目标矩形框中寻找预估位置附件的点列
               for(int k = 0; k<temp_list.size();)
               {
                   zchxRadarRectDef next = temp_list[k];
                   //计算两者的距离
                   double distance = getDisDeg(old_lat, old_lon, next.centerlatitude(),next.centerlongitude());
                   if(distance < target_merge_distance)
                   {
                       //目标存在,从原来的队列里边删除
                       merge_list.append(next);
                       temp_list.removeAt(k);
                       continue;
                   }
                   k++;
               }
               qDebug()<<"find target near old pos with size:"<<merge_list.size();
           }
           //没有找到符合要求的目标,目标不更新
           if(merge_list.size() == 0)
           {
               qDebug()<<"next target not found. now next loop"<<rect_num<<" dir confirmed:"<<it->dirconfirmed();
               continue;
           }
           qDebug()<<"target need to be merge with size:"<<merge_list.size();
           //根据找到的目标进行位置更新
           //过滤筛选符合方位角要求的目标
           zchxRadarRectDefList target_list;
           if(it->historyrects_size() == 0)
           {
               //如果目标还是第一个点,则方向角还未确定,则所有目标的均值作为下一个目标
               target_list.append(merge_list);
           } else
           {
               //如果目标的方向角已经存在了,则优先使用和目标方向值大体方向相同的所有目标的均值作为下一个目标,
               //如果相同的不存在,则使用所有的均值作为下一个目标
               QGeoCoordinate last_pos(old_lat, old_lon);
               for(int i=0; i < merge_list.size(); i++)
               {
                   zchxRadarRectDef cur = merge_list[i];
                   QGeoCoordinate cur_pos(cur.centerlatitude(), cur.centerlongitude());
                   double cog = last_pos.azimuthTo(cur_pos);
                   if(!isDirectionChange(est_cog, cog))
                   {
                       target_list.append(cur);
                   }
               }
               if(target_list.size() == 0)
               {
                   target_list.append(merge_list);
               }

           }
           qDebug()<<"finnaly target need to be merge with size:"<<target_list.size();
           //计算目标合并后的中心位置
           Latlon ll = getMergeTargetLL(target_list);
           //计算新目标和就目标之间的距离
           double distance = getDisDeg(old_lat, old_lon, ll.lat, ll.lon);
           if(distance < 1.0)
           {
               //目标的距离太近,认为目标没有移动, 不进行处理
               qDebug()<<"merge target too closed. not update. continue..."<<distance;
               //仅仅更新目标的当前时间,防止目标被删除
               it->mutable_currentrect()->set_timeofday(list_time);
               continue;
           }
           //新目标对应的矩形单元确定
           zchxRadarRectDef target = target_list.first();
           target.set_rectnumber(it.key());
           changeTargetLL(ll, target);
//           if(target.timeofday() == 0)
//           {
//               qDebug()<<"is time_of_day set:"<<target.has_timeofday();
//               for(int i=0; i<target_list.size(); i++)
//               {
//                   qDebug()<<"target timeof day:"<<target_list[i].timeofday();
//               }
//           }

           //确定新目标
           zchxRadarRect total;
           total.set_dirconfirmed(it->dirconfirmed());  //初始化目标是否确认方向
           total.set_estcount(cur_est_cnt);
           total.mutable_currentrect()->CopyFrom(target);
           //旧目标添加历史轨迹点的0号位置
           zchxRadarRectDef *single_Rect = total.add_historyrects();
           single_Rect->CopyFrom(it->currentrect());
           //移动就目标的历史轨迹点到新目表的历史轨迹点.历史轨迹点最大数目为20
           int update_his_size = 20 - total.historyrects_size();
           int k = 0;
           for(int i= 0; i<it->historyrects_size(); i++)
           {
               if(k >= update_his_size) break;
               zchxRadarRectDef *history = total.add_historyrects();
               history->CopyFrom(it->historyrects(i));
               k++;
           }
           //更新速度角度
           if(total.historyrects_size() > 0)
           {
               zchxRadarRectDef* last = total.mutable_historyrects(0);
               QGeoCoordinate last_pos(last->centerlatitude(), last->centerlongitude());
               QGeoCoordinate cur_pos(total.currentrect().centerlatitude(), total.currentrect().centerlongitude());
               total.mutable_currentrect()->set_cog(last_pos.azimuthTo(cur_pos));
               //检查新目标的方向是否与原来确定的目标方向相同
               if(it->dirconfirmed())
               {
                   if(isDirectionChange(it->currentrect().cog(), total.currentrect().cog()))
                   {
                       //目标反方向了, 不再继续更新
                       qDebug()<<"target go to opposite..when update new sog cog  continue yet..";
                       continue;
                   }
               }
               double delta_time = total.currentrect().timeofday() - last->timeofday();
               if(delta_time <= 0) delta_time += (3600 * 24);
               double cal_dis = last_pos.distanceTo(cur_pos);
               total.mutable_currentrect()->set_sog( cal_dis / delta_time);
               if(total.historyrects_size() == 1)
               {
                   //添加第一个历史轨迹点
                   //第一个点的方向还未确定,默认赋值为第二个点的角度
                   last->set_cog(total.currentrect().cog());
               }
               QList<double> history_cog;
               for(int i=0; i<total.historyrects_size(); i++)
               {
                   history_cog.append(total.historyrects(i).cog());
               }
               qDebug()<<"now history cogs:"<<history_cog;


               //           cout<<current.currentrect().rectnumber()<<"dis:"<<cal_dis<<"time_gap:"<<delta_time<<"speed:"<<current.currentrect().sog()<< current.currentrect().timeofday()<< last.timeofday();
           }
           //目标没有跳跃,更新目标的方位角为平均值
           if(!total.dirconfirmed())
           {
               qDebug()<<"now check target is dir stable or not...."<<total.currentrect().cog();
               double avg_cog = 0;
               int target_dir = isTargetDirectStable(total, target_check_num, &avg_cog);
               if( target_dir == TARGET_DIRECTION_STABLE)
               {
                   total.set_dirconfirmed(true);
                   if(cog_avg_adjust)total.mutable_currentrect()->set_cog(avg_cog);
                   qDebug()<<"taregt is stable now. with new cog:"<<total.currentrect().cog();

               } else if(target_dir == TARGET_DIRECTION_UNSTABLE)
               {

                   //现在进行最后的确认.检查目标是否是来回地跳来跳去,如果是,删除跳来跳去的轨迹点,保留最初的点
                   qDebug()<<"now start check new target is jumping or not"<<"  time:"<<total.currentrect().timeofday()<< " size:"<<total.historyrects_size();
                   int jump_num = target_check_num + 1;
                   if(isTargetJumping(total, target_merge_distance, jump_num))
                   {
                       //将目标退回到check_num对应的轨迹点
                       int restore_history_index = jump_num - 2;
                       if(restore_history_index > total.historyrects_size())
                       {
                           //异常情况
                           continue;
                       }
                       zchxRadarRectDef now = total.historyrects(restore_history_index);
                       total.mutable_currentrect()->CopyFrom(now);
                       total.mutable_historyrects()->DeleteSubrange(0, restore_history_index + 1);
                       qDebug()<<"target is jumping... move to old one:"<<total.currentrect().timeofday()<<" size:"<<total.historyrects_size();

                   }
               } else
               {
                   //目标的点列数还不够,难以判断
               }
           } else if(cog_avg_adjust)
           {
               //根据最近的点列的坐标更新当前点的方位角
               zchxRadarRectDefList list;
               list.append(total.currentrect());
               for(int i=0; i<total.historyrects_size(); i++)
               {
                   list.append(total.historyrects(i));
                   if(list.size() == 5) break;
               }
               double avg_cog = calAvgCog(list);
               total.mutable_currentrect()->set_cog(avg_cog);
           }

           if(total.historyrects_size() < target_check_num)
           {
               qDebug()<<"target is dir not stable  ....for not enough history size";
               total.set_dirconfirmed(false);
           }

           //将就目标的历史轨迹清除,只有新目标保存历史轨迹
           it->clear_historyrects();
           //将新目标更新到队列中
           it->CopyFrom(total);
//           qDebug()<<"update exist target:"<<it->currentrect().rectnumber()<<it->currentrect().timeofday()<<target.timeofday();
       }
   }

   checkTargetRectAfterUpdate(target_merge_distance);
   //检查目标之间的距离值
   dumpTargetDistance("old target after update", target_merge_distance);
   //将剩余的矩形目标作为单独的目标添加
   //目标点为空,所有的雷达目标点自动生成初始化的矩形点
   for(int i=0; i<temp_list.size(); i++)
   {
       zchxRadarRect rect;
       rect.set_dirconfirmed(false);
       rect.set_estcount(0);
       rect.mutable_currentrect()->CopyFrom(temp_list[i]);
       //更新编号,没有找到重复的点,直接添加
       if(rectNum > maxNum)
       {
           rectNum = objNum;
       }

       //检查当前是否还存在空闲的
       while (rectNum <= maxNum) {
           if(m_radarRectMap.contains(rectNum)){
               rectNum++;
               continue;
           }
           break;
       }
       rect.mutable_currentrect()->set_rectnumber(rectNum++);
       m_radarRectMap[rect.currentrect().rectnumber()] = rect;
       qDebug()<<"new rect maked now:"<<rect.currentrect().rectnumber()<<rect.currentrect().timeofday();
   }

   checkTargetRectAfterUpdate(target_merge_distance);

   if(0){
       zchxRadarRectDefList list;
       foreach (zchxRadarRect rect, m_radarRectMap) {
           list.append(rect.currentrect());
       }
       exportRectDef2File(list, QString("%1_rect").arg(QDateTime::currentMSecsSinceEpoch()));
   }

   int ClearTrack_Time = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();
   //清理目标,删除超时未更新的目标
   double time_of_day = timeOfDay();
   QList<int> allKeys = m_radarRectMap.keys();
   foreach (int key, allKeys) {
       zchxRadarRect obj = m_radarRectMap[key];
       if((time_of_day - obj.currentrect().timeofday()) > ClearTrack_Time)
       {
           m_radarRectMap.remove(key);
           continue;
       }
       if(obj.estcount() >= 10)
       {
           m_radarRectMap.remove(key);
       }
   }
   //检查目标之间的距离值
   dumpTargetDistance("all target with new ", target_merge_distance);

   //连续存在3笔数据的认为是目标，然后输出
   //目标构造
   foreach (zchxRadarRect rectObj, m_radarRectMap)
   {

       zchxRadarRectDef target;
       target.CopyFrom(rectObj.currentrect());
       TrackPoint trackObj;
       LatLong startLatLong(m_dCentreLon,m_dCentreLat);
       //编号
       trackObj.set_tracknumber(target.rectnumber());
       //速度
       trackObj.set_sog(target.sog());
       //方向
       trackObj.set_cog(target.cog());
       if(!rectObj.dirconfirmed())
       {
           trackObj.set_sog(0.0);
       }

       //经纬度
       trackObj.set_wgs84poslat(target.centerlatitude());
       trackObj.set_wgs84poslong(target.centerlongitude());
       //笛卡尔坐标
       double x,y;
       double lat = trackObj.wgs84poslat();
       double lon = trackObj.wgs84poslong();
       getDxDy(startLatLong, lat, lon, x, y);
       trackObj.set_cartesianposx(x);
       trackObj.set_cartesianposy(y);
       //other
       trackObj.set_timeofday(target.timeofday());
       trackObj.set_systemareacode(0);
       trackObj.set_systemidentificationcode(1);
       trackObj.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
       trackObj.set_tracklastreport(false);
       trackObj.set_cartesiantrkvel_vx(0);
       trackObj.set_cartesiantrkvel_vy(0);
       //直径
       trackObj.set_diameter(target.diameter());
       //完成
       m_radarPointMap_1[trackObj.tracknumber()] = trackObj;
   }
   //根据回波块中心确定目标
   m_radarPointMap = m_radarPointMap_1;


   cout<<"大小-缓存 rect size:"<<m_radarRectMap.size()<<" taregt size:"<<m_radarPointMap.size();

   sendRadarTrack();//小雷达发送目标
   signalVideoRects(m_radarRectMap);//绘制矩形块
   sendRadarRectPixmap();
}

void ZCHXAnalysisAndSendRadar::dumpTargetDistance(const QString &tag, double merge_dis)
{
    qDebug()<<"dump target distance now:"<<tag;
    QList<int> keys = m_radarRectMap.keys();
    for(int i=0; i<keys.size(); i++)
    {
        zchxRadarRect cur = m_radarRectMap[keys[i]];
        double cur_lat = cur.currentrect().centerlatitude();
        double cur_lon = cur.currentrect().centerlongitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            zchxRadarRect next = m_radarRectMap[keys[k]];
            double next_lat = next.currentrect().centerlatitude();
            double next_lon = next.currentrect().centerlongitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().timeofday()<<next.currentrect().timeofday();
            }
        }
    }
}

void ZCHXAnalysisAndSendRadar::checkTargetRectAfterUpdate(double merge_dis)
{
    QList<int> keys = m_radarRectMap.keys();
    for(int i=0; i<keys.size(); i++)
    {
        if(!m_radarRectMap.contains(keys[i])) continue;
        zchxRadarRect cur = m_radarRectMap[keys[i]];
        double cur_lat = cur.currentrect().centerlatitude();
        double cur_lon = cur.currentrect().centerlongitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            if(!m_radarRectMap.contains(keys[k])) continue;
            zchxRadarRect next = m_radarRectMap[keys[k]];
            double next_lat = next.currentrect().centerlatitude();
            double next_lon = next.currentrect().centerlongitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                if(cur.dirconfirmed() ^ next.dirconfirmed())
                {
                    qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" confirmed:"<<cur.dirconfirmed()<<next.dirconfirmed()<<" remove not confirmed one.";

                    if(cur.dirconfirmed() && !next.dirconfirmed())
                    {
                        m_radarRectMap.remove(keys[k]);
                        continue;
                    }
                    if((!cur.dirconfirmed()) && next.dirconfirmed())
                    {
                        m_radarPointMap.remove(keys[i]);
                        break;
                    }
                } else
                {
                    qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().timeofday()<<next.currentrect().timeofday()<<" remove old one.";

                    if(next.currentrect().timeofday() <= cur.currentrect().timeofday())
                    {
                        m_radarRectMap.remove(keys[k]);
                    } else
                    {
                        m_radarPointMap.remove(keys[i]);
                        break;
                    }
                }


            }
        }
    }
}

void ZCHXAnalysisAndSendRadar::mergeRectTargetInDistance(zchxRadarRectDefList &temp_list, int target_merge_distance)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
//  exportRectDef2File(list, QString("%1_origon").arg(now));
//    qDebug()<<"before merge. rect list size:"<<temp_list.size();
    //目标预处理.将距离太近的目标进行合并,合并的中心点取二者的重点
    for(int i=0; i<temp_list.size();)
    {
        zchxRadarRectDef &cur = temp_list[i];
        //获取和当前目标距离较近的目标,目标可能多个,这里暂且设定为list,
        zchxRadarRectDefList merge_list;
        for(int k = i+1; k<temp_list.size();)
        {
            zchxRadarRectDef next = temp_list[k];
            //计算两者的距离
            double distance = getDisDeg(cur.centerlatitude(), cur.centerlongitude(),
                                        next.centerlatitude(),next.centerlongitude());
            if(distance < target_merge_distance)
            {
                //目标存在,从原来的队列里边删除
                merge_list.append(next);
                temp_list.removeAt(k);
                continue;
            }
            k++;
        }
        if(merge_list.size() == 0)
        {
            i++;
            continue;
        }
        //合并目标
        merge_list.append(cur);
        Latlon ll = getMergeTargetLL(merge_list);
        changeTargetLL(ll, cur);
    }
//    qDebug()<<"after merge. rect list size:"<<temp_list.size();
//  exportRectDef2File(temp_list, QString("%1_merge").arg(now));
}

//查找最近的目标,结果第一个元素保存距离最近的目标的信息.如果相等,就传回时间最早的那个目标
zchxTrackPointList ZCHXAnalysisAndSendRadar::findSameObj(const zchxTrackPoint &obj, bool self)
{
    zchxTrackPointList list;
    double min_dis = ULONG_MAX;
    zchxTrackPointMap::iterator it = m_radarPointMap_1.begin();
    for(; it != m_radarPointMap_1.end(); it++)
    {
        zchxTrackPoint point = it.value();
        if((!self) && (point.tracknumber() == obj.tracknumber())) continue;
        double dis = getDisDeg(obj.wgs84poslat(),obj.wgs84poslong(), point.wgs84poslat(), point.wgs84poslong());
        if(dis <= mTargetMergeRadius)
        {
            if(dis < min_dis)
            {
                list.prepend(point);
                min_dis = dis;
            } else if(dis > min_dis)
            {
                continue;
            } else
            {
                if(list.size() > 0 && point.timeofday() < list.first().timeofday())
                {
                    list.prepend(point);
                }
            }
        }
    }
    return list;
}

//发送目标2
void ZCHXAnalysisAndSendRadar::sendTrackSlot2(const zchxTrackPointList &list)
{
    /*
    //先找中心点
    TrackPointList list0;
    TrackPointList list1 = list;
    TrackPointList list2;
    foreach (TrackPoint obj, list)
    {
        list2.clear();
        foreach (TrackPoint obj1, list1)
        {
            TrackPoint point = obj1;
            double dis = getDisDeg(obj.wgs84poslat(),obj.wgs84poslong(), point.wgs84poslat(), point.wgs84poslong());
            //if(dis == 0) continue;
            if(dis <= distance)
            {
                list2.prepend(point);
            }
        }
        if(list2.size()==1)
        {
            list0.append(obj);
        }
        else
        {
            QList<QGeoCoordinate> mQgList;
            TrackPoint objTemp;
            foreach (TrackPoint obj2, list2)
            {
                QGeoCoordinate latlon(obj2.wgs84poslat(),obj.wgs84poslong());
                mQgList.append(latlon);
                objTemp = obj2;
            }
            QGeoCoordinate mGeoCoordinate = GetCenterPointFromListOfCoordinates(mQgList);
            objTemp.set_wgs84poslat(mGeoCoordinate.latitude());
            objTemp.set_wgs84poslong(mGeoCoordinate.longitude());
            //判断是否有距离过近的目标
            bool flag = 1;
            foreach (TrackPoint obj0, list0)
            {
                double dis = getDisDeg(obj0.wgs84poslat(),obj0.wgs84poslong(), objTemp.wgs84poslat(), objTemp.wgs84poslong());
                if(dis <= distance)
                {
                    flag = 0;
                    break;
                }
            }
            if(flag)
            {
                //cout<<"添加";
                list0.append(objTemp);
            }
        }
    }
    //cout<<"合并前"<<list.size()<<"合并后"<<list0.size();
    LOG(LOG_RTM, "%s %s %d,处理目标 = %d", __FILE__, __FUNCTION__,__LINE__, list.size());
    //cout<<"发送目标2";
    //static int nplan = 1; //重新赋值编号
    LatLong startLatLong(m_dCentreLon,m_dCentreLat);
    //cout<<"nplan"<<nplan;
    nplan++;
    if(list0.size() < 1)return;
    foreach (TrackPoint obj, list0)
    {
        //更新时间
        QDateTime curDateTime = QDateTime::currentDateTime();
        QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                      curDateTime.date().day()),QTime(0, 0));
        float time_of_day = startDateTime.secsTo(curDateTime);
        obj.set_timeofday(time_of_day);
        if(objNum>maxNum)
        {
            bh = 0;
            objNum = 1+(radar_num-1)*10000;
        }
        //标记目标的数据信息来自的序列号,借用system_area_code
        //obj.set_systemareacode(index);
        //cout<<"时间"<<obj.timeofday();
        double x,y;
        double lat = obj.wgs84poslat();
        double lon = obj.wgs84poslong();
        getDxDy(startLatLong, lat, lon, x, y);
        obj.set_cartesianposx(x);
        obj.set_cartesianposy(y);

        //obj.set_sog(0);
        //检查有没有位置重复的目标, 注意这里重复的目标返回最多1个
        QList<ITF_Track_point> pnt_list = findSameObj(obj);
        //cout<<"pnt_list"<<pnt_list.size()<<nplan;
        if(pnt_list.size() == 0)
        {
            //没有找到重复的点,直接添加
            if(nplan > 20)
            {
                objNum = 1+(m_uSourceID-1)*10000;
                nplan = 1;
            }
            if(nplan > 19)continue;
            while(m_radarPointMap_1.find(objNum) != m_radarPointMap_1.end())
            {
                objNum++;
            }
            obj.set_tracknumber(objNum);
            objNum++;
        } else
        {
            //找到重复的点
            //1)先从原来的map中将找出的重复点删除
//            foreach (ITF_Track_point pnt, pnt_list) {
//                m_radarPointMap_1.remove(pnt.tracknumber());
//                mPushNumMap.remove(pnt.tracknumber());
//            }

            //2)决定重复点为第一个点
            ITF_Track_point ref = pnt_list.first();
            //3)重复点的信息赋值给当前的目标
            obj.set_tracknumber(ref.tracknumber());
            //cout<<"ref.tracks().tracks_size()"<<ref.tracks().tracks_size();
            //m_radarPointMap_1[obj.tracknumber()] = ref;
            com::zhichenhaixin::proto::RadarHistoryTracks *signle_track0 = ref.mutable_tracks();
            //检查当前的目标历史轨迹数是否超出了设定的值,如果超出了,移除第一个历史轨迹点
            int start_obj_track_index = 0;
            if(signle_track0->track_size() > hIniNum)
            {
                start_obj_track_index = 1;
            }
            for(int i = start_obj_track_index; i < signle_track0->track_size(); i++)
            {
                //依次进一,移除第一个位置,方便更新最新点迹到第五个位置
                com::zhichenhaixin::proto::RadarHistoryTrack *signle_track = obj.mutable_tracks()->add_track();
                signle_track->set_tracknumber(signle_track0->track(i).tracknumber());
                signle_track->set_wgs84poslat(signle_track0->track(i).wgs84poslat());
                signle_track->set_wgs84poslong(signle_track0->track(i).wgs84poslong());
                signle_track->set_timeofday(signle_track0->track(i).timeofday());
                signle_track->set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(0));
                //signle_track->set_tracklastreport(0);
                signle_track->set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(1));
                signle_track->set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(1));
                signle_track->set_cog(signle_track0->track(i).cog());
                signle_track->set_sog(signle_track0->track(i).sog());
                signle_track->set_utc(signle_track0->track(i).utc());
            }
            //速度确认
            //cout<<"速度不为0"<<obj.sog();
            if(1)//ref.sog() != 0
            {
                double dis = getDisDeg(obj.wgs84poslat(),obj.wgs84poslong(), ref.wgs84poslat(), ref.wgs84poslong());
                int time = qAbs(obj.timeofday() - ref.timeofday());
                int msog = (dis / time) * 3.6 / 1.852;
                if(time != 0)
                {
                    obj.set_sog(((dis / time) * 3.6 / 1.852)); //节
                }
                if(time == 0)
                {
                    obj.set_sog(ref.sog());
                }
                //4)判断是否距离大于10米，大于的话丢掉
                if(dis > m_jupmdis)
                {
                    obj = ref;
                    obj.set_timeofday(time_of_day);
                }
                cout<<"时间"<<time<<"距离"<<dis<<"速度"<<obj.sog();
            }
        }
        float angle = 0.0;
        //采用和第一个位置计算方向,再取平均值,减小误差
        if(obj.tracks().track_size())
        {
            double fx,fy;
            double dlat = obj.tracks().tracks(0).wgs84poslat();
            double dlon = obj.tracks().tracks(0).wgs84poslong();
            LatLong startLatLong(m_dCentreLon,m_dCentreLat);
            getDxDy(startLatLong, dlat, dlon, fx, fy);
            int len_x = obj.cartesianposx() - fx;
            int len_y = obj.cartesianposy() - fy;
            //double tan_yx = (std::abs(len_y)) / (std::abs(len_x));
            double aby = std::abs(len_y);
            double abx = std::abs(len_x);
            double tan_yx = aby/abx;
            //cout<<"tan_yx"<<tan_yx;
            if(len_y > 0 && len_x < 0)
            {
                angle =270 + atan(tan_yx)*180/M_PI;
                //cout<<"distance"<<distance<<"angle"<<angle;
            }
            else if (len_y > 0 && len_x > 0)
            {
                angle =90- atan(tan_yx)*180/M_PI;
                //cout<<"distance"<<distance<<"angle"<<angle;
            }
            else if(len_y < 0 && len_x < 0)
            {
                angle =180+90- atan(tan_yx)*180/M_PI;
                //cout<<"distance"<<distance<<"angle"<<angle;
            }
            else if(len_y < 0 && len_x > 0)
            {
                angle = 90 + atan(tan_yx)*180/M_PI;
                // cout<<"distance"<<distance<<"angle"<<angle;
            }
            else if(len_x == 0 && len_y > 0)
            {
                angle = 0;
                // cout<<"distance"<<distance<<"angle"<<angle;
            }
            else if(len_x == 0 && len_y < 0)
            {
                angle = 180;
                //cout<<"distance"<<distance<<"angle"<<angle;
            }
            else if(len_y == 0 && len_x > 0)
            {
                angle = 90;
                //cout<<"distance"<<distance<<"angle"<<angle;
            }
            else if(len_y == 0 && len_x < 0)
            {
                angle = 270;
                //cout<<"distance"<<distance<<"angle"<<angle;
            }

        }
        obj.set_cog(angle);
        //先判断是否有历史轨迹,有的话是否超过5笔
        qint64 utc = QDateTime::currentMSecsSinceEpoch();
        int hisNum = obj.tracks().tracks_size();
        com::zhichenhaixin::proto::RadarHistoryTracks *signle_track1 = obj.mutable_tracks();
        //方法1:取最新的连续N笔数据来计算平均方向
        //每次只保存后面N笔数据,移除最久远的一笔
        //雷达轨迹
        com::zhichenhaixin::proto::RadarHistoryTrack *signle_track = 0;
        //当前的雷达历史轨迹数超出了最大的设定数
        if(hisNum == 0)
        {
            //历史轨迹还不存在,直接添加;
            //signle_track = signle_track1->add_tracks();
            signle_track = obj.mutable_tracks()->add_tracks();
        } else{
            //历史轨迹点已经存在,检查与上一个点的位置是否相同,相同就不添加,否则就添加
            com::zhichenhaixin::proto::RadarHistoryTrack pre_track = signle_track1->tracks(hisNum - 1);
            if(obj.wgs84poslat() != pre_track.wgs84poslat() && obj.wgs84poslong() != pre_track.wgs84poslong())
            {
                //signle_track = signle_track1->add_tracks();
                signle_track = obj.mutable_tracks()->add_tracks();
            }
        }
        if(signle_track)
        {
            signle_track->set_tracknumber(obj.tracknumber());
            signle_track->set_wgs84poslat(obj.wgs84poslat());
            signle_track->set_wgs84poslong(obj.wgs84poslong());
            signle_track->set_timeofday(obj.timeofday());
            signle_track->set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(0));
            signle_track->set_tracklastreport(0);
            signle_track->set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(1));
            signle_track->set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(1));
            signle_track->set_cog(obj.cog());
            signle_track->set_sog(obj.sog());
            signle_track->set_utc(utc);
            //cout<<"添加";
        }

        //方法2:通过遍历历史轨迹,算出平均方向,或者首尾2点的方向
        double mCog = 0;
        for(int i = 0; i < obj.tracks().tracks_size()-1; i++)
        {
            mCog += obj.tracks().tracks(i).cog();
        }
        if(obj.tracks().tracks_size() > 0)
        mCog = mCog/obj.tracks().tracks_size();
        obj.set_cog(mCog);
        m_radarPointMap_1[obj.tracknumber()] = obj;

    }
    m_radarPointMap = m_radarPointMap_1;
    //自我排查
     foreach (ITF_Track_point obj, m_radarPointMap)
     {
         foreach (ITF_Track_point obj1, m_radarPointMap) {
             if(obj1.tracknumber() > obj.tracknumber())
             {
                 double dis = getDisDeg(obj.wgs84poslat(),obj.wgs84poslong(), obj1.wgs84poslat(), obj1.wgs84poslong());
                 if(dis <= distance)
                 {
                     m_radarPointMap_1.remove(obj1.tracknumber());
                     obj.set_wgs84poslat((obj.wgs84poslat()+obj1.wgs84poslat())/2);
                     obj.set_wgs84poslong((obj.wgs84poslong()+obj1.wgs84poslong())/2);
                     obj.set_cartesianposx((obj.cartesianposx()+obj1.cartesianposx())/2);
                     obj.set_cartesianposy((obj.cartesianposy()+obj1.cartesianposy())/2);
                 }
             }
         }
     }
     m_radarPointMap = m_radarPointMap_1;
    //浮标设置
    foreach (ITF_Track_point point, m_radarPointMap_1) {
        //与先前设定好的浮标位置作比较
        bool fFlag = 0;
        foreach (QStringList mList, fMap) {
            QString num = mList.first();
            double mLon = num.toDouble();
            num = mList.back();
            double mLat = num.toDouble();
            double dis = getDisDeg(point.wgs84poslat(),point.wgs84poslong(),mLat,mLon);
            if(dis < floatRange)
            {
                cout<<"找到浮标"<<dis<<point.tracknumber();
                fFlag = 1;
            }
        }
        if(1 == fFlag)
        {
            cout<<"移除浮标"<<point.tracknumber();
            m_radarPointMap_1.remove(point.tracknumber());
            fFlag = 0;
        }
    }
    clearRadarTrack();
    sendRadarTrack();
    LOG(LOG_RTM, "%s %s %d,准备发送目标:通道 = %d", __FILE__, __FUNCTION__,__LINE__,m_uSourceID);
    */
}

void ZCHXAnalysisAndSendRadar::readRadarLimitFormat()
{
    QString path = QCoreApplication::applicationDirPath();
    QString pathName;
    QRegExp na("(\/)(\\w)+(\\.)(json)"); //初始化名称结果
    QString name("");
    if(na.indexIn(m_limit_file) != -1)
    {
        //匹配成功
        name = na.cap(0);
        cout<<"name"<<name;
    }
    cout<<"打印区域文件地址";
    //pathName = m_limit_file;
    pathName = path+name;
    cout<<"地址pathName"<< pathName;
    m_landPolygon.clear();
    m_seaPolygon.clear();
    analysisLonLatTOPolygon(pathName,m_landPolygon,m_seaPolygon);
    cout<<"m_landPolygon"<<m_landPolygon.size()<<"m_seaPolygon"<<m_seaPolygon.size();
    //test
}

void ZCHXAnalysisAndSendRadar::processVideoData(bool rotate)
{
    if (m_radarVideoMap.isEmpty()) {
        return;
    }

    //使用开线程
    if (m_DrawRadarVideo == NULL) {
        m_DrawRadarVideo = new ZCHXDrawRadarVideo(m_uSourceID);
        m_DrawRadarVideo->setLimit(m_bLimit);
        m_DrawRadarVideo->setLimitArea(m_landPolygon,m_seaPolygon);
        m_DrawRadarVideo->setDistance(m_distance);
        m_DrawRadarVideo->setAfterglowType(m_uLoopNum);
        m_DrawRadarVideo->setStradar(str_radar);

        connect(m_DrawRadarVideo,SIGNAL(signalRadarAfterglowPixmap(int,QPixmap,QPixmap,QPixmap)),
                this,SLOT(setRadarAfterglowPixmap(int,QPixmap,QPixmap,QPixmap)));
        connect(m_DrawRadarVideo,SIGNAL(signalRadarVideoAndTargetPixmap(QPixmap,Afterglow)),
                this,SIGNAL(signalRadarVideoAndTargetPixmap(QPixmap,Afterglow)));
         //设置画笔像素大小
        connect(this, SIGNAL(set_pen_width(int)), m_DrawRadarVideo, SLOT(slotSetPenwidth(int)));
        //画根据回波块识别出来的目标
        //connect(this,SIGNAL(signalTrackList(QMap<int,QList<TrackNode> >)),m_DrawRadarVideo,SLOT(slotTrackMap(QMap<int,QList<TrackNode> >)));
        //回波颜色设置
        connect(this,SIGNAL(colorSetSignal(int,int,int,int,int,int)),m_DrawRadarVideo,SLOT(slotSetColor(int,int,int,int,int,int)));
        //显示回波块最边上的点
        connect(this,SIGNAL(signalShowTheLastPot(QList<QPointF>,QList<QPointF>)),m_DrawRadarVideo,SLOT(showTheLastPot(QList<QPointF>,QList<QPointF>)));
        //处理矩形回波块
        connect(m_DrawRadarVideo,SIGNAL(signalSendRects(zchxRadarRectDefList)), this,SLOT(sendVideoRects(zchxRadarRectDefList)));
        //绘制发送的历史回波块
        connect(this,SIGNAL(signalVideoRects(zchxRadarRectMap)),m_DrawRadarVideo,SLOT(getRects(zchxRadarRectMap)));
    }

    bool bProcessing = m_DrawRadarVideo->getIsProcessing();
    finish_flag = bProcessing;
    if(bProcessing) {
        qDebug()<<"draw video still process.......... abnormal found";
        return;
    }

    Afterglow objAfterglow;
    objAfterglow.m_RadarVideo = m_radarVideoMap;
    objAfterglow.m_path = m_latLonVec;
    objAfterglow.m_pathCartesian = m_latLonVecCartesian;

    //qDebug()<<"track size"<<m_latLonVec.size();

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("proces video data to pixmap");
    //emit signalSendRecvedContent(utc,"VIDEO_PROCESS",sContent);

    if(0)
    {
        cout<<"----发送繁忙----"<<send_finish<<rect_finish<<track_finish;
        return;
    }
    emit m_DrawRadarVideo->signalDrawAfterglow(m_dRadius, objAfterglow, m_radarPointMap_draw, rotate);
//    if(m_radarRectMap.size() > 0)
//    {
//        send_finish = false;
//        rect_finish = false;
//        track_finish = false;
//    }
}

void ZCHXAnalysisAndSendRadar::analysisCat253Radar(QByteArray ba)//解析新雷达控制数据
{
    cout<<"雷达控制数据"<<ba.size()<<endl<<ba;
}

void ZCHXAnalysisAndSendRadar::analysisCat020Radar(QByteArray ba)//解析新雷达目标数据
{
    cout<<"解析新雷达目标数据020"<<ba;
    // m_radarPointMap.clear();
    track_finish = false;
    const unsigned char* data = (const unsigned char*)(ba.constData());
    //cout<<*data<<sizeof(data)<<sizeof(*data) ;
    int i = 0;
    while(i<30)
    {
        //cout<<"data["<<i<<"]"<<data[i];
        i++;
    }
    if('\n' != data[0])
        return;
    com::zhichenhaixin::proto::TrackPoint trackPoint;
    trackPoint.set_wgs84poslat(0);
    trackPoint.set_wgs84poslong(0);
    trackPoint.set_cartesianposx(0);
    trackPoint.set_cartesianposy(0);
    trackPoint.set_systemareacode(0);
    trackPoint.set_systemidentificationcode(1);
    trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
    trackPoint.set_tracknumber(0);
    trackPoint.set_timeofday(0);
    trackPoint.set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(0));
    trackPoint.set_tracklastreport(0);
    trackPoint.set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(1));
    trackPoint.set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(1));
    trackPoint.set_sigmax(0);
    trackPoint.set_sigmay(0);
    trackPoint.set_sigmaxy(0);
    trackPoint.set_ampofpriplot(0);
    trackPoint.set_cartesiantrkvel_vx(0);
    trackPoint.set_cartesiantrkvel_vy(0);
    trackPoint.set_cog(0);
    trackPoint.set_sog(0);

    trackPoint.set_systemareacode(data[5]);
    trackPoint.set_systemidentificationcode(data[6]);

    int tempValue = (data[7]);
    cout<<"航迹状态tempValue"<<tempValue;
    trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(tempValue));
    int tempTrackNumber = data[29] *256 + data[28] ;
//  if(pDB->getItemValue(i, strValue, "161", "TrkNb"))
    trackPoint.set_tracknumber(tempTrackNumber);
    //cout<<"TrkNb"<<(int)data[28]<<tempTrackNumber;
    double tempWgs84PosLat = 0.0;
    double tempWgs84PosLong = 0.0;
    //if(pDB->getItemValue(i, strValue, "041", "Lat"))
    tempWgs84PosLat = (data[16]+data[17]*256+data[18]*256*256+data[19]*256*256*256)/10000000.000000f;
    trackPoint.set_wgs84poslat(tempWgs84PosLat);
    //if(pDB->getItemValue(i, strValue, "041", "Lon"))
    tempWgs84PosLong = (data[20]+data[21]*256+data[22]*256*256+data[23]*256*256*256)/10000000.000000f;
    trackPoint.set_wgs84poslong(tempWgs84PosLong);
    //cout<<"16,20"<<tempWgs84PosLat<<tempWgs84PosLong;
    //没有042
    double dcarx = 0, dcary = 0;
    LatLong startLatLong(m_dCentreLon,m_dCentreLat);
    getDxDy(startLatLong, tempWgs84PosLat, tempWgs84PosLong, dcarx, dcary);
     //cout<<"经纬度转笛卡尔 x y"<<x<<y;
    float tempCartesianPosX = dcarx;
    float tempCartesianPosY = dcary;
    trackPoint.set_cartesianposx(tempCartesianPosX);
    trackPoint.set_cartesianposy(tempCartesianPosY);
//    //cout<<"笛卡尔转经纬度 x y"<<x<<y;
//    void getNewLatLong(LatLong &A, double &lat, double &lng, double &x, double &y) ;
//    double lat,lng;
//    getNewLatLong (startLatLong, lat, lng, dcarx, dcary);
//    cout<<"笛卡尔转经纬度 lat lng"<<lat<<lng<<tempWgs84PosLat<<tempWgs84PosLong;
    int time = data[11]*256*256 + data[10]*256 + data[9];
    trackPoint.set_timeofday(time/128);
    //cout<<"时间:"<<trackPoint.timeofday();
    double cog = 0.0;
    double sog = 0.0;
    sog = (data[25] * 256 + data[24]);//速度
    cog = (data[27] * 256 + data[26])*0.0055;//方向角
    cout<<"200的速度字段"<<data[25]<<data[24]<<data[25]*256+data[24];
    trackPoint.set_sog(sog);//速度
    trackPoint.set_cog(cog);//方向角
//    //test
//    trackPoint.set_wgs84poslat(31.9635);
//    trackPoint.set_wgs84poslong(118.615);
    //cout<<"速度方位"<<cog<<sog;
    //cout<<"040RHO"<<data[12]<<"040THETA"<<data[14];
    m_radarPointMap[tempTrackNumber] = trackPoint;
    qDebug()<<"经度:" <<  QString::number(trackPoint.wgs84poslat(), 'f',7) << " \n";
    qDebug()<<"经度:" <<  QString::number(trackPoint.wgs84poslong(), 'f',7) << " \n";
    qDebug()<<"轨迹点信息如下:"<< "  \n"
           << "系统区域代码: "<< trackPoint.systemareacode() << "  \n"
           << "系统识别代码 :" << trackPoint.systemidentificationcode() << "  \n"
           << "消息类型  :" << trackPoint.messagetype() << "  \n"
           << "航迹号  :" << tempTrackNumber << "  \n"
           << "经度 :" << trackPoint.wgs84poslong() <<" \n"
           << "纬度 :" << trackPoint.wgs84poslat() <<" \n"
           << "当日时间 :" << trackPoint.timeofday() << " \n"
           << "航迹状态 :" << trackPoint.tracktype() << " \n"
           << "当前目标最后一次上报 :" << trackPoint.tracklastreport() << " \n"
           << "外推法 :" << trackPoint.extrapolation() << " \n"
           << "位置来历 :" << trackPoint.trackpositioncode() << " \n"
           << "x轴标准差 :" << trackPoint.sigmax() << " \n"
           << "y轴标准差 :" << trackPoint.sigmay() << " \n"
           << "2轴平方方差 :" << trackPoint.sigmaxy() << " \n"
           << "震荡波强度检测 :" << trackPoint.ampofpriplot() << " \n"
           << "方位角 :" << cog << " \n"
           << "速度 :" << sog << " \n";
    //m_latLonVec = latLonVec;
    cout<<"发送_4"<<m_radarPointMap.size();
    //clearRadarTrack();
    sendRadarTrack();
}

void ZCHXAnalysisAndSendRadar::analysisCat010Radar(DataBlock *pDB)
{
    //cout<<"解析010目标"<<sizeof(*pDB);
    m_radarPointMap.clear();
    track_finish = false;

    int uNum = pDB->m_lDataRecords.size();
    qDebug()<<"uNum"<<uNum;
    if(uNum<=0)
    {
        return;
    }

    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);

    std::string strValue;

    //std::vector<std::pair<double, double>> latLonVec;
    //latLonVec.clear();
    for(int i=0; i< uNum; i++)
    {
        QJsonObject objObject;
        com::zhichenhaixin::proto::TrackPoint trackPoint;
        if(pDB->getItemValue(i, strValue, "010", "SAC"))
        {
            trackPoint.set_systemareacode(atoi(strValue.c_str()));
            //objObject.insert("SAC",atoi(strValue.c_str()));
        }

        if(pDB->getItemValue(i, strValue, "010", "SIC"))
        {
            trackPoint.set_systemidentificationcode(atoi(strValue.c_str()));
            //objObject.insert("SIC",atoi(strValue.c_str()));
        }

        if(pDB->getItemValue(i, strValue, "001", "MsgTyp"))
        {
            int tempValue = atoi(strValue.c_str());
            trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(tempValue));
            //objObject.insert("MsgTyp",tempValue);
        }

        int tempTrackNumber = 0 ;
        if(pDB->getItemValue(i, strValue, "161", "TrkNb"))
        {
            tempTrackNumber = atoi(strValue.c_str());
            //cout<<"strValue.c_str() %02x"<<str;
        }
        trackPoint.set_tracknumber(tempTrackNumber);
        //objObject.insert("TrkNb",tempTrackNumber);

        float tempCartesianPosX = 0.0;
        float tempCartesianPosY = 0.0;
        if(pDB->getItemValue(i, strValue, "042", "X"))
        {
            tempCartesianPosX = atof(strValue.c_str());
        }
        if(pDB->getItemValue(i, strValue, "042", "Y"))
        {
            tempCartesianPosY = atof(strValue.c_str());
        }
        trackPoint.set_cartesianposx(tempCartesianPosX);
        trackPoint.set_cartesianposy(tempCartesianPosY);
        //objObject.insert("X",tempCartesianPosX);
        //objObject.insert("Y",tempCartesianPosY);

        double tempWgs84PosLat = 0.0;
        double tempWgs84PosLong = 0.0;
        //        if(pDB->getItemValue(i, strValue, "041", "Lat"))
        //        {
        //            tempWgs84PosLat = atof(strValue.c_str());
        //        }
        //        if(pDB->getItemValue(i, strValue, "041", "Lon"))
        //        {
        //            tempWgs84PosLong = atof(strValue.c_str());
        //        }

        double dTempLon = m_dCentreLon;
        double dTempLat = m_dCentreLat;

        //        if(0 == tempWgs84PosLong || 0 == tempWgs84PosLat)
        //        {
        convertXYtoWGS(dTempLat,dTempLon,tempCartesianPosX, tempCartesianPosY, &tempWgs84PosLat ,&tempWgs84PosLong);
        //ZCHXLOG_DEBUG("convertXYtoWGS");
        //}

        trackPoint.set_wgs84poslat(tempWgs84PosLat);
        trackPoint.set_wgs84poslong(tempWgs84PosLong);
        //objObject.insert("Lat",tempWgs84PosLat);
        //objObject.insert("Lon",tempWgs84PosLong);
        //std::pair<double, double> latLonPair(tempWgs84PosLat,tempWgs84PosLong);
        //latLonVec.push_back(latLonPair);
        if(pDB->getItemValue(i, strValue, "140", "ToD"))
        {
            //cout<<"当日时间，使用接收到的时间"<<atof(strValue.c_str());
            trackPoint.set_timeofday(atof(strValue.c_str()));
            //objObject.insert("ToD",atof(strValue.c_str()));
        }
        //当日时间，使用接收到的时间
        //trackPoint.set_timeofday(time_of_day);

        if(pDB->getItemValue(i, strValue, "170", "CNF"))
        {
            int tempValue = atoi(strValue.c_str());
            trackPoint.set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(tempValue));
            //objObject.insert("CNF",tempValue);
        }

        int tempTrackLastReport = 0;
        if(pDB->getItemValue(i, strValue, "170", "TRE"))
        {
            tempTrackLastReport = atoi(strValue.c_str());
        }
        trackPoint.set_tracklastreport(tempTrackLastReport);
        //objObject.insert("TRE",tempTrackLastReport);

        if(pDB->getItemValue(i, strValue, "170", "CST"))
        {
            int tempValue = atoi(strValue.c_str());;
            trackPoint.set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(tempValue));
            //objObject.insert("CST",tempValue);
        }

        if(pDB->getItemValue(i, strValue, "170", "STH"))
        {
            int tempValue = atoi(strValue.c_str());;
            trackPoint.set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(tempValue));
            //objObject.insert("STH",tempValue);
        }

        if(pDB->getItemValue(i, strValue, "500", "Qx"))
        {
            trackPoint.set_sigmax(atof(strValue.c_str()));
            //objObject.insert("Qx",atof(strValue.c_str()));
        }
        if(pDB->getItemValue(i, strValue, "500", "Qy"))
        {
            trackPoint.set_sigmay(atof(strValue.c_str()));
            //objObject.insert("Qy",atof(strValue.c_str()));
        }
        if(pDB->getItemValue(i, strValue, "500", "Qxy"))
        {
            trackPoint.set_sigmaxy(atof(strValue.c_str()));
            //objObject.insert("Qxy",atof(strValue.c_str()));
        }

        if(pDB->getItemValue(i, strValue, "131", "PAM"))
        {
            trackPoint.set_ampofpriplot(atof(strValue.c_str()));
            //objObject.insert("PAM",atof(strValue.c_str()));
        }

        double tempCartesianTrkVel_vx = 0.0;
        double tempCartesianTrkVel_vy = 0.0;
        double cog = 0.0;
        double sog = 0.0;
        if(pDB->getItemValue(i, strValue, "202", "Vx"))
        {
            tempCartesianTrkVel_vx =  atof(strValue.c_str());
        }
        if(pDB->getItemValue(i, strValue, "202", "Vy"))
        {
            tempCartesianTrkVel_vy =  atof(strValue.c_str());
        }
        trackPoint.set_cartesiantrkvel_vx(tempCartesianTrkVel_vx);
        trackPoint.set_cartesiantrkvel_vy(tempCartesianTrkVel_vy);
        //objObject.insert("Vx",tempCartesianTrkVel_vx);
        //objObject.insert("Vy",tempCartesianTrkVel_vy);
        cog = calCog(tempCartesianTrkVel_vx,tempCartesianTrkVel_vy);
        sog = calSog(tempCartesianTrkVel_vx,tempCartesianTrkVel_vy);
        trackPoint.set_cog(cog);
        trackPoint.set_sog(sog);
//        //test
//        trackPoint.set_wgs84poslat(31.9635);
//        trackPoint.set_wgs84poslong(118.615);
        //objObject.insert("Cog",cog);
        //objObject.insert("Sog",sog);
        double dcarx = 0, dcary = 0;
        LatLong startLatLong(m_dCentreLon,m_dCentreLat);
        getDxDy(startLatLong, tempWgs84PosLat, tempWgs84PosLong, dcarx, dcary);
         //cout<<"经纬度转笛卡尔 x y"<<x<<y;
        float tempPosX = dcarx;
        float tempPosY = dcary;
        trackPoint.set_cartesianposx(tempPosX);
        trackPoint.set_cartesianposy(tempPosY);
        m_radarPointMap[tempTrackNumber] = trackPoint;

//            qDebug()<<"轨迹点信息如下:"<< "  \n"
//                   << "系统区域代码: "<< trackPoint.systemareacode() << "  \n"
//                   << "系统识别代码 :" << trackPoint.systemidentificationcode() << "  \n"
//                   << "消息类型  :" << trackPoint.messagetype() << "  \n"
//                   << "航迹号  :" << tempTrackNumber << "  \n"
//                   << "笛卡尔坐标计算X位置 :" << tempCartesianPosX << " \n"
//                   << "笛卡尔坐标计算Y位置 :" << tempCartesianPosY << " \n"
//                   << "经度 :" << trackPoint.wgs84poslat()  << " \n"
//                   << "纬度 :" << trackPoint.wgs84poslong() << " \n"
//                   << "当日时间 :" << trackPoint.timeofday() << " \n"
//                   << "航迹状态 :" << trackPoint.tracktype() << " \n"
//                   << "当前目标最后一次上报 :" << trackPoint.tracklastreport() << " \n"
//                   << "外推法 :" << trackPoint.extrapolation() << " \n"
//                   << "位置来历 :" << trackPoint.trackpositioncode() << " \n"
//                   << "x轴标准差 :" << trackPoint.sigmax() << " \n"
//                   << "y轴标准差 :" << trackPoint.sigmay() << " \n"
//                   << "2轴平方方差 :" << trackPoint.sigmaxy() << " \n"
//                   << "震荡波强度检测 :" << trackPoint.ampofpriplot() << " \n"
//                   << "迪卡尔坐标航迹计算x速度(米/秒) :" << tempCartesianTrkVel_vx << " \n"
//                   << "迪卡尔坐标航迹计算y速度(米/秒) :" << tempCartesianTrkVel_vy << " \n"
//                   << "方位角 :" << cog << " \n"
//                   << "速度 :" << sog << " \n";
    }
    //m_latLonVec = latLonVec;
    //cout<<"发送_3"<<m_radarPointMap.size();
    //clearRadarTrack();
    sendRadarTrack();
}

void ZCHXAnalysisAndSendRadar::analysisCat240Radar(DataBlock *pDB, int uLineNum, int uCellNum, int uHeading)
{
    //cout<<"analysisCat240Radar";
    std::string strValue;
    int uNum = pDB->m_lDataRecords.size();
    //cout<<"uNum"<<uNum;
    if(uNum<=0)
    {
        return;
    }
    for(int i=0; i<uNum; i++)
    {

        int messageType = 0;
        if (pDB->getItemValue(i, strValue, "000", "MsgTyp"))
        {
            //ZCHXLOG_INFO("messageType: " << strValue);
            messageType = atoi(strValue.c_str());

        }

        int systemAreaCode = 0 ;
        if(pDB->getItemValue(i, strValue, "010", "SAC"))
        {
            //ZCHXLOG_INFO("systemAreaCode: " << strValue);
            systemAreaCode = atoi(strValue.c_str());
        }

        int systemIdentificationCode = 0;
        if(pDB->getItemValue(i, strValue, "010", "SIC"))
        {
            //ZCHXLOG_INFO("systemIdentificationCode: " << strValue);
            systemIdentificationCode = atoi(strValue.c_str());
            //cout<<"bits-8/1(SIC)			系统唯一识别码"<<systemIdentificationCode;
        }


        int msgIndex = 0; // 消息唯一序列号
        if(pDB->getItemValue(i, strValue, "020", "MSG_INDEX"))
        {
            //ZCHXLOG_INFO("msgIndex: " << strValue);
            msgIndex = atoi(strValue.c_str());
        }


        int rep = 0;
        if (pDB->getItemValue(i, strValue, "030", "REP"))
        {
            //ZCHXLOG_INFO("rep: " << strValue);
            rep = atoi(strValue.c_str());
            //cout<<"没有视频摘要"<<rep;
        }


        float start_az = 0;  // 方向角起始位置
        if(pDB->getItemValue(i, strValue, "040", "START_AZ"))
        {
            //ZCHXLOG_INFO("方向角起始位置: " << strValue);
            start_az = atof(strValue.c_str());
        }

        float end_az = 0;    // 方向角结束位置
        if(pDB->getItemValue(i, strValue, "040", "END_AZ"))
        {
            //ZCHXLOG_INFO("方向角结束位置: " << strValue);
            //cout<<"方向角结束位置.c_str()"<<strValue.data();
            end_az = atof(strValue.c_str());
        }

        unsigned long  start_rg = 0 ;  // 开始区域号
        if(pDB->getItemValue(i, strValue, "040", "START_RG"))
        {
            //ZCHXLOG_INFO("start_rg: " << strValue);
            start_rg = atol(strValue.c_str());
            //cout<<"开始距离"<<start_rg;
        }

        double  cell_dur = 0.0 ;  // 持续时间
        if(pDB->getItemValue(i, strValue, "040", "CELL_DUR"))
        {
            //ZCHXLOG_INFO("cell_dur: " << strValue);
            cell_dur = atof(strValue.c_str());
        }

        int  bit_resolution = 0;  //视频分辨率 默认值为4
        if(pDB->getItemValue(i, strValue, "048", "RES"))
        {
            //ZCHXLOG_INFO("bit_resolution: " << strValue);
            bit_resolution = atoi(strValue.c_str());
            //cout<<"视频分辨率"<<strValue.data();
        }
        if (bit_resolution ==0)
        {
            bit_resolution = 4;
        }

        int time_of_day = 0 ;
        if(pDB->getItemValue(i, strValue, "140", "ToD"))
        {
            //ZCHXLOG_INFO("time_of_day: " << strValue);

            //cout<<"time_of_day"<<strValue.data();
            time_of_day = std::round(atoi(strValue.c_str())/128); // 1/128 s
        }
        if (time_of_day == 0)
        {
            //ZCHXLOG_DEBUG("...... time_of_day == 0....");
            //            boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
            //            boost::posix_time::time_duration td = now.time_of_day();
            //            time_of_day = td.total_seconds();
            time_of_day = QDateTime::currentMSecsSinceEpoch();
            //cout<<"time_of_day"<<time_of_day;
        }
        double range_factor = CELL_RANGE(cell_dur);
//                qDebug()<<"扇区频谱信息如下:"<< "  \n"
//                       << "系统区域代码: "<< systemAreaCode << "  \n"
//                       << "系统识别代码: " << systemIdentificationCode << "  \n"
//                       << "消息唯一序列号: " << msgIndex << " \n"
//                       << "消息类型: " << messageType << "  \n"
//                       <<"消息转发次数: " <<rep<<"\n"
//                      << "方向角起始位置: " << start_az << "  \n"
//                      << "方向角结束位置: " << end_az << " \n"
//                      << "扫描起始距离: " << start_rg << " \n"
//                      << "持续时间: " << cell_dur  << " \n"
//                      << "距离因子: " << range_factor  << " \n"
//                      << "视频分辨率: " << bit_resolution << " \n"
//                      << "当日时间: " << time_of_day << " \n"
//                         ;

        int numRadials = end_az - start_az;
        //高容积052
        if(pDB->getItemBinary(i, strValue, "052"))
        {
            //cout<<"高容积052";
            //ZCHXLOG_DEBUG("config.range_cell: "<< config.range_cell);
            unsigned long int nDestLen = numRadials *uCellNum;
            // unsigned long int nDestLen = 2048;
            //cout<<"nDestLen"<<nDestLen<<numRadials<<uCellNum;
            unsigned char* pDest = new unsigned char[nDestLen];

            unsigned char* pSrcBuf = (unsigned char*)strValue.data()+1;
            unsigned long int  nSrcLen = strValue.length()-1;
            //cout<<"相关参数:"<<nDestLen<<nSrcLen;
            if (0 != pDest)
            {
                int ret = uncompress(pDest, &nDestLen, pSrcBuf, nSrcLen);
                //cout<<"ret"<<ret;//ret == Z_OK
                if (ret == Z_OK)
                {
                    // ================调试16进制的内容: 开始=====================
//                    int  body_length = numRadials *uCellNum +1;
//                    cout<<"body_length:" << body_length;
//                    char converted[111849];
//                    int i;
//                    for(i=0;i<nDestLen;i++)
//                    {
//                      sprintf(&converted[i*2], "%02X", pDest[i]);
//                    }
//                    std::string str(converted);
//                    //cout<< "hex:" <<   str ;
                    // ================调试16进制的内容: 结束=====================

                    int position = 0;
                    for (int line = 0; line < numRadials; line++)
                    {
                        RADAR_VIDEO_DATA objVideoData;
                        objVideoData.m_uSourceID = m_uSourceID;
                        objVideoData.m_uSystemAreaCode = systemAreaCode;
                        objVideoData.m_uSystemIdentificationCode = systemIdentificationCode;
                        objVideoData.m_uMsgIndex = msgIndex;
                        objVideoData.m_uAzimuth = start_az+line;
                        objVideoData.m_dStartRange = start_rg*range_factor;
                        objVideoData.m_dRangeFactor = range_factor;
                        objVideoData.m_uTotalNum = uCellNum;
                        objVideoData.m_dCentreLon = m_dCentreLon;
                        objVideoData.m_dCentreLat = m_dCentreLat;
                        objVideoData.m_uLineNum = uLineNum;
                        objVideoData.m_uHeading = uHeading;
                        QList<int> AmplitudeList;
                        QList<int> pIndexList;
                        AmplitudeList.clear();
                        pIndexList.clear();
                        for (int range = 0; range < uCellNum; range++)
                        {
                            int value =  (int)(pDest[position]);
                            //videoFrame.add_amplitude(pDest[position]);
                            if(value>0)
                            {
                                //cout<<"position"<<position<<"value"<<value<<"range"<<range;
                                AmplitudeList.append(value);
                                pIndexList.append(range);
                            }
                            position++;
                        }
                        objVideoData.m_pAmplitude = AmplitudeList;
                        objVideoData.m_pIndex = pIndexList;

                        //半径
                        m_dRadius = objVideoData.m_dStartRange+objVideoData.m_dRangeFactor*uCellNum;

//                                                qDebug()<<"AgilTrack 视频帧信息如下:"<< "  \n"
//                                                << "系统区域代码: "<< objVideoData.m_uSystemAreaCode << "  \n"
//                                                << "系统识别代码: "<< objVideoData.m_uSystemIdentificationCode << "  \n"
//                                                << "消息唯一序列号 : "<< objVideoData.m_uMsgIndex << "  \n"
//                                                << "扫描方位: "<< objVideoData.m_uAzimuth << "  \n"
//                                                << "扫描起始距离: "<< objVideoData.m_dStartRange << "  \n"
//                                                << "距离因子 :" << objVideoData.m_dRangeFactor << "  \n"
//                                                << "一条线上点个数  :" << objVideoData.m_uTotalNum << "  \n"
//                                                << "总共线的个数  :" << objVideoData.m_uLineNum << "  \n"
//                                                << "中心纬度  :" << objVideoData.m_dCentreLat << "  \n"
//                                                << "中心经度  :" << objVideoData.m_dCentreLon << "  \n"
//                                                ;

                        m_radarVideoMap[objVideoData.m_uAzimuth] = objVideoData;
                        //序列化
                        //                        std::string pstring;
                        //                        videoFrame.SerializeToString(&pstring);
                        //                        message->push_back(pstring.c_str(), pstring.length());
                    }
                    delete[]pDest;
                    //ZCHXLOG_DEBUG("encode finish:" << " position:" << position );
                }
                else
                {
                    qDebug()<<"uncompress is failed";
                    delete[]pDest;
                    return;
                }
            }
        }
        //中容积051
        else if(pDB->getItemBinary(i, strValue, "051"))
        {
                if (1)//ret == 0
                {
                    int position = 0;
                        RADAR_VIDEO_DATA objVideoData;
                        objVideoData.m_uSourceID = m_uSourceID;
                        objVideoData.m_uSystemAreaCode = systemAreaCode;
                        objVideoData.m_uSystemIdentificationCode = systemIdentificationCode;
                        objVideoData.m_uMsgIndex = msgIndex;
                        objVideoData.m_uAzimuth = end_az;
                        objVideoData.m_dStartRange = start_rg*range_factor;
                        objVideoData.m_dRangeFactor = range_factor;
                        objVideoData.m_uTotalNum = uCellNum;
                        objVideoData.m_dCentreLon = m_dCentreLon;
                        objVideoData.m_dCentreLat = m_dCentreLat;
                        objVideoData.m_uLineNum = uLineNum;
                        objVideoData.m_uHeading = uHeading;
                        QList<int> AmplitudeList;
                        QList<int> pIndexList;
                        AmplitudeList.clear();
                        pIndexList.clear();
                        for (int range = 0; range < uCellNum; range++)
                        {
                            int value =  (int)strValue.at(position+1);

                            //videoFrame.add_amplitude(pDest[position]);
                            if(value>0)
                            {
                                //cout<<"value"<<value;
                                //cout<<"position"<<position<<"value"<<value<<"range"<<range;
                                AmplitudeList.append(value);
                                pIndexList.append(range);
                            }
                            position++;
                        }
                        objVideoData.m_pAmplitude = AmplitudeList;
                        objVideoData.m_pIndex = pIndexList;
                        //半径
                        m_dRadius = objVideoData.m_dStartRange+objVideoData.m_dRangeFactor*uCellNum;


                        m_radarVideoMap[objVideoData.m_uAzimuth] = objVideoData;
                }
        }
    }
    if(track_finish == true)
        processVideoData();
}
//发送合并的雷达目标
void ZCHXAnalysisAndSendRadar::slotSendComTracks(const zchxTrackPointMap& mp)
{
    //通过zmq发送
    //cout<<"通过zmq发送";
    if(m_uSourceID != 1)return;//只有通道1发送
    //LOG(LOG_RTM, "%s %s %d,通过zmq发送目标通道 = %d", __FILE__, __FUNCTION__,__LINE__,m_uSourceID);
    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uTrackSendPort);

    QString sTopic = m_sTrackTopic;
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();

    //zmq_bind(m_pTrackLisher, sIPport.toLatin1().data());//
    zmq_send(m_pTrackLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pTrackLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    //整体发送
    int uNum = mp.size();
    QString sNum = QString::number(uNum);
    QByteArray sNumArray = sNum.toUtf8();
    zmq_send(m_pTrackLisher, sNumArray.data(), sNumArray.size(), ZMQ_SNDMORE);
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    RadarSurfaceTrack totalRadarSurfaceTrack;
    totalRadarSurfaceTrack.set_flag(1);
    totalRadarSurfaceTrack.set_length(uNum);
    totalRadarSurfaceTrack.set_sourceid("240");
    totalRadarSurfaceTrack.set_utc(utc);
    foreach (com::zhichenhaixin::proto::TrackPoint pnt, mp) {
        com::zhichenhaixin::proto::TrackPoint objRadarPoint = pnt;
        TrackPoint *mRadarSurfaceTrack = totalRadarSurfaceTrack.add_trackpoints();
        mRadarSurfaceTrack->CopyFrom(objRadarPoint);
    }
    QByteArray totalData;//整体发送
    totalData.resize(totalRadarSurfaceTrack.ByteSize());
    totalRadarSurfaceTrack.SerializeToArray(totalData.data(),totalData.size());
    zmq_send(m_pTrackLisher, totalData.data(), totalData.size(), 0);

    //cout<< "雷达目标笛卡尔坐标集合" <<m_latLonVecCartesian.size();
    QString sContent = tr("send analysis radar track data,num = %1").arg(uNum);
    emit signalSendRecvedContent(utc,"TRACK________SEND",sContent);
    //LOG(LOG_RTM, "%s %s %d,通过zmq发送目标通道完成 = %d", __FILE__, __FUNCTION__,__LINE__,m_uSourceID);
    qDebug()<<QDateTime::currentDateTime()<<" radar point size:"<<uNum;
    track_finish = true;
}

void ZCHXAnalysisAndSendRadar::sendRadarTrack()
{
    //cout<<"开始-----------------------------------------------------------";
    //LOG(LOG_RTM, "%s %s %d,sendRadarTrack:通道 = %d", __FILE__, __FUNCTION__,__LINE__,m_uSourceID);
    if(m_radarPointMap.size()<=0)
    {
        //LOG(LOG_RTM, "目标小于0不推送= %d",m_uSourceID);
        cout<<"m_radarPointMap小于0";
        track_finish = true;
        return;
    }
    QMap<int,com::zhichenhaixin::proto::TrackPoint>::iterator itor = m_radarPointMap.begin();
    std::vector<std::pair<double, double>> latLonVec;
    std::vector<std::pair<float, float>> latLonVecCartesian;
    latLonVec.clear();
    latLonVecCartesian.clear();
    QVector<com::zhichenhaixin::proto::TrackPoint> filterRadarPointVec;
    filterRadarPointVec.clear();
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    //先过滤
    for (itor; itor != m_radarPointMap.end(); itor++) {
//        if(itor.value().tracknumber() == 47)
//        cout<< QString("经度: ") <<  QString::number(itor.value().wgs84poslat(), 'f',10)<< QString("纬度: ") <<  QString::number(itor.value().wgs84poslong(), 'f',10);

        com::zhichenhaixin::proto::TrackPoint objRadarPoint = itor.value();

        if(m_bLimit) {
            if(!inLimitAreaForTrack(objRadarPoint.wgs84poslat(), objRadarPoint.wgs84poslong())) {
                //cout<<"经纬度-"<<objRadarPoint.wgs84poslat()<<objRadarPoint.wgs84poslong()<<"笛卡尔-"<<objRadarPoint.cartesianposx()<<objRadarPoint.cartesianposy();
                continue;
            }
        }
        std::pair<double, double> latLonPair(objRadarPoint.wgs84poslat(), objRadarPoint.wgs84poslong());
        latLonVec.push_back(latLonPair);

        std::pair<double, double> latLonPairCartesian(objRadarPoint.cartesianposx(), objRadarPoint.cartesianposy());
        latLonVecCartesian.push_back(latLonPairCartesian);

        filterRadarPointVec.append(objRadarPoint);
    }
    //cout<<"结束-----------------------------------------------------------";
    int uFilterNum = filterRadarPointVec.size();

    emit show_video(m_radarPointMap.size(), uFilterNum);

    //整体发送
    for(int i = 0; i < uFilterNum; i++) {
        com::zhichenhaixin::proto::TrackPoint objRadarPoint = filterRadarPointVec[i];
        m_radarPointMap_3[objRadarPoint.tracknumber()] = objRadarPoint;
    }

    m_radarPointMap_draw = m_radarPointMap_3;
    m_radarPointMap_3.clear();
    signalCombineTrackc(m_radarPointMap_draw);//合并双通道目标
    track_finish = true;
    //LOG(LOG_RTM, "%s %s %d,sendRadarTrack:通道 = %d", __FILE__, __FUNCTION__,__LINE__,m_uSourceID);
}

void ZCHXAnalysisAndSendRadar::clearRadarTrack()
{
    //间隔一定时间清理回波数据
    int nInterval = m_clearRadarTrackTime;//秒
    //int nInterval = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();
    //cout<<"m_clearRadarTrackTime"<<m_clearRadarTrackTime;
    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);

    QMap<int,com::zhichenhaixin::proto::TrackPoint>::iterator itor = m_radarPointMap_1.begin();
    //qDebug()<<"ITF_RadarPoint num "<<m_radarPointMap.size();
    for(itor; itor != m_radarPointMap_1.end();) {
        com::zhichenhaixin::proto::TrackPoint trackPoint = itor.value();
        int uKey = itor.key();
        //qDebug()<<"time_of_day"<<time_of_day;
        //qDebug()<<"objRadarPoint.tracknumber()"<<objRadarPoint.tracknumber();
        // qDebug()<<"objRadarPoint.timeofday()"<<objRadarPoint.timeofday();
        if(time_of_day - trackPoint.timeofday() > nInterval) {
            //qDebug()<<"remove"<<trackPoint.tracknumber();
            itor = m_radarPointMap_1.erase(itor);
           //out<<"remove"<<time_of_day - trackPoint.timeofday() << nInterval;
        } else {
//            qDebug()<<"间隔一定时间清理回波数据" << uKey
//                   << "系统区域代码: "<< trackPoint.systemareacode() << "  \n"
//                   << "系统识别代码 :" << trackPoint.systemidentificationcode() << "  \n"
//                   << "消息类型  :" << trackPoint.messagetype() << "  \n"
//                   << "航迹号  :" << trackPoint.tracknumber() << " "
//                   << "笛卡尔坐标计算X位置 :" << trackPoint.cartesianposx() << " "
//                   << "笛卡尔坐标计算Y位置 :" << trackPoint.cartesianposy() << " "
//                   << "经度 :" << trackPoint.wgs84poslong()  << " "
//                   << "纬度 :" << trackPoint.wgs84poslat() << " "
//                   << "当日时间 :" << trackPoint.timeofday() << " ";
//                   << "航迹状态 :" << trackPoint.tracktype() << " \n"
//                   << "当前目标最后一次上报 :" << trackPoint.tracklastreport() << " \n"
//                   << "外推法 :" << trackPoint.extrapolation() << " \n"
//                   << "位置来历 :" << trackPoint.trackpositioncode() << " \n"
//                   << "x轴标准差 :" << trackPoint.sigmax() << " \n"
//                   << "y轴标准差 :" << trackPoint.sigmay() << " \n"
//                   << "2轴平方方差 :" << trackPoint.sigmaxy() << " \n"
//                   << "震荡波强度检测 :" << trackPoint.ampofpriplot() << " \n"
//                   << "迪卡尔坐标航迹计算x速度(米/秒) :" << trackPoint.cartesiantrkvel_vx() << " \n"
//                   << "迪卡尔坐标航迹计算y速度(米/秒) :" <<  trackPoint.cartesiantrkvel_vy() << " \n"
//                   << "方位角 :" << trackPoint.cog() << " \n"
//                   << "速度 :" << trackPoint.sog() << " \n";
            itor++;
        }
    }
}

//从区域文件读取限制区域
void ZCHXAnalysisAndSendRadar::analysisLonLatTOPolygon(const QString sFileName, QList<QPolygonF> &landPolygon, QList<QPolygonF> &seaPolygon)
{
    cout<<"从区域文件读取限制区域"<<sFileName;
    if(sFileName.isEmpty())
    {
        return;
    }
    landPolygon.clear();
    seaPolygon.clear();
    //qDebug()<<"filepath"<<sFileName;
    QFile objFile(sFileName);
    if(!objFile.open(QIODevice::ReadOnly))
    {
        return;
    }
    QString sAllData = "";
    while (!objFile.atEnd())
    {
        QByteArray LineArray = objFile.readLine();
        QString str(LineArray);
        str = str.trimmed();
        sAllData +=str;
    }
    //cout<<"sAllData"<<sAllData;


    QJsonParseError err;
    QJsonDocument docRcv = QJsonDocument::fromJson(sAllData.toLatin1(), &err);

    if(err.error != QJsonParseError::NoError)
    {
        cout<<"parse completetion list error:"<<err.error;
        return ;
    }
    if(!docRcv.isObject())
    {
        cout<<" status statistics list with wrong format.";
        return ;
    }
    cout<<"区域限制";
    QJsonArray objSeaArray = docRcv.object().value("watercourse").toArray();
    QJsonArray objLandArray1 = docRcv.object().value("land1").toArray();
    QJsonArray objLandArray2 = docRcv.object().value("land2").toArray();
    cout<<"objSeaArray.size():--------"<<objSeaArray.size();
    cout<<"objLandArray1.size():--------"<<objLandArray1.size();
    cout<<"objLandArray2.size():--------"<<objLandArray2.size();
    QVector<QPointF> pointVec;
    for(int i = 0; i < objSeaArray.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objSeaArray.at(i).toArray();
        cout<<"objSeaArray.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        seaPolygon.append(objPolygon);
    }

    for(int i = 0; i < objLandArray1.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray1.at(i).toArray();
        cout<<"objLandArray1.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        landPolygon.append(objPolygon);
    }

    for(int i = 0; i < objLandArray2.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray2.at(i).toArray();
        cout<<"objLandArray2.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        landPolygon.append(objPolygon);
    }
    //陆地3
    QJsonArray objLandArray3 = docRcv.object().value("land3").toArray();
     cout<<"objLandArray3.size():--------"<<objLandArray3.size();
    for(int i = 0; i < objLandArray3.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray3.at(i).toArray();
        cout<<"objLandArray3.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        landPolygon.append(objPolygon);
    }
    cout<<"landPolygon.size()"<<landPolygon.size();

}

//区域限制功能
bool ZCHXAnalysisAndSendRadar::inLimitAreaForTrack(const double dLat, const double dLon)
{
    bool bOk = false;
    QPointF pos(dLon,dLat);
    if(m_seaPolygon.count()<=0&&m_landPolygon.count()<=0)
    {
        return true;//没有限制区域
        //cout<<"没有限制区域";
    }
    if(m_seaPolygon.count()>0 && m_seaPolygon.first().size())
    {
        //cout<<"有海限制区域"<<m_seaPolygon.first().size()<<m_seaPolygon.first();
        for(int uIndex = 0;uIndex<m_seaPolygon.count();uIndex++)
        {
            const QPolygonF curSeaPolygonF = m_seaPolygon[uIndex];
            if(curSeaPolygonF.containsPoint(pos,Qt::OddEvenFill))
            {
                //return true;
                bool bInLand = false;
                for(int i = 0;i<m_landPolygon.count();i++)
                {
                    const QPolygonF curLandPolygonF = m_landPolygon[i];
                    if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
                    {
                        bInLand = true;
                        break;
                    }
                }
                bOk = !bInLand;
                return bOk;
            }
        }
    }
    else
    {
        //cout<<"无海限制区域"<<m_seaPolygon.count()<<m_landPolygon.count();
        bool bInLand = false;
        for(int i = 0;i<m_landPolygon.count();i++)
        {
            const QPolygonF curLandPolygonF = m_landPolygon[i];
            if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
            {
                bInLand = true;
                break;
            }
        }
        bOk = !bInLand;
        return bOk;
    }

    return false;
}

bool ZCHXAnalysisAndSendRadar::inLimitArea(const double dCentreLat, const double dCentreLon, const double dAzimuth, const int uPosition, const double dStartRange, const double dRangeFactor)
{
    bool bOk = false;

    double dDis = dStartRange+uPosition*dRangeFactor;
    double dLon;
    double dLat;
    distbearTolatlon(dCentreLat,dCentreLon,dDis,dAzimuth,dLat,dLon);
    QPointF pos(dLon,dLat);

    for(int uIndex = 0;uIndex<m_seaPolygon.count();uIndex++)
    {
        const QPolygonF curSeaPolygonF = m_seaPolygon[uIndex];
        if(curSeaPolygonF.containsPoint(pos,Qt::OddEvenFill))
        {
            //return true;
            bool bInLand = false;
            for(int i = 0;i<m_landPolygon.count();i++)
            {
                const QPolygonF curLandPolygonF = m_landPolygon[i];
                if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
                {
                    bInLand = true;
                    break;
                }
            }
            bOk = !bInLand;
            return bOk;
        }
    }
    return false;
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
    cout<<"显示编号"<<flag;
    if(m_DrawRadarVideo != NULL)
    m_DrawRadarVideo->SignalShowTrackNum(flag);
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
    if (!m_DrawRadarVideo) {
        return;
    }
    m_DrawRadarVideo->slotTrackMap(map,m_uSourceID);
    //signalCombineVideo(map,m_uSourceID);
}
//绘制2个通道的回波
void ZCHXAnalysisAndSendRadar::slotDrawCombinVideo(QList<TrackNode> mList)
{
     if (m_DrawRadarVideo == NULL) {
        return;
    }
    m_DrawRadarVideo->drawCombineVideo(mList);
}
