#ifndef ZCHXFUNCTION_H
#define ZCHXFUNCTION_H
#include <math.h>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QTime>
#include <QDebug>
#include "zchxradarcommon.h"

const double PI = 3.1415926;
#define atan2fbystephen(y,x) ((float)atan2((double)(y),(double)(x)))
#define RADIUS 6372795.0            //Earth radius 6371000
#define VELOCITY_OF_LIGHT 300000000 //光速
#define CELL_RANGE(dur_time) ((VELOCITY_OF_LIGHT/2) * (dur_time))
#define LINES_PER_ROTATION (2048)

#define SPOKES (4096)               // BR radars can generate up to 4096 spokes per rotation,

#define SCALE_RAW_TO_DEGREES(raw) ((raw) * (double)DEGREES_PER_ROTATION / SPOKES)
#define SCALE_RAW_TO_DEGREES2048(raw) ((raw) * (double)DEGREES_PER_ROTATION / LINES_PER_ROTATION)
#define SCALE_DEGREES_TO_RAW(angle) ((int)((angle) * (double)SPOKES / DEGREES_PER_ROTATION))
#define SCALE_DEGREES_TO_RAW2048(angle) ((int)((angle) * (double)LINES_PER_ROTATION / DEGREES_PER_ROTATION))
#define MOD_DEGREES(angle) (fmod(angle + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION))
#define MOD_ROTATION(raw) (((raw) + 2 * SPOKES) % SPOKES)
#define MOD_ROTATION2048(raw) (((raw) + 2 * LINES_PER_ROTATION) % LINES_PER_ROTATION)
#define DEGREES_PER_ROTATION (360)  // Classical math
#define HEADING_TRUE_FLAG 0x4000
#define HEADING_MASK (SPOKES - 1)
#define HEADING_VALID(x) (((x) & ~(HEADING_TRUE_FLAG | HEADING_MASK)) == 0)
#define HEADING_TRUE_FLAG 0x4000

#define Rc  6378137
#define	Rj  6356725



//自定义链表结构体
typedef struct Node{
    bool have_value; //是否已经属于某个目标
    std::pair<double, double> latlonPair; //经纬度
    QPoint trackP; // 二维数组中的坐标
    int amplitude;//振幅值
    int index;//振幅位置0-511,扫描线上对应的位置
    //Node *down,*left,*right; //下,左,右节点
    Node():have_value(false){}//初始化函数
}TrackNode;

struct TrackInfo
{
    int   iTraIndex;    // 航迹编号
    float fRng;     // 航迹距离
    float fAzm;     // 航迹方位
    float fSpeed;     // 航迹速度
    float fCourse;    // 航迹航向
};

struct LatLong
{
    double m_LoDeg, m_LoMin, m_LoSec;
    double m_LaDeg, m_LaMin, m_LaSec;
    double m_Longitude, m_Latitude;//
    double m_RadLo, m_RadLa;
    double Ec;
    double Ed;
    LatLong()
    {}
    LatLong(double longitude, double latitude)
    {
        m_LoDeg = (int)longitude;
        m_LoMin = (int)((longitude - m_LoDeg) * 60);
        m_LoSec = (longitude - m_LoDeg - m_LoMin / 60) * 3600;

        m_LaDeg = (int)latitude;
        m_LaMin = (int)((latitude - m_LaDeg) * 60);
        m_LaSec = (latitude - m_LaDeg - m_LaMin / 60) * 3600;

        m_Longitude = longitude;
        m_Latitude = latitude;
        m_RadLo = longitude * PI / 180;
        m_RadLa = latitude * PI / 180;
        Ec = Rj + (Rc - Rj) * (90 - m_Latitude) / 90;
        Ed = Ec * cos(m_RadLa);
    }
};

enum RES
{
        MONOBIT_RESOLUTION      = 1,
        LOW_RESOLUTION          = 2,
        MEDIUM_RESOLUTION       = 3,
        HIGH_RESOLUTION         = 4,
        VERY_HIGH_RESOLUTION    = 5,
        ULTRA_HIGH_RESOLUTION   = 6
};
struct RADAR_VIDEO_DATA
{
  int m_uSourceID;
  int m_uSystemAreaCode;            // 系统区域代码 I240/010 _sac
  int m_uSystemIdentificationCode;  // 系统识别代码 I240/010 _sic
  int m_uMsgIndex;                  // 消息唯一序列号
  int m_uLineNum;
  int m_uAzimuth;                   // 扫描方位
  int m_uHeading;                   // 航向
  int m_timeofDay;                  //时间
  double m_dStartRange;             // 扫描起始距离
  double m_dRangeFactor;            // 距离因子 或者 采样距离
  RES    m_bitResolution;           // 视频分辨率
//  QList<int> m_pAmplitude;
//  QList<int> m_pIndex;
  QList<int>    mLineData;
  int    m_uTotalCellNum;
  double m_dCentreLon;
  double m_dCentreLat;
  RADAR_VIDEO_DATA();
  ~RADAR_VIDEO_DATA();
};
struct Afterglow
{
    QMap<int,RADAR_VIDEO_DATA> m_RadarVideo;
    std::vector<std::pair<double, double>> m_path;//雷达目标点集合
    std::vector<std::pair<float, float>> m_pathCartesian;//雷达目标点集合(笛卡尔坐标)
};

struct zchxRadarVideoTask
{
    QMap<int,RADAR_VIDEO_DATA>  m_RadarVideo;
    bool                        m_Rotate;
    double                      m_Range;
    quint64                     m_TimeStamp;
};
typedef QList<zchxRadarVideoTask>   zchxRadarVideoTaskList;

Q_DECLARE_METATYPE(zchxRadarVideoTask)
Q_DECLARE_METATYPE(zchxRadarVideoTaskList)

struct receive_statistics {
  int packets;
  int broken_packets;
  int spokes;
  int broken_spokes;
  int missing_spokes;
};


// 输出单个点迹数据
float* OutPlotInfo(float* pfData);

// 输出所有点迹数据
int* OutPlot(int iScan, int* pPlot);

// 输出单个航迹数据
float* OutTrackInfo(float* pfData, TrackInfo & trackinfo);

// 输出所有航迹数据
int* OutTrack(int iScan, int* pTrack, std::list<TrackInfo> & trackList);


void getLatLong(LatLong &A, double distance, double angle, double &lat, double &lng);
void getDxDy(LatLong &A, double lat, double lng, double &x, double &y);
void getNewLatLong(LatLong &A, double &lat, double &lng, double x, double y);

#include <QPolygon>
QPointF getCenterOfGravityPoint(double& area, const QPolygonF& list);
QPointF getCenterOfPoint(const QPolygonF& list);
double  getArea( const QPointF& p0 , const QPointF& p1 , const QPointF& p2 );



int GetPolar(float x,float y, float *range, float *bearing);

/*-------------------------------------------
*
* 角度转弧度
*
---------------------------------------------*/
double toRad(double deg);


/*-------------------------------------------
*
* 弧度转角度
*
---------------------------------------------*/
double toDeg(double rad);


/*-------------------------------------------
*
* 输入：笛卡尔坐标航迹计算速度坐标
* 输出：角度
*
---------------------------------------------*/
double calCog(double Vx, double Vy);

/*-------------------------------------------
*
* 输入：笛卡尔坐标航迹计算速度坐标
* 输出：径向速度
*
---------------------------------------------*/
double calSog(double Vx, double Vy);


void distbearTolatlon(double lat1, double lon1, double dist, double brng, double &lat_out, double &lon_out);


void distbearTolatlon1(double lat1,double lon1,double dist,double brng,double *lat_out,double *lon_out );

void convertXYtoWGS(double centerLat,double centreLon,float X,float Y,double *lat,double *lon);


double getDisDeg(double lat1, double lon1, double lat2, double lon2);

#define         GLOB_PI                                 (3.14159265358979323846)
#define         DOUBLE_EPS                              0.000001
#define         EARTH_HALF_CIRCUL_LENGTH                20037508.3427892


struct Mercator
{
    double mX;
    double mY;
    Mercator()
    {
        mX = 0.0;
        mY = 0.0;
    }

    Mercator(double x, double y)
    {
        mX = x;
        mY = y;
    }

    Mercator offset(double dx, double dy)
    {
        return Mercator(mX +dx, mY + dy);
    }
};

struct Latlon{
    double lat;
    double lon;

    Latlon()
    {
        lat = 0.0;
        lon = 0.0;
    }

    Latlon(double y, double x)
    {
        lon = x;
        lat = y;
    }
};

Latlon mercatorToLatlon(const Mercator& mct);
Mercator latlonToMercator(const Latlon& ll);

class zchxPosConverter
{
public:
    zchxPosConverter(const QPointF& pnt, const Latlon& center, double range_facor)
    {
        mCenterPnt = pnt;
        mCenterLL.lat = center.lat;
        mCenterLL.lon = center.lon;
        mRangeFactor = range_facor;
    }
    Latlon pixel2Latlon(const QPointF& pix)
    {
        double dkx = mRangeFactor * (-(pix.y() -  mCenterPnt.y()));
        double dky = mRangeFactor * (pix.x() - mCenterPnt.x());
        LatLong startLatLong(mCenterLL.lon, mCenterLL.lat);
        double lat;
        double lon;
        getNewLatLong(startLatLong,lat, lon, dkx, dky);
        return Latlon(lat, lon);
    }

    QPointF Latlon2Pixel(const Latlon& ll)
    {
        LatLong startLatLong(mCenterLL.lon, mCenterLL.lat);
        double dx = 0, dy =0;
        getDxDy(startLatLong, ll.lat, ll.lon, dx, dy);
        dx /= mRangeFactor;
        dy /= mRangeFactor;
        return mCenterPnt + QPointF(dy, -dx);
    }

private:
    QPointF     mCenterPnt;
    Latlon      mCenterLL;
    double      mRangeFactor;
};

class zchxTimeElapsedCounter
{
public:
    zchxTimeElapsedCounter(const QString& func)
    {
        mFunc = func;
        mTimer.start();
        mTotal = 0;
    }
    ~zchxTimeElapsedCounter()
    {
        print();
    }

    void reset(const QString& tag = QString())
    {
        QString old = mFunc;
        if(tag.size() > 0)
        {
            mFunc = tag;
        }
        print();
        mFunc = old;

        mTimer.start();
    }

    void print()
    {
        qint64 counter = mTimer.elapsed();
        mTotal += counter;
        qDebug()<<mFunc<<" elapsed:"<<counter<< "and total msecs:"<<mTotal;
    }

private:
    QString mFunc;
    QTime   mTimer;
    qint64  mTotal;
};


double timeOfDay();

void   exportRectDef2File(const zchxRadarRectDefList& list, const QString& fileName);

enum RadarType{
    RADAR_UNKNOWN = 0,
    RADAR_BR24,
    RADAR_3G,
    RADAR_4G,
    RADAR_6G,
};

#endif // ZCHXFUNCTION_H
