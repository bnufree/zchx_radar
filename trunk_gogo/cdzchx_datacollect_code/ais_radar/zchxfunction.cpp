#include "zchxfunction.h"
#include <QPointF>
#include <QDebug>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

RADAR_VIDEO_DATA::RADAR_VIDEO_DATA()
{
    m_uSourceID = 0;
    m_uSystemAreaCode = 0;
    m_uSystemIdentificationCode = 0;
    m_uMsgIndex = 0;
    m_uLineNum = 0;
    m_uAzimuth = 0;
    m_uHeading = 0;
    m_dStartRange = 0;
    m_dRangeFactor = 0;
    m_bitResolution = RES::MONOBIT_RESOLUTION;
    mLineData.clear();
    m_uTotalCellNum = 0;
    m_dCentreLon = 0;
    m_dCentreLat = 0;
}

RADAR_VIDEO_DATA::~RADAR_VIDEO_DATA()
{
    mLineData.clear();
}

// 输出单个点迹数据
float* OutPlotInfo(float* pfData)
{
    float fRng = *pfData++;
    float fAzm = *pfData++;
    //ZCHXLOG_INFO("===plot===\n"
    //  "fRng: " << fRng << "\n"
    //  "fAzm: " << fAzm << "\n");
    return pfData;
}

// 输出所有点迹数据
int* OutPlot(int iScan, int* pPlot)
{
    int k = 0;
    int iPlotCnt = *pPlot++;
    float* pfData = (float*)pPlot;

    for (k = 0; k<iPlotCnt; k++)
    {
        pfData = OutPlotInfo(pfData);
    }
    return (int*)pfData;
}

// 输出单个航迹数据
float* OutTrackInfo(float* pfData, TrackInfo & trackinfo)
{
    trackinfo.iTraIndex = *(int*)pfData++;
    trackinfo.fRng = *pfData++;
    trackinfo.fAzm = *pfData++;
    trackinfo.fSpeed = *pfData++;
    trackinfo.fCourse = *pfData++;
//    qDebug()<<"===trace===\n"
//                "trackinfo.iTraIndex: " << trackinfo.iTraIndex << "\n"
//                "trackinfo.fRng: " << trackinfo.fRng << "\n"
//                "trackinfo.fAzm: " << trackinfo.fAzm << "\n"
//                "trackinfo.fSpeed(m/s): " << trackinfo.fSpeed << "\n"
//                "trackinfo.fCourse(弧度): " << trackinfo.fCourse << "\n";
    pfData++;
    return pfData;
}

// 输出所有航迹数据
int* OutTrack(int iScan, int* pTrack, std::list<TrackInfo> & trackList)
{
    char cfilename[200];
    int k = 0;
    int iTrackCnt = *pTrack++;

    float* pfData = (float*)pTrack;
    for (k = 0; k<iTrackCnt; k++)
    {
        TrackInfo trackinfo;
        pfData = OutTrackInfo(pfData, trackinfo);
        trackList.push_back(trackinfo);
    }
    return (int*)pfData;
}

void getLatLong(LatLong &A, double distance, double angle, double &lat, double &lng)
{
    double dx = distance * 1000 * sin(angle*PI / 180);
    double dy = distance * 1000 * cos(angle*PI / 180);
    double bjd = 0.0, bwd = 0.0;
    bjd = (dx / A.Ed + A.m_RadLo)*180. / PI;
    bwd = (dy / A.Ec + A.m_RadLa)*180. / PI;
    //cout<<"经纬度 bjd bwd"<<bjd<<bwd;
    LatLong tempLatLong(bjd, bwd);
    lat = tempLatLong.m_Latitude;
    lng = tempLatLong.m_Longitude;
    double y = A.Ed * (bjd*PI/180 - A.m_RadLo);
    double x= A.Ec * (bwd*PI/180 - A.m_RadLa);
    //cout<<"笛卡尔 x y"<<x<<y;
}

void getDxDy(LatLong &A, double lat, double lng, double &x, double &y)
{
    double bjd = 0.0, bwd = 0.0;
    bjd = lng;
    bwd = lat;
    y = A.Ed * (bjd*PI/180 - A.m_RadLo);
    x = A.Ec * (bwd*PI/180 - A.m_RadLa);
    //cout<<"经纬度转笛卡尔 x y"<<x<<y;

}

void getNewLatLong(LatLong &A, double &lat, double &lng, double x, double y)
{
    //cout<<"笛卡尔转经纬度 x y"<<x<<y;
    lng = (y/A.Ed + A.m_RadLo) * 180/PI;
    lat = (x/A.Ec + A.m_RadLa) * 180/PI;
}

int GetPolar(float x,float y, float *range, float *bearing)
{

    *bearing = (float) atan2fbystephen( (float) x, (float) y) * 180.0f /(float)PI ;
    if (*bearing < 0)
    {
        *bearing = *bearing + 360.0f ;
    }
    *range = sqrt( (float) (x*x + y*y) );
    return 1;
}
/*-------------------------------------------
*
* 角度转弧度
*
---------------------------------------------*/
double toRad(double deg)
{
    double pi = M_PI;

    return deg*pi/180.0;
}

/*-------------------------------------------
*
* 弧度转角度
*
---------------------------------------------*/
double toDeg(double rad)
{
    double pi = M_PI;

    return rad*180/pi;
}

/*-------------------------------------------
*
* 输入：笛卡尔坐标航迹计算速度坐标
* 输出：角度
*
---------------------------------------------*/
double calCog(double Vx, double Vy)
{
    double angle = 0.0;
    if(Vx < 0)
    {
       angle = 2*PI + atan2(Vx,Vy);
    }else{
        angle = atan2(Vx,Vy);
    }
    angle = angle/PI * 180;
    return angle;
}

/*-------------------------------------------
*
* 输入：笛卡尔坐标航迹计算速度坐标
* 输出：径向速度
*
---------------------------------------------*/
double calSog(double Vx, double Vy)
{
    double speed = 0.0;
    speed = sqrt(Vx*Vx+ Vy*Vy);
    return speed;
}

void distbearTolatlon(double lat1, double lon1, double dist, double brng, double &lat_out, double &lon_out)
{
    brng = toRad(brng);

    lat1=toRad(lat1);
    lon1=toRad(lon1);
    double R=6378000.0; //Earth radius 6372795.0
    double lat2 = asin( sin(lat1)*cos(dist/R) + cos(lat1)*sin(dist/R)*cos(brng) );
    double lon2 = lon1 + atan2(sin(brng)*sin(dist/R)*cos(lat1),cos(dist/R)-sin(lat1)*sin(lat2));

    lat_out=toDeg(lat2);
    lon_out=toDeg(lon2);
}

void distbearTolatlon1(double lat1,double lon1,double dist,double brng,double *lat_out,double *lon_out )
{
    lat1=toRad(lat1);
    lon1=toRad(lon1);
    double R=RADIUS; //Earth radius 6371000
    double lat2 = asin( sin(lat1)*cos(dist/R) + cos(lat1)*sin(dist/R)*cos(brng) );
    double lon2 = lon1 + atan2(sin(brng)*sin(dist/R)*cos(lat1),cos(dist/R)-sin(lat1)*sin(lat2));

    *lat_out=toDeg(lat2);
    *lon_out=toDeg(lon2);

}
void convertXYtoWGS(double centerLat,double centreLon,float X,float Y,double *lat,double *lon)
{
    float range=0 ; float bearing=0 ;
    GetPolar(X,Y, &range,&bearing);
    distbearTolatlon1(centerLat,centreLon,range,bearing*PI/180,lat,lon);
}

double getDisDeg(double lat1, double lon1, double lat2, double lon2)
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

    double p1_lat = toRad( p1.y() ), p1_lon = toRad( p1.x() );
    double p2_lat = toRad( p2.y() ), p2_lon = toRad( p2.x() );

    double L = p2_lon - p1_lon;
    double U1 = atan(( 1 - f ) * tan( p1_lat ) );
    double U2 = atan(( 1 - f ) * tan( p2_lat ) );
    double sinU1 = sin( U1 ), cosU1 = cos( U1 );
    double sinU2 = sin( U2 ), cosU2 = cos( U2 );
    double lambda = L;
    double lambdaP = 2 * PI;

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
        *course2 = atan2( cosU1 * sinLambda, - sinU1 * cosU2 + cosU1 * sinU2 * cosLambda ) + PI;
    }

    return s;

    //return get_distance_rad(degToRad(lat1), degToRad(lon1), degToRad(lat2), degToRad(lon2));
}


//计算重心

QPointF getCenterOfGravityPoint(double& area, const QPolygonF& list)
{
    area = 0.0;//多边形面积
    double Gx = 0.0, Gy = 0.0;// 重心的x、y
    int size = list.size();
#if 0
    for (int i = 1; i <= list.size(); i++) {
        double p1y = list[i%size].y();
        double p1x = list[i%size].x();
        double p0y = list[i-1].y();
        double p0x = list[i-1].x();
        double temp = (p0x * p1y - p0y * p1x) / 2.0;
        area += temp;
        Gy += temp * (p1y + p0y) / 3.0;
        Gx += temp * (p1x + p0x) / 3.0;
    }
    if(area > 0)
    {
        Gx = Gx / area;
        Gy = Gy / area;
    }    
#else
    double sum_x = 0.0, sum_y = 0.0;
    if(size >= 3)
    {
        QPointF p0 = list[0];
        for (int i = 1; i < list.size(); i++) {
            QPointF p1 = list[i];
            QPointF p2 = list[(i+1) % size];
            double temp = getArea(p0,p1,p2) ;
            area += temp ;
            sum_x += (p0.x() + p1.x() + p2.x()) * temp ;
            sum_y += (p0.y() + p1.y() + p2.y()) * temp ;
        }
        if(area > 0)
        {
            Gx = sum_x / area / 3;
            Gy = sum_y / area / 3 ;
        }
    }
#endif
    return QPointF(Gx, Gy);
}

QPointF getCenterOfPoint(const QPolygonF &list)
{
    return list.boundingRect().center();
}

double getArea( const QPointF& p0 , const QPointF& p1 , const QPointF& p2 )
{
    double area = 0 ;
    area =  p0.x() * p1.y() + p1.x() * p2.y() + p2.x() * p0.y() - p1.x() * p0.y() - p2.x() * p1.y() - p0.x() * p2.y();
    return fabs(area / 2) ;  // 另外在求解的过程中，不需要考虑点的输入顺序是顺时针还是逆时针，相除后就抵消了。
}


Latlon mercatorToLatlon(const Mercator& mct)
{
    double x = mct.mX/EARTH_HALF_CIRCUL_LENGTH * 180.0;
    double y = mct.mY/EARTH_HALF_CIRCUL_LENGTH * 180.0;
    y = 180/GLOB_PI*(2*atan(exp(y*GLOB_PI/180.0))-GLOB_PI/2.0);
    return Latlon(y, x);
}

Mercator latlonToMercator(const Latlon& ll)
{
    //qDebug()<<"wgs:"<<wgs84.mLon<<wgs84.mLat;
    double mx = ll.lon * EARTH_HALF_CIRCUL_LENGTH / 180;
    double my = log(tan((90 + ll.lat) * GLOB_PI / 360)) / (GLOB_PI / 180);
    my = my * EARTH_HALF_CIRCUL_LENGTH / 180;

    return Mercator(mx, my);
}

double timeOfDay()
{
    QDateTime startDate;
    startDate.setDate(QDate::currentDate());
    return startDate.secsTo(QDateTime::currentDateTime());
}


//将矩形回波点的坐标输出
void  exportRectDef2File(const zchxRadarRectDefList& list, const QString& fileName)
{
    QDir dir(QApplication::applicationDirPath());
    QString path("temp/rect");
    if(!dir.exists(path)) dir.mkpath(path);
    QString file_name= QString("%1/%2/%3.txt").arg(dir.absolutePath()).arg(path).arg(fileName);
    QFile file(file_name);
    if(!file.open(QIODevice::WriteOnly)) return;
    for(int i=0; i<list.size(); i++)
    {
        zchxRadarRectDef rect = list[i];
        QString line;
        line.append(QString("").sprintf("%.6f,%.6f,%d",rect.centerlatitude(), rect.centerlongitude(), rect.rectnumber()));
        file.write(line.toUtf8());
        file.write("\n");
    }
    file.close();
}
