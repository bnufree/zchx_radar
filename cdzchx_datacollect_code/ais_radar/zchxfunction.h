#ifndef ZCHXFUNCTION_H
#define ZCHXFUNCTION_H
#include <math.h>
#include <QList>
#include <QMap>
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

enum ZCHX_RES
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
  int m_uSystemAreaCode;
  int m_uSystemIdentificationCode;
  int m_uMsgIndex;
  int m_uLineNum;
  int m_uAzimuth;
  int m_uHeading;
  double m_dStartRange;
  double m_dRangeFactor;
  ZCHX_RES    m_bitResolution;
  QList<int> m_pAmplitude;
  QList<int> m_pIndex;
  int    m_uTotalNum;
  double m_dCentreLon;
  double m_dCentreLat;
  RADAR_VIDEO_DATA();
  ~RADAR_VIDEO_DATA();
};
struct Afterglow
{
    QMap<int,RADAR_VIDEO_DATA> m_RadarVideo;
    std::vector<std::pair<double, double>> m_path;//雷达目标点集合
};

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
void getLatLong_1(LatLong &A, double distance, double angle, double &lat, double &lng, double &x, double &y);


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



#endif // ZCHXFUNCTION_H
