#ifndef GLOGFUNCTION_H
#define GLOGFUNCTION_H

#include <string>
#include <QObject>
#include <QDebug>
#include <QColor>
#include <vector>
#include <QVector2D>
#include "windef.h"

#ifndef GLOB_PI
#define GLOB_PI  (3.14159265358979323846)
#endif

#define         GLOB_SPEED_MS2KNOT          0.5144444
#define         GLOB_DEGPERRAD              180.0 / GLOB_PI
#define         GLOB_WATER_DENSITY          1025
#define         GLOB_GRAVITY_ACCE           9.8

#define         INT_VAL(val)                (int)((val) < 0? (val) - 0.5 : (val)+0.5)


#define         GLOB_FUNC_INS               GlogFunction::instance()
//判断是否相交
//(X1,Y1)(X2,Y2)前线
//(X3,Y3)(X4,Y4)后线
//(x,y)交点坐标
//相交在前线上return 1，后线return 2，交于外面return 3,不相交return 0
int MyCross(double X1,double Y1,double X2,double Y2,double X3,double Y3,double X4,double Y4,double &x,double &y);

typedef struct tagLLStu{
    int d;
    int m;
    int s;
    void tagLL()
    {
        d=m=s=0;
    }
}LLStu;

enum NUM_COMPARE_MODE
{
    COMPARE_RELATIVE =0,
    COMPARE_PLUS,
    COMPARE_MINUS,
};
class Point{
public:
    double m_pointX;
    double m_pointY;
public:
    Point(){}
    Point(double x, double y){m_pointX = x; m_pointY = y;}
};

class  Line:public Point{
public:
    double a;
    double b;
    double c;
public:
    Line GetLine(Point ptSource, Point ptDestination);
    Point GetCrossPoint(Line l1, Line l2);
};

#define     DOUBLE_EPS   0.000001
struct Mercator{
public:
    Mercator(double x, double y){mX = x; mY= y;}
    bool operator ==(const Mercator& other)
    {
        return fabs(this->mX- other.mX) <= DOUBLE_EPS  && \
               fabs(this->mY - other.mY) <= DOUBLE_EPS ;
    }
    double mX;
    double mY;
};

struct Wgs84LonLat{
public:
    Wgs84LonLat() {mLon = 0.0; mLat = 0.0;}
    Wgs84LonLat(double x, double y){mLon = x; mLat= y;}
    bool operator ==(const Wgs84LonLat& other) const
    {
        return fabs(this->mLon - other.mLon) <= DOUBLE_EPS  && \
               fabs(this->mLat - other.mLat) <= DOUBLE_EPS ;
    }

    double mLon;
    double mLat;
};

enum PNTPOSTION{
    POS_UNDETERMINED = -1,
    POS_ON_LINE = 0,        //点在直线上
    POS_LEFT,//左侧
    POS_RIGHT,//右侧
    POS_AHEAD,//前方
    POS_AFTER,//后方
    POS_ON_VERTEX,

};

enum BYTE_POS_MODE
{
    HIGH_FIRST = 0,
    LOW_FIRST,
};



class  GlogFunction : public QObject
{
    Q_OBJECT
public:
    ~GlogFunction();
    static GlogFunction *instance();

    double toDeg(double rad);

    double ang2Cir(double angle);

    //墨卡托和wgs84互转
    Wgs84LonLat mercatorToWgs84LonLat(const Mercator& mercator);
    Mercator wgs84LonlatToMercator(const Wgs84LonLat& wgs84 );

    //计算回收张力 dWaterDepth-水深,dBaseWaterWeight-海缆水中重量,dBaseTransFriction-海缆摩擦系数,
    //           dKpSegment-kp间距,dCableSegment-海缆间距，dDeltaTension-前两个点的张力差
    //1.布揽回收张力
    double calcTension1(double dWaterDepth,double dBaseWaterWeight,double dKpSegment,double dCableSegment);
    //2.未断海缆回收张力
    double calcTension2(double dWaterDepth,double dBaseWaterWeight,double dBaseTransFriction,double dKpSegment,double dCableSegment,double dWaterIntoAngle,double dDeltaTension);

    double degToRad(double d);
    double radToDeg(double r);
    /**
     * @brief getDistanceDeg
     * @param d
     * @return
     * 计算两个经纬度之间的直线距离(球面距离)
     */
    double getDistanceDeg(double lat1, double lon1, double lat2, double lon2);
    double calcAzimuth(double lon1, double lat1, double lon2, double lat2);
    double getDistanceDeg(const Wgs84LonLat& p1, const Wgs84LonLat& p2);

    //计算向量夹角
    double      calIncluedAng(const QVector2D& v1, const QVector2D& v2);
    //计算点到路由点的投影点(经纬度点到球面直线的投影点)
    Wgs84LonLat calProjectionPnt(const Wgs84LonLat& target,\
                                 const Wgs84LonLat& line_start,\
                                 const Wgs84LonLat& line_end);
    //计算点到球面直线的距离(参数传入经纬度值。度)
    double calDisOfPnt2Line(double x, double y, double p1x, double p1y, double p2x, double p2y );
    double calDisOfPnt2Line2(double x, double y, double p1x, double p1y, double p2x, double p2y );

    //计算线段的起点到投影点的距离
    double calDisOfPntDivLine(double x, double y, double p1x, double p1y, double p2x, double p2y );
    double calDisOfPntDivLine2(double x, double y, double p1x, double p1y, double p2x, double p2y );
    /**
     * @brief str_cast_loc
     * @param str
     * @return
     * 经纬度转换
     */
    double strCastLoc(const QString &str1); //度分秒 N S
    QString latLon2String(double coordinate);
    QString latLon2String_new(double coordinate);
    QString latLon2String(double coordinate, bool latflag, int mode = 1);
    QString latLon2StringDMS(double coordinate, bool latflag);
    LLStu latLon2String_split(const double &coordinate);
    double latLon2Double(int d,int m,int s);
    /**
     * @brief
     * @param
     * @return
     * 笛卡尔坐标计算（X,Y）就是平面直角的坐标计算
     */
    double getDistance(double X1, double Y1, double X2, double Y2);
    double getAngle(double X1, double Y1, double X2, double Y2);
    bool CalculatXY(double X1, double Y1, double distance, double angle, double &x_out,double &y_out);

    //将N10 59.085467等转换为double型的经纬度
    double latLon2Double(const QString& latlonstr);

    //度分/度分秒/有NEWS格式的字符串转成度格式
    double latLonStrToDouble(const QString& sLonLatStr);

    /**
     * @brief secToTime
     * @param sec
     * @return
     * 秒数据转化为时:分:秒
     */
    QString secToTime(const int &sec);

    QString md5(const QString &str);

    int from_hex(char a);
    std::string cmp_chksum(std::string str);

    /*通过与前一个经纬度点的距离和方位求出该点的经纬度
    **lat1-前一个纬度
    **lon1-前一个经度
    **dist-距离
    **brng-方位
    **lat_out-输出纬度
    **lon_out-输出经度*/
    void distbear_to_latlon(double lat1, double lon1, double dist, double brng, double &lat_out, double &lon_out);

    //数字的分组显示(1,000,000)
    QString int2grp(int num);
    QString double2grp(double num);
    int  fmtstr2Int(const QString& str);
    double fmtstr2Double(const QString& str);

    //GBK转UTF-8
    QString GBK2UTF8(const QString& src);
    QString UTF82GBK(const QString& src);

    //计划和实时的值进行比较
    bool  checkDouble(double real, double plan, double limit, NUM_COMPARE_MODE mode);
    bool  checkInt(qint64 real, qint64 plan, double limit, NUM_COMPARE_MODE mode);

    //RGB颜色和16进制颜色的转换
    QString FFColor2RGB(const QString& src);
    QString RGBColor2FF(const QString& src);
    QColor  colorFromRGBString(const QString& src);

    //计算入水角度(dShipSpeed船速，cableDiameter海缆直径，cableWaterWeight水中重量，constant水力系数)
    //double calculateCableWaterEntryAngle(const double dShipSpeed,const double cableDiameter
    //                                     ,const double cableWaterWeight,const double constant);

    //计算入水角度(dShipSpeed船速，cableDiameter海缆直径，cableWaterWeight水中重量，constant阻力系数  FluifDensity密度)
    double NewCalculateCableWaterEntryAngle(const double dShipSpeed,const double cableDiameter
                                         ,const double cableWaterWeight,const double constant
                                         ,const double FluifDensity);
    double calculateCableWaterEntryAngle(const double dShipSpeed, const double dCableSinkSpeed);
    double calculateCableWaterEntryAngleAtan2(const double dShipSpeed, const double dCableSinkSpeed);
    double convertSpeedMS2Knots(double speed);
    double convertSpeedMS2DegreeKnots(double speed);
    //根据沉降速度和入水角确定船速
    double calShipSpeed(double sink_speed, double cable_angle);

    /*计算沉降速度（m/s）
        cableDiameter海缆直径(m)
        cableWaterWeight水中重量(kg/m）
        constant水力系数
        dWaterDensity海水密度(1025kg/m3)
    */
    double calculateCableSinkSpeed(const double cableDiameter,\
                                   const double cableWaterWeight,\
                                   const double constant,\
                                   const double dWaterDensity = GLOB_WATER_DENSITY);

    //分钟数转马凯的时间显示格式互转
    int         msecs2MakaiDay(qint64 msecs);
    QString     msecs2MakaiStringHHMMSS(qint64 msecs);
    QString     msecs2MakaiStringDDHHMMSS(qint64 msecs);
    QString     msecs2MakaiString(qint64 msecs);
    QString     msecs2MakaiStringOutput(qint64 msecs);
    QString     msecs2MakaiStringDDHHMMSSOutPut(qint64 msecs);
    qint64      makaiStringDDHHMMSS2MSecs(const QString& str);
    qint64      makaiStringHHMMSS2MSecs(const QString& str);
    qint64      makaiString2MSecs(const QString& str);


    //判断经纬度点是否在线段上。pos（0： 线段上， 1：线段左侧，2：线段右侧, 3:线段后方, 4:线段前方)
    bool   isSamePoint(const Wgs84LonLat& p1, const Wgs84LonLat& p2);
    bool   isPntOnLine(const QPointF& target, const QPointF& p1, const QPointF& p2, int& pos, double& a1, double& a2);
    PNTPOSTION pointPosOfLine(double& pnt2line_dis, double& pntDivline_dis, const Wgs84LonLat& target, const Wgs84LonLat& tart, const Wgs84LonLat& end);
    //判断点是否在前进的方向上
    bool   isPntAhead(const QPointF& target, const QPointF& p1, const QPointF& p2);
    //判断点是否在附近
    bool   isPntNear(double& dis, const QPointF& refer, const QPointF& test, double limit = 15.0);
    //计算犁设备的经纬度
    bool   calPlowPostion(double tow_wrie_out, double water_depth, double ship_course, double ship_kp, double ship_lon, double ship_lat, double &plow_kp, double &plow_lon, double &plow_lat);
    //添加位置到轨迹点列
    void   addPointIntoList(std::vector<std::pair<double, double> > &path, double lat, double lon);
    //CRC校验值生成
    ushort CRCModbus16( uchar* pucFrame, ushort usLen );
    //根据高低位设置的不同进行转换
    QByteArray short2Bytes(short val, int mode);
    short bytes2Short(const QByteArray& bytes, int mode);
    double byte2ShortDouble(const QByteArray& val, int mode);
    QByteArray int2Bytes(int val, int mode);
    int byte2Int(const QByteArray& val, int mode);
signals:
    void readExcelForTideDataProcess(const int data);
    void readExcelForTrendDataProcess(const int data);
private:
    explicit GlogFunction(QObject * parent = 0);
private:
    static GlogFunction *m_instance;
};
#endif // GLOGFUNCTION_H
