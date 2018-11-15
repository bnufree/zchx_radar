#include "zchxanalysisandsendradar.h"
#include "BR24.hpp"
#include <QDebug>
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
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
extern "C"

{

#include "ctrl.h"

}
static uint8_t BR24MARK[] = {0x00, 0x44, 0x0d, 0x0e};
ZCHXAnalysisAndSendRadar::ZCHXAnalysisAndSendRadar(int uSourceID, QObject *parent)
    : QObject(parent),
      mRangeFactor(13.0),
      m_uSourceID(uSourceID)
{
    d_32 = 0;//初始化每次推给周老师库的个数
    QString str_radar = QString("Radar_%1").arg(m_uSourceID);
    m_limit_file = Utils::Profiles::instance()->value(str_radar,"Limet_File").toString();//读取限制区域文件
    m_dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble();
    m_uLoopNum = Utils::Profiles::instance()->value(str_radar,"Loop_Num").toInt();
    m_bLimit = Utils::Profiles::instance()->value(str_radar,"Limit").toBool();
    m_uVideoSendPort = Utils::Profiles::instance()->value("Radar","video_Send_Port").toInt();
    m_sVideoTopic = Utils::Profiles::instance()->value("Radar","video_Topic").toString();
    m_uTrackSendPort = Utils::Profiles::instance()->value("Radar","Track_Send_Port").toInt();
    m_sTrackTopic = Utils::Profiles::instance()->value("Radar","Track_Topic").toString();

    m_distance = Utils::Profiles::instance()->value(str_radar,"Distance").toDouble();
    m_clearRadarTrackTime = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();

    m_DrawRadarVideo = NULL;

    m_pGetTrackProcess = new ZCHXGetTrackProcess(m_dCentreLat,m_dCentreLon);
    connect(m_pGetTrackProcess,SIGNAL(sendTrack(int,ITF_Track_point)),
            this,SLOT(sendTrackSlot(int,ITF_Track_point)));
    connect(this,SIGNAL(startTrackProcessSignal(SAzmData)),
            m_pGetTrackProcess,SIGNAL(getTrackProcessSignal(SAzmData)));
    connect(m_pGetTrackProcess, SIGNAL(sendTrack(TrackObjList)),
            this, SLOT(sendTrackSlot(TrackObjList)));
    connect(this, SIGNAL(startTrackProcessSignal(SAzmDataList)),
            m_pGetTrackProcess, SIGNAL(getTrackProcessSignal(SAzmDataList)));

    //发送回波和余辉的zmq
    //创建context，zmq的socket 需要在context上进行创建
    m_pVideoContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pVideoLisher= zmq_socket(m_pVideoContext, ZMQ_PUB);
    QString videoAddr = "tcp://*:";
    videoAddr += QString::number(m_uVideoSendPort);
    zmq_bind(m_pVideoLisher, videoAddr.toLatin1().data());//

    //发送雷达目标的zmq
    //创建context，zmq的socket 需要在context上进行创建
    m_pTrackContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pTrackLisher= zmq_socket(m_pTrackContext, ZMQ_PUB);
    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uTrackSendPort);
    zmq_bind(m_pTrackLisher, sIPport.toLatin1().data());//

    //监听回波和余辉zmq
    QString monitorVideoUrl = "inproc://monitor.radarVideoclient";
    zmq_socket_monitor (m_pVideoLisher, monitorVideoUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pVideoMonitorThread = new ZmqMonitorThread(m_pVideoContext, monitorVideoUrl, 0);
    connect(m_pVideoMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pVideoMonitorThread, SIGNAL(finished()), m_pVideoMonitorThread, SLOT(deleteLater()));
    m_pVideoMonitorThread->start();

    //监听雷达目标zmq
    QString monitorTrackUrl = "inproc://monitor.radarTrackclient";
    zmq_socket_monitor (m_pTrackLisher, monitorTrackUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pTrackMonitorThread = new ZmqMonitorThread(m_pTrackContext, monitorTrackUrl, 0);
    connect(m_pTrackMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pTrackMonitorThread, SIGNAL(finished()), m_pTrackMonitorThread, SLOT(deleteLater()));
    m_pTrackMonitorThread->start();

    readRadarLimitFormat();//读取限制区域

    //初始新科雷达解析库的调用
    QString sAppPath = QApplication::applicationDirPath();
    m_sPath = sAppPath+"/AsterixSpecification/asterix.ini";
    std::string str = m_sPath.toStdString();
    const char* asterixDefinitionsFile = str.data();
    //qDebug()<<"path"<<asterixDefinitionsFile;
    m_pCAsterixFormat = new CAsterixFormat(asterixDefinitionsFile);
    m_pCAsterixFormatDescriptor = dynamic_cast<CAsterixFormatDescriptor*>(m_pCAsterixFormat->CreateFormatDescriptor(0, ""));


    connect(this, SIGNAL(analysisLowranceRadarSignal(QByteArray,int,int,int)),
            this, SLOT(analysisLowranceRadarSlot(QByteArray,int,int,int)));
    connect(this, SIGNAL(analysisCatRadarSignal(QByteArray,int,int,int,QString)),
            this, SLOT(analysisCatRadarSlot(QByteArray,int,int,int,QString)));
    moveToThread(&m_workThread);
    m_workThread.start();

    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    m_pTimer->start(2000);
    m_pTimer_1 = new QTimer(this);
    connect(m_pTimer_1, SIGNAL(timeout()), this, SLOT(handleTimeout_1()));
    m_pTimer_1->start(100);



}

ZCHXAnalysisAndSendRadar::~ZCHXAnalysisAndSendRadar()
{
    if(m_workThread.isRunning())
    {
        m_workThread.quit();
    }
    m_workThread.terminate();
    zmq_close(m_pVideoLisher);
    zmq_ctx_destroy(m_pVideoContext);

    zmq_close(m_pTrackLisher);
    zmq_ctx_destroy(m_pTrackContext);
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

void ZCHXAnalysisAndSendRadar::analysisLowranceRadarSlot(const QByteArray &sRadarData, int uLineNum, int uCellNum, int uHeading)
{

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("process Lowrance radar ");
    //emit signalSendRecvedContent(utc,"LOWRANCE",sContent);

    //cout<<sRadarData.size();
    const char *buf = sRadarData.constData();
    int len = sRadarData.size();
    //qDebug()<<"len:"<<len;
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
        qDebug()<<"此包没有32条扫描线，丢包！";
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
//    QString uu = QDateTime::currentDateTime().toString("yy-MM-dd hh-mm-ss");
//    cout<<"接收-原始数据-雷达目标时间:" <<time_of_day;
//    static int tt = 0;
//    tt++;
//    cout<<tt<<"------------------------";


    //std::vector<std::pair<double, double>> latLonVec;
    //latLonVec.clear();

    QList<int> AmplitudeList;
    QList<int> pIndexList;
    struct SAzmData sAzmData;
    //std::list<TrackInfo> trackList;
    SAzmDataList srcDataList;
    for (int scanline = 0; scanline < scanlines_in_packet; scanline++)
    {
        QDateTime curDateTime_1 = QDateTime::currentDateTime();
        QDateTime startDateTime_1(QDate(curDateTime_1.date().year(),curDateTime_1.date().month(),
                                      curDateTime_1.date().day()),QTime(0, 0));
        int time_of_day_1 = startDateTime_1.secsTo(curDateTime_1);//当前时间

        BR24::Constants::radar_line *line = &packet->line[scanline];

        // Validate the spoke
        int spoke = line->common.scan_number[0] | (line->common.scan_number[1] << 8);

        m_ri.spokes++;
        //cout<<spoke;

        if (line->common.headerLen != 0x18)
        {
            qDebug()<<"strange header length:" << line->common.headerLen;
            // Do not draw something with this...
            qDebug()<<"该"<< scanline << "扫描线头长度不是24字节，丢包！";
            m_ri.missing_spokes++;//1_没有扫描线
            m_next_spoke = (spoke + 1) % SPOKES;
            QString str =QString("--丢失的扫描线--编号spoke:%1, 线头common.headerLen:%2, tatus:%3, scan_number[0]:%4"
                                 ", scan_number[1]:%5, u00[0]:%6, u00[1]:%7, u00[2]:%8, u00[3]:%9"
                                 ",angle[0]:%10, angle[1]:%11, heading[0]:%12, heading[1]:%13, TIME:%14")
                    .arg(spoke).arg(line->common.headerLen).arg(line->common.status)
                    .arg(line->common.scan_number[0]).arg(line->common.scan_number[1])
                    .arg(line->common.u00[0]).arg(line->common.u00[1]).arg(line->common.u00[2]).arg(line->common.u00[3])
                    .arg(line->common.angle[0]).arg(line->common.angle[1])
                    .arg(line->common.heading[0]).arg(line->common.heading[1]).arg(QString::number(time_of_day_1));
            emit show_missing_spokes(str);
            emit show_received_spokes(str);
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
                cout << "m_ri.missing_spokes_1:"<<m_ri.missing_spokes<<"m_next_spoke"<<m_next_spoke;
                QString str =QString("--当前的扫描线--编号spoke:%1, 线头common.headerLen:%2, tatus:%3, scan_number[0]:%4"
                                     ", scan_number[1]:%5, u00[0]:%6, u00[1]:%7, u00[2]:%8, u00[3]:%9"
                                     ",angle[0]:%10, angle[1]:%11, heading[0]:%12, heading[1]:%13  丢失了%14条扫描线,正常扫描线编号:%15")
                        .arg(spoke).arg(line->common.headerLen).arg(line->common.status)
                        .arg(line->common.scan_number[0]).arg(line->common.scan_number[1])
                        .arg(line->common.u00[0]).arg(line->common.u00[1]).arg(line->common.u00[2]).arg(line->common.u00[3])
                        .arg(line->common.angle[0]).arg(line->common.angle[1])
                        .arg(line->common.heading[0]).arg(line->common.heading[1])
                        .arg(m_ri.missing_spokes).arg(m_next_spoke);
                emit show_missing_spokes(str);
                //emit show_received_spokes(str);
                cout<<"打印missing_spokes";
              } else {
                m_ri.missing_spokes += SPOKES + spoke - m_next_spoke;
                cout << "m_ri.missing_spokes_2:"<<m_ri.missing_spokes;
                QString str =QString("--当前的扫描线--编号spoke:%1, 线头common.headerLen:%2, tatus:%3, scan_number[0]:%4"
                                     ", scan_number[1]:%5, u00[0]:%6, u00[1]:%7, u00[2]:%8, u00[3]:%9"
                                     ",angle[0]:%10, angle[1]:%11, heading[0]:%12, heading[1]:%13 丢失了 %14 条扫描线,正常扫描线编号:"+m_next_spoke)
                        .arg(spoke).arg(line->common.headerLen).arg(line->common.status)
                        .arg(line->common.scan_number[0]).arg(line->common.scan_number[1])
                        .arg(line->common.u00[0]).arg(line->common.u00[1]).arg(line->common.u00[2]).arg(line->common.u00[3])
                        .arg(line->common.angle[0]).arg(line->common.angle[1])
                        .arg(line->common.heading[0]).arg(line->common.heading[1]).arg(m_ri.missing_spokes);
                emit show_missing_spokes(str);
                //emit show_received_spokes(str);
                cout<<"打印missing_spokes";
              }
            }
        m_next_spoke = (spoke + 1) % SPOKES;


        int range_raw = 0;
        int angle_raw = 0;
        short int heading_raw = 0;
        double range_meters = 0;

        heading_raw = (line->common.heading[1] << 8) | line->common.heading[0];

        if (memcmp(line->br24.mark, BR24MARK, sizeof(BR24MARK)) == 0)
        {
            // BR24 and 3G mode
            range_raw = ((line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
            angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
            range_meters = (int)((double)range_raw * 10.0 / sqrt(2.0));
//            range_meters = 7249.92;//1_写死距离因子,和半径
//          打印接收数据
            QString str = QString("--3G mode--spoke编号:%1,br24.headerLen:%2,br24.status:%3,br24.scan_number[0]:%4"
                                  ",br24.scan_number[1]:%5,br24.mark[0]:%6,br24.mark[1]:%7,br24.mark[2]:%8,br24.mark[3]:%9"
                                  ",br24.angle[0]:%10,br24.angle[1]:%11,br24.heading[0]:%12,br24.heading[1]:%13 -above- br24.range[0]:%14"
                                  ",br24.range[1]:%15,br24.range[2]:%16,br24.range[3]:%17")
                    .arg(spoke).arg(line->br24.headerLen).arg(line->br24.status).arg(line->br24.scan_number[0]).arg(line->br24.scan_number[1])
                    .arg(line->br24.mark[0]).arg(line->br24.mark[1]).arg(line->br24.mark[2]).arg(line->br24.mark[3]).arg(line->br24.angle[0])
                    .arg(line->br24.angle[1]).arg(line->br24.heading[0]).arg(line->br24.heading[1])
                    .arg(line->br24.range[0]).arg(line->br24.range[1]).arg(line->br24.range[2]).arg(line->br24.range[3]);
            emit show_received_spokes(str);
            qDebug()<<"br24";
        } else {
            //打印接收数据
            QString str = QString("--4G mode-- 编号spoke:%1, 线头br4g.headerLen:%2, status:%3, scan_number[0]:%4"
                                  ", scan_number[1]:%5, u00[0]:%6, u00[1]:%7, u00[2]:%8, u00[3]:%9"
                                  ", angle[0]:%10, angle[1]:%11, heading[0]:%12, heading[1]:%13 -above-  smallrange[0]:%14"
                                  ", smallrange[1]:%15, largerange[0]:%26, largerange[1]:%27"
                                  ", rotation[0]:%16,rotation[1]:%17, u02[0]:%18, u02[1]:%19, u02[2]:%20, u02[3]:%21"
                                  ", u03[0]:%22, u03[1]:%23, u03[2]:%24, u03[3]:%25, TIME:%28"
                                  )
                    .arg(spoke).arg(line->br4g.headerLen).arg(line->br4g.status)
                    .arg(line->br4g.scan_number[0]).arg(line->br4g.scan_number[1])
                    .arg(line->br4g.u00[0]).arg(line->br4g.u00[1]).arg("-").arg("-")
                    .arg(line->br4g.angle[0]).arg(line->br4g.angle[1])
                    .arg(line->br4g.heading[0]).arg(line->br4g.heading[1])
                    .arg(line->br4g.smallrange[0]).arg(line->br4g.smallrange[1])
                    .arg(line->br4g.rotation[0]).arg(line->br4g.rotation[1])
                    .arg(line->br4g.u02[0]).arg(line->br4g.u02[1]).arg(line->br4g.u02[2]).arg(line->br4g.u02[3])
                    .arg(line->br4g.u03[0]).arg(line->br4g.u03[1]).arg(line->br4g.u03[2]).arg(line->br4g.u03[3])
                    .arg(line->br4g.largerange[0]).arg(line->br4g.largerange[1]).arg(QString::number(time_of_day_1))
                    ;
             //emit show_received_spokes(str);//打印所有数据
             //cout<<"br4g";
            qDebug()<<str;
            // 4G mode
            short int large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
            short int small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
            //ZCHXLOG_DEBUG("large_range=" << large_range );
            //ZCHXLOG_DEBUG("small_range=" << small_range );
            angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
            if (large_range == 0x80) {
                if (small_range == -1) {
                    range_raw = 0;  // Invalid range received
                } else {
                    range_raw = small_range;
                }

            } else {
                range_raw = large_range * 256;
            }
            range_meters = range_raw / 4;
//            range_meters = 7249.92;//1_写死距离因子,和半径
        }

        int azimuth_cell = uLineNum;
        bool radar_heading_valid = (((heading_raw) & ~(HEADING_TRUE_FLAG | (azimuth_cell - 1))) == 0);
        double heading;
        if (radar_heading_valid)
        {
            double heading_rotation = (((heading_raw) + 2 * azimuth_cell) % azimuth_cell);
            double heading_degrees = ((heading_rotation) * (double)DEGREES_PER_ROTATION / azimuth_cell);
            heading = (fmod(heading_degrees + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION));
        }else
        {
            //ZCHXLOG_DEBUG("radar_heading_valid=" << radar_heading_valid );
        }


        angle_raw = MOD_ROTATION2048(angle_raw / 2);
        //qDebug()<<"angle_raw:"<<angle_raw;

        double start_range = 0.0 ;
        double range_factor = range_meters/uCellNum;

        //qDebug()<<"range_meter:"<<range_meters<<" cellNum:"<<uCellNum<<" range_factor"<<range_factor;

        AmplitudeList.clear();
        pIndexList.clear();
        double dAzimuth = angle_raw*(360.0/(uLineNum/2))+uHeading;
        //cout<<"dAzimuth:"<<dAzimuth<<"angle_raw"<<angle_raw<<"uHeading"<<uHeading; //1_扫描方位,angle_raw(0-2047),uHeading(180)
        for (int range = 0; range < uCellNum; range++)
        {
            int value =  (int)(line->data[range]);
            sAzmData.iRawData[range] = 0;
            if(value>0)
            {
                AmplitudeList.append(value);
                pIndexList.append(range);
                //qDebug()<<"range"<<range<<"value"<<value;
//                if(m_bLimit)
//                {
//                    if(inLimitArea(m_dCentreLat,m_dCentreLon,dAzimuth,range,start_range,range_factor))
//                    {

//                        sAzmData.iRawData[range] = value;
//                    }
//                }
//                else
                {
                    //sAzmData.iRawData[range] = value;
                }
            }
        }

        //qDebug()<<"Amplitude"<<pIndexList.size()<< sizeof(sAzmData.iRawData);
        RADAR_VIDEO_DATA objVideoData;
        objVideoData.m_uSourceID = m_uSourceID; //1 接入雷达编号
        objVideoData.m_uSystemAreaCode = 1; //系统区域代码
        objVideoData.m_uSystemIdentificationCode = 1; //系统识别代码
        objVideoData.m_uMsgIndex = spoke; //消息唯一序列号
        objVideoData.m_uAzimuth = angle_raw; //扫描方位
        objVideoData.m_dStartRange = start_range;//扫描起始距离
//        objVideoData.m_dRangeFactor = range_factor;//1_距离因子
        objVideoData.m_dRangeFactor = mRangeFactor;//1_距离因子
        objVideoData.m_uTotalNum = uCellNum;//1_一条线上有多少个点
        objVideoData.m_dCentreLon = m_dCentreLon; //中心经度
        objVideoData.m_dCentreLat = m_dCentreLat; //中心纬度
        //objVideoData.m_uLineNum = uLineNum/2; //总共线的个数
        objVideoData.m_uLineNum = uLineNum/2; //1_总共线的个数
        objVideoData.m_uHeading = uHeading;//雷达方位

        objVideoData.m_pAmplitude = AmplitudeList;
        objVideoData.m_pIndex = pIndexList;
        //半径
        m_dRadius = range_meters;
        //qDebug()<<"radius:"<<m_dRadius;
        //m_radarVideoMap[objVideoData.m_uAzimuth] = objVideoData;
        m_radarVideoMap[objVideoData.m_uMsgIndex] = objVideoData;

        if(m_radarVideoMap.count() == 4096)
        {
            d_32++;
            if(d_32 == 4096)
                d_32 = 0;
            sAzmData.sHead.iArea = 1;//系统区域代码
            sAzmData.sHead.iSys = 1;//系统识别代码
            sAzmData.sHead.iMsg = m_radarVideoMap[d_32% SPOKES].m_uMsgIndex;//消息唯一序列号
            sAzmData.sHead.iAzm = m_radarVideoMap[d_32% SPOKES].m_uAzimuth;//扫描方位
            sAzmData.sHead.iHead = m_radarVideoMap[d_32% SPOKES].m_uHeading;//雷达方位
            sAzmData.sHead.fR0 = m_radarVideoMap[d_32% SPOKES].m_dStartRange;//扫描起始距离
            sAzmData.sHead.fDR = m_radarVideoMap[d_32% SPOKES].m_dRangeFactor;//1_距离因子
            sAzmData.sHead.iBit = 4;
            sAzmData.sHead.iTime = time_of_day;//当前时间
            for(int i = 0; i<m_radarVideoMap[d_32% SPOKES].m_pIndex.size(); i++)
            {
                sAzmData.iRawData[m_radarVideoMap[d_32% SPOKES].m_pIndex[i]] = m_radarVideoMap[d_32% SPOKES].m_pAmplitude[i];
            }
            srcDataList.append(sAzmData);
//            cout<<"d_32";
//            cout<<  d_32;
        }

    }
    emit signalRadiusFactorUpdated(m_dRadius, m_dRadius / uCellNum);
    if(srcDataList.size())
    {
        QString uu = QDateTime::currentDateTime().toString("yy-MM-dd hh-mm-ss");
        //cout<<"发送雷达目标解析信号时间" <<uu;
        emit startTrackProcessSignal(srcDataList);
        int i=0;
        for(i=0; i<srcDataList.size(); i++)
        {
            QString info = QString("%1   半径:%2--距离因子:%3   fDR:%4   fR0:%5   iArea:%6   iAzm:%7   iBit:%8   iHead:%9   iMsg:%10   iSys:%11   iTime:%12   iRadius:%13")
                    .arg("推送给周老师库的数据").arg(m_dRadius).arg(srcDataList[i].sHead.fDR).arg(srcDataList[i].sHead.fDR)
                    .arg(srcDataList[i].sHead.fR0).arg(srcDataList[i].sHead.iArea)
                    .arg(srcDataList[i].sHead.iAzm).arg(srcDataList[i].sHead.iBit)
                    .arg(srcDataList[i].sHead.iHead).arg(srcDataList[i].sHead.iMsg)
                    .arg(srcDataList[i].sHead.iSys).arg(srcDataList[i].sHead.iTime)

                    .arg(m_dRadius);//.arg(srcDataList[i].sHead.iRadius);

 //           qDebug()<<info;
            emit show_info(info, m_dRadius, srcDataList[i].sHead.fDR);//打印txt信号
        }
    }
    processVideoData();

}

void ZCHXAnalysisAndSendRadar::analysisCatRadarSlot(const QByteArray &sRadarData, int uLineNum, int uCellNum, int uHeading,const QString &sRadarType)
{
    //使用解析库进行解析cat010,cat240协议雷达
    if(m_pCAsterixFormatDescriptor == NULL)
    {
        qDebug()<<"m_pCAsterixFormatDescriptor is NULL";
        return;
    }
    if(m_pCAsterixFormatDescriptor->m_pAsterixData)
    {
        delete m_pCAsterixFormatDescriptor->m_pAsterixData;
        m_pCAsterixFormatDescriptor->m_pAsterixData = NULL;
    }
    m_pCAsterixFormatDescriptor->m_pAsterixData = m_pCAsterixFormatDescriptor->m_InputParser.parsePacket((const unsigned char*)(sRadarData.constData()), sRadarData.size());
    DataBlock* pDB = m_pCAsterixFormatDescriptor->m_pAsterixData->m_lDataBlocks.front();
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    if(sRadarType == "cat010")
    {
        QString sContent = tr("process cat010 radar ");
        emit signalSendRecvedContent(utc,"CAT010",sContent);
        analysisCat010Radar(pDB);
    }
    else if(sRadarType == "cat240")
    {
        QString sContent = tr("process cat240 radar ");
        emit signalSendRecvedContent(utc,"CAT240",sContent);
        analysisCat240Radar(pDB,uLineNum,uCellNum,uHeading);
    }
}

void ZCHXAnalysisAndSendRadar::setRadarAfterglowPixmap(const int uIndex, const QPixmap &videoPixmap, const QPixmap &objPixmap, const QPixmap &prePixmap)
{
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
    objRadarVideo.set_radarid(m_uSourceID);
    objRadarVideo.set_radarname("雷达回波余辉");
    objRadarVideo.set_latitude(m_dCentreLat);
    objRadarVideo.set_longitude(m_dCentreLon);
    objRadarVideo.set_utc(QDateTime::currentMSecsSinceEpoch());
    objRadarVideo.set_height(objPixmap.height());
    objRadarVideo.set_width(objPixmap.width());
    objRadarVideo.set_radius(m_dRadius);
    objRadarVideo.set_imagedata(videoArray.data(),videoArray.size());
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

    QString sTopic = "RadarVideo";
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();

 //   zmq_bind(m_pVideoLisher, sIPport.toLatin1().data());//
    zmq_send(m_pVideoLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sendData.data(), sendData.size(), 0);
    m_prePixmap = prePixmap;

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("send analysis radar video data,size = %1").arg(sendData.size());
    //emit signalSendRecvedContent(utc,"VIDEO________SEND",sContent);
}

void ZCHXAnalysisAndSendRadar::sendTrackSlot(int uKey,ITF_Track_point radarPoint)
{
    m_radarPointMap[uKey] = radarPoint;
    qDebug()<<"size before clear:"<<m_radarPointMap.size();
    cout<<"发送_1";
    clearRadarTrack();
    sendRadarTrack();
    qDebug()<<"size after clear:"<<m_radarPointMap.size();
}

void ZCHXAnalysisAndSendRadar::sendTrackSlot(const TrackObjList &list)
{
    //qDebug()<<__FUNCTION__<<__LINE__<<list.size();
    foreach (TrackObj obj, list) {
        m_radarPointMap_1[obj.uKey] = obj.radarPoint;
    }
    m_radarPointMap = m_radarPointMap_1;
    m_radarPointMap_1.clear();
    //cout<<"发送_2 大小为:"<<m_radarPointMap.size();
    clearRadarTrack();
    sendRadarTrack();
}

void ZCHXAnalysisAndSendRadar::readRadarLimitFormat()
{
    QString path = QCoreApplication::applicationDirPath();
    QString pathName;
    if(m_limit_file == "foshan")
    {
        pathName = path + "/radar_offset.json";
    }
    if(m_limit_file == "shandong")
    {
        pathName = path + "/radar_offset_sd.json";
    }

    cout<<"地址"<< pathName;
    QString pathName_1 = path+"/radar_offset.json";
    cout<<"pathName_1 地址"<<pathName_1<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    m_landPolygon.clear();
    m_seaPolygon.clear();
    analysisLonLatTOPolygon(pathName,m_landPolygon,m_seaPolygon);

    //test
    QString pathName1 = path+"/data";
    char p[] = "aaabbb";

      m_pFile = fopen(pathName1.toLatin1().constData(),"ab");

      if(m_pFile == NULL)
      {

          qDebug()<<"m_pFile->open failed";
      }
      else
      {
          //fwrite(p,sizeof(p),1,m_pFile);
          qDebug()<<"m_pFile->open success";
      }
      //fclose(m_pFile);


}

void ZCHXAnalysisAndSendRadar::processVideoData()
{
    if(m_radarVideoMap.isEmpty())
    {
        return;
    }


    //使用开线程
    if(m_DrawRadarVideo == NULL)
    {
        m_DrawRadarVideo = new ZCHXDrawRadarVideo();
        m_DrawRadarVideo->setLimit(m_bLimit);
        m_DrawRadarVideo->setLimitArea(m_landPolygon,m_seaPolygon);
        m_DrawRadarVideo->setDistance(m_distance);
        m_DrawRadarVideo->setAfterglowType(m_uLoopNum);

        connect(m_DrawRadarVideo,SIGNAL(signalRadarAfterglowPixmap(int,QPixmap,QPixmap,QPixmap)),
                            this,SLOT(setRadarAfterglowPixmap(int,QPixmap,QPixmap,QPixmap)));
        connect(this, SIGNAL(set_pen_width(int)), m_DrawRadarVideo, SLOT(slotSetPenwidth(int)));
    }
    bool bProcessing = m_DrawRadarVideo->getIsProcessing();
    if(bProcessing)
    {
        return;
    }
    Afterglow objAfterglow;
    objAfterglow.m_RadarVideo = m_radarVideoMap;
    objAfterglow.m_path = m_latLonVec;
    //cout<<"objAfterglow.m_RadarVideo大小"<<objAfterglow.m_RadarVideo.size();

    //qDebug()<<"track size"<<m_latLonVec.size();

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("proces video data to pixmap");
    //emit signalSendRecvedContent(utc,"VIDEO_PROCESS",sContent);


    emit m_DrawRadarVideo->signalDrawAfterglow(objAfterglow);

}

void ZCHXAnalysisAndSendRadar::analysisCat010Radar(DataBlock *pDB)
{
    int uNum = pDB->m_lDataRecords.size();
    //qDebug()<<"uNum"<<uNum;
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
//        if(pDB->getItemValue(i, strValue, "140", "ToD"))
//        {
//            trackPoint.set_timeofday(atof(strValue.c_str()));
//            //objObject.insert("ToD",atof(strValue.c_str()));
//        }
        //当日时间，使用接收到的时间
        trackPoint.set_timeofday(time_of_day);

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
        //objObject.insert("Cog",cog);
        //objObject.insert("Sog",sog);
        m_radarPointMap[tempTrackNumber] = trackPoint;


//        qDebug()<<"轨迹点信息如下:"<< "  \n"
//               << "系统区域代码: "<< objObject.value("SAC").toInt() << "  \n"
//               << "系统识别代码 :" << objObject.value("SIC").toInt() << "  \n"
//               << "消息类型  :" << objObject.value("MsgTyp").toInt() << "  \n"
//               << "航迹号  :" << objObject.value("TrkNb").toInt() << "  \n"
//               << "笛卡尔坐标计算X位置 :" << objObject.value("X").toDouble() << " \n"
//               << "笛卡尔坐标计算Y位置 :" << objObject.value("Y").toDouble() << " \n"
//               << "经度 :" << objObject.value("Lat").toDouble()  << " \n"
//               << "纬度 :" << objObject.value("Lon").toDouble() << " \n"
//               << "当日时间 :" << objObject.value("ToD").toDouble() << " \n"
//               << "航迹状态 :" << objObject.value("CNF").toInt() << " \n"
//               << "当前目标最后一次上报 :" << objObject.value("TRE").toInt() << " \n"
//               << "外推法 :" << objObject.value("CST").toInt() << " \n"
//               << "位置来历 :" << objObject.value("STH").toInt() << " \n"
//               << "x轴标准差 :" << objObject.value("Qx").toDouble() << " \n"
//               << "y轴标准差 :" << objObject.value("Qy").toDouble() << " \n"
//               << "2轴平方方差 :" << objObject.value("Qxy").toDouble() << " \n"
//               << "震荡波强度检测 :" << objObject.value("PAM").toDouble() << " \n"
//               << "迪卡尔坐标航迹计算x速度(米/秒) :" << objObject.value("Vx").toDouble() << " \n"
//               << "迪卡尔坐标航迹计算y速度(米/秒) :" << objObject.value("Vy").toDouble() << " \n"
//               << "方位角 :" << objObject.value("Cog").toDouble() << " \n"
//               << "速度 :" << objObject.value("Sog").toDouble() << " \n";

    }
    //m_latLonVec = latLonVec;
    cout<<"发送_3";
    clearRadarTrack();
    sendRadarTrack();
}

void ZCHXAnalysisAndSendRadar::analysisCat240Radar(DataBlock *pDB, int uLineNum, int uCellNum, int uHeading)
{
    std::string strValue;
    int uNum = pDB->m_lDataRecords.size();
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
            end_az = atof(strValue.c_str());
        }

        unsigned long  start_rg = 0 ;  // 开始区域号
        if(pDB->getItemValue(i, strValue, "040", "START_RG"))
        {
            //ZCHXLOG_INFO("start_rg: " << strValue);
            start_rg = atol(strValue.c_str());
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

        }
        if (bit_resolution ==0)
        {
            bit_resolution = 4;
        }

        int time_of_day = 0 ;
        if(pDB->getItemValue(i, strValue, "140", "ToD"))
        {
            //ZCHXLOG_INFO("time_of_day: " << strValue);
            time_of_day = std::round(atoi(strValue.c_str())/128); // 1/128 s
        }
        if (time_of_day == 0)
        {
            //ZCHXLOG_DEBUG("...... time_of_day == 0....");
//            boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
//            boost::posix_time::time_duration td = now.time_of_day();
//            time_of_day = td.total_seconds();
            time_of_day = QDateTime::currentMSecsSinceEpoch();
        }

        double range_factor = CELL_RANGE(cell_dur);
//        qDebug()<<"扇区频谱信息如下:"<< "  \n"
//               << "系统区域代码: "<< systemAreaCode << "  \n"
//               << "系统识别代码: " << systemIdentificationCode << "  \n"
//               << "消息唯一序列号: " << msgIndex << " \n"
//               << "消息类型: " << messageType << "  \n"
//               <<"消息转发次数: " <<rep<<"\n"
//              << "方向角起始位置: " << start_az << "  \n"
//              << "方向角结束位置: " << end_az << " \n"
//              << "扫描起始距离: " << start_rg << " \n"
//              << "持续时间: " << cell_dur  << " \n"
//              << "距离因子: " << range_factor  << " \n"
//              << "视频分辨率: " << bit_resolution << " \n"
//              << "当日时间: " << time_of_day << " \n"
//                 ;

        int numRadials = end_az - start_az;

        if(pDB->getItemBinary(i, strValue, "052"))
        {

            //ZCHXLOG_DEBUG("config.range_cell: "<< config.range_cell);
            unsigned long int nDestLen = numRadials *uCellNum;

            unsigned char* pDest = new unsigned char[nDestLen];

            unsigned char* pSrcBuf = (unsigned char*)strValue.data()+1;
            unsigned long int  nSrcLen = strValue.length()-1;
            if (0 != pDest)
            {
                int ret = uncompress(pDest, &nDestLen, pSrcBuf, nSrcLen);
                if (ret == Z_OK)
                {
                    // ================调试16进制的内容: 开始=====================
                    //int  body_length = numRadials *config.range_cell*2 +1;
                    //ZCHXLOG_DEBUG("body_length:" << body_length);
                    //char converted[111849];
                    //int i;
                    //for(i=0;i<nDestLen;i++)
                    //{
                    //  sprintf(&converted[i*2], "%02X", pDest[i]);
                    //}
                    //std::string str(converted);
                    //ZCHXLOG_DEBUG( "hex:" <<   str );
                    // ================调试16进制的内容: 结束=====================



                    int position = 0;
                    for (int line = 0; line < numRadials; line++)
                    {

//                        com::zchxlab::radar::protobuf::VideoFrame videoFrame;
//                        videoFrame.set_systemareacode(systemAreaCode);
//                        videoFrame.set_systemidentificationcode(systemIdentificationCode);
//                        videoFrame.set_msgindex(msgIndex);
//                        videoFrame.set_azimuth(start_az+line);
//                        videoFrame.set_startrange(start_rg*range_factor);
//                        videoFrame.set_rangefactor(range_factor);
//                        videoFrame.set_bitresolution(boost::numeric_cast<com::zchxlab::radar::protobuf::RES ,int>(bit_resolution));
//                        videoFrame.set_timeofday(time_of_day);
//                        if (line == 0)
//                            qDebug()<<"start 扫描方位: " << start_az + line;
//                        if (line + 1 == numRadials)
//                            qDebug()<<"end   扫描方位: " << start_az + line;

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
                                AmplitudeList.append(value);
                                pIndexList.append(range);
                            }
                            position++;
                        }
                        objVideoData.m_pAmplitude = AmplitudeList;
                        objVideoData.m_pIndex = pIndexList;

                        //半径
                        m_dRadius = objVideoData.m_dStartRange+objVideoData.m_dRangeFactor*uCellNum;

//                        qDebug()<<"AgilTrack 视频帧信息如下:"<< "  \n"
//                        << "系统区域代码: "<< objVideoData.m_uSystemAreaCode << "  \n"
//                        << "系统识别代码: "<< objVideoData.m_uSystemIdentificationCode << "  \n"
//                        << "消息唯一序列号 : "<< objVideoData.m_uMsgIndex << "  \n"
//                        << "扫描方位: "<< objVideoData.m_uAzimuth << "  \n"
//                        << "扫描起始距离: "<< objVideoData.m_dStartRange << "  \n"
//                        << "距离因子 :" << objVideoData.m_dRangeFactor << "  \n"
//                        << "一条线上点个数  :" << objVideoData.m_uTotalNum << "  \n"
//                        << "总共线的个数  :" << objVideoData.m_uLineNum << "  \n"
//                        << "中心纬度  :" << objVideoData.m_dCentreLat << "  \n"
//                        << "中心经度  :" << objVideoData.m_dCentreLon << "  \n"
//                        ;

//                        qint64 utc = QDateTime::currentMSecsSinceEpoch();
//                        QString sContent = "";
//                        sContent+= QString("扫描方位: %1").arg(objVideoData.m_uAzimuth);
//                        sContent+= "  \n";
//                        sContent+= QString("持续时间: %1").arg(cell_dur);
//                        sContent+= "  \n";
//                        sContent+= QString("距离因子: %1").arg(objVideoData.m_dRangeFactor);
//                        sContent+= "  \n";
//                        emit signalSendRecvedContent(utc,"",sContent);


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
        else if(pDB->getItemBinary(i, strValue, "051"))
        {
            //ZCHXLOG_DEBUG("data is 051");
            unsigned long int nDestLen = numRadials *uCellNum;

            unsigned char* pDest = new unsigned char[nDestLen];

            unsigned char* pSrcBuf = (unsigned char*)strValue.data() + 1;
            unsigned long int  nSrcLen = strValue.length() - 1;
            if (0 != pDest)
            {
                int ret = uncompress(pDest, &nDestLen, pSrcBuf, nSrcLen);
                if (ret == Z_OK)
                {
                    //ZCHXLOG_DEBUG("uncompress ok");
                }
                else
                {
                    //ZCHXLOG_DEBUG("uncompress error");
                }
            }
        }
    }
    processVideoData();
}

void ZCHXAnalysisAndSendRadar::sendRadarTrack()
{
    if(m_radarPointMap.size()<=0)
        return;
    //通过zmq发送
    //emit show_statistics(m_ri.packets,m_ri.broken_packets,m_ri.spokes,m_ri.broken_spokes,m_ri.missing_spokes);
    int uNum = m_radarPointMap.size();
    QString sNum = QString::number(uNum);
    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uTrackSendPort);

    QString sTopic = m_sTrackTopic;
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
    QByteArray sNumArray = sNum.toUtf8();

    //zmq_bind(m_pTrackLisher, sIPport.toLatin1().data());//
    zmq_send(m_pTrackLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pTrackLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pTrackLisher, sNumArray.data(), sNumArray.size(), ZMQ_SNDMORE);
    QMap<int,com::zhichenhaixin::proto::TrackPoint>::iterator itor = m_radarPointMap.begin();
    std::vector<std::pair<double, double>> latLonVec;
    latLonVec.clear();
    QVector<com::zhichenhaixin::proto::TrackPoint> filterRadarPointVec;
    filterRadarPointVec.clear();
    //先过滤
    for(itor;itor!=m_radarPointMap.end();itor++)
    {
        com::zhichenhaixin::proto::TrackPoint objRadarPoint = itor.value();

        if(m_bLimit)
        {
            if(!inLimitAreaForTrack(objRadarPoint.wgs84poslat(),objRadarPoint.wgs84poslong()))
            {
                continue;
            }
        }
        std::pair<double, double> latLonPair(objRadarPoint.wgs84poslat(),objRadarPoint.wgs84poslong());
        latLonVec.push_back(latLonPair);

        filterRadarPointVec.append(objRadarPoint);
    }
    int uFilterNum = filterRadarPointVec.size();
    emit show_video(m_radarPointMap.size(),uFilterNum);
    for(int i = 0;i<uFilterNum;i++)
    {
        com::zhichenhaixin::proto::TrackPoint objRadarPoint = filterRadarPointVec[i];
        QByteArray sendData;
        sendData.resize(objRadarPoint.ByteSize());
        objRadarPoint.SerializePartialToArray(sendData.data(),sendData.size());
        if(i!=uFilterNum-1)
        {
            zmq_send(m_pTrackLisher, sendData.data(), sendData.size(), ZMQ_SNDMORE);
        }
        else
        {
            zmq_send(m_pTrackLisher, sendData.data(), sendData.size(), 0);
        }

//        QString str = QString::number(objRadarPoint.timeofday()).toUtf8();
//        cout<<"原始数据时间:"<<str;
    }
    m_latLonVec = latLonVec; //雷达目标经纬度集合
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("send analysis radar track data,num = %1").arg(uFilterNum);
    emit signalSendRecvedContent(utc,"TRACK________SEND",sContent);
//    QDateTime curDateTime = QDateTime::currentDateTime();
//    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
//                                  curDateTime.date().day()),QTime(0, 0));
//    int time_of_day = startDateTime.secsTo(curDateTime);
//    cout<<"雷达目标解析完成推送时间:" <<time_of_day ;
}

void ZCHXAnalysisAndSendRadar::clearRadarTrack()
{
    //间隔一定时间清理回波数据
    int nInterval = m_clearRadarTrackTime;//秒

    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);
    QMap<int,com::zhichenhaixin::proto::TrackPoint>::iterator itor = m_radarPointMap.begin();
    //qDebug()<<"ITF_RadarPoint num "<<m_radarPointMap.size();
    for(itor;itor!=m_radarPointMap.end();)
    {
        com::zhichenhaixin::proto::TrackPoint objRadarPoint = itor.value();
        int uKey = itor.key();
        //qDebug()<<"time_of_day"<<time_of_day;
        //qDebug()<<"objRadarPoint.tracknumber()"<<objRadarPoint.tracknumber();
       // qDebug()<<"objRadarPoint.timeofday()"<<objRadarPoint.timeofday();
        if(time_of_day-objRadarPoint.timeofday()>nInterval)
        {
            //qDebug()<<"remove";
            itor = m_radarPointMap.erase(itor);
        }
        else
        {
            itor++;
        }
    }
}

void ZCHXAnalysisAndSendRadar::analysisLonLatTOPolygon(const QString sFileName, QList<QPolygonF> &landPolygon, QList<QPolygonF> &seaPolygon)
{
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
    //qDebug()<<"sAllData"<<sAllData;


    QJsonParseError err;
    QJsonDocument docRcv = QJsonDocument::fromJson(sAllData.toLatin1(), &err);

    if(err.error != QJsonParseError::NoError)
    {
        qDebug()<<"parse completetion list error:"<<err.error;
        return ;
    }
    if(!docRcv.isObject())
    {
        qDebug()<<" status statistics list with wrong format.";
        return ;
    }
    QJsonArray objSeaArray = docRcv.object().value("watercourse").toArray();
    QJsonArray objLandArray = docRcv.object().value("land").toArray();

    QVector<QPointF> pointVec;
    for(int i = 0; i < objSeaArray.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objSeaArray.at(i).toArray();
        //qDebug()<<"objArray.size()"<<objArray.size();
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



    for(int i = 0; i < objLandArray.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray.at(i).toArray();
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
}

bool ZCHXAnalysisAndSendRadar::inLimitAreaForTrack(const double dLat, const double dLon)
{
    bool bOk = false;
    QPointF pos(dLon,dLat);
    if(m_seaPolygon.count()<=0&&m_landPolygon.count()<=0)
    {
        return true;//没有限制区域
    }

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
}
