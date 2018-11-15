#include "zchxfunction.h"
#include <QPointF>
#include <QDebug>

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
    LatLong tempLatLong(bjd, bwd);
    lat = tempLatLong.m_Latitude;
    lng = tempLatLong.m_Longitude;
}

void getLatLong_1(LatLong &A, double distance, double angle, double &lat, double &lng, double &x, double &y)
{
    double dx = distance * 1000 * sin(angle*PI / 180) * 1.18;
    double dy = distance * 1000 * cos(angle*PI / 180) * 1.18;
    x = dx;
    y = dy;
    double bjd = 0.0, bwd = 0.0;
    bjd = (dx / A.Ed + A.m_RadLo)*180. / PI;
    bwd = (dy / A.Ec + A.m_RadLa)*180. / PI;
    LatLong tempLatLong(bjd, bwd);
    lat = tempLatLong.m_Latitude;
    lng = tempLatLong.m_Longitude;
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
