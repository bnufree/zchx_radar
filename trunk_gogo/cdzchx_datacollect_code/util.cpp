#include "util.h"
#include <QtMath>
#include <QPointF>

Util::Util(QObject *parent) : QObject(parent)
{

}

QByteArray Util::HexStringToByteArray(QString HexString)
{
    bool ok;
    char c;
    QByteArray ret;
    c = HexString.mid(0,2).toInt(&ok,16)&0xFF;
    ret.append(c);
    c = HexString.mid(2,2).toInt(&ok,16)&0xFF;
    ret.append(c);
    c = HexString.mid(4,2).toInt(&ok,16)&0xFF;
    ret.append(c);
    c = HexString.mid(6,2).toInt(&ok,16)&0xFF;
    ret.append(c);
    return ret;
}

// 4字节数据长度(十六进制)
QByteArray Util::intToByte16(int i)
{
    QByteArray byte16;
    byte16.resize(4);
    byte16[3] = (i & 0xFF);
    byte16[2] = ((i>>8) & 0xFF);
    byte16[1] = ((i>>16) & 0xFF);
    byte16[0] = ((i>>24) & 0xFF);
    return byte16;
}

double Util::degToRad(double d)
{
    return ((d * GLOB_PI) / 180.0);
}

double Util::radToDeg(double r)
{
    return ((r * 180.0) / GLOB_PI);
}

double Util::getDistanceDeg(double lat1, double lon1, double lat2, double lon2)
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

    double p1_lat = degToRad( p1.y() ), p1_lon = degToRad( p1.x() );
    double p2_lat = degToRad( p2.y() ), p2_lon = degToRad( p2.x() );

    double L = p2_lon - p1_lon;
    double U1 = atan(( 1 - f ) * tan( p1_lat ) );
    double U2 = atan(( 1 - f ) * tan( p2_lat ) );
    double sinU1 = sin( U1 ), cosU1 = cos( U1 );
    double sinU2 = sin( U2 ), cosU2 = cos( U2 );
    double lambda = L;
    double lambdaP = 2 * GLOB_PI;

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
        *course2 = atan2( cosU1 * sinLambda, - sinU1 * cosU2 + cosU1 * sinU2 * cosLambda ) + GLOB_PI;
    }

    return s;
}

/**
  * 根据经纬度，距离，方位角计算下一点经纬度坐标。（方式二）计算相对精确
  * @param lon 经度
  * @param lat 纬度
  * @param dist 距离（米）
  * @param brng 方位角
  * @return double[] 0：经度 1：纬度
**/
double * Util::computerThatLonLat(double lon, double lat, double dist, double brng)
{
    /** 长半径a=6378137 */
    static double a = 6378137;
    /** 短半径b=6356752.3142 */
    static double b = 6356752.3142;
    /** 扁率f=1/298.2572236 */
    static double f = 1 / 298.2572236;

    double alpha1 = rad(brng);
    double sinAlpha1 = sin(alpha1);
    double cosAlpha1 = cos(alpha1);
    double tanU1 = (1 - f) * tan(rad(lat));
    double cosU1 = 1 / sqrt((1 + tanU1 * tanU1));
    double sinU1 = tanU1 * cosU1;
    double sigma1 = atan2(tanU1, cosAlpha1);
    double sinAlpha = cosU1 * sinAlpha1;
    double cosSqAlpha = 1 - sinAlpha * sinAlpha;
    double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
    double A = 1 + uSq / 16384 * (4096 + uSq * (-768 + uSq * (320 - 175 * uSq)));
    double B = uSq / 1024 * (256 + uSq * (-128 + uSq * (74 - 47 * uSq)));
    double cos2SigmaM = 0;
    double sinSigma = 0;
    double cosSigma = 0;
    double sigma = dist / (b * A), sigmaP = 2 * GLOB_PI;
    while (abs(sigma - sigmaP) > 1e-12) {
        cos2SigmaM = cos(2 * sigma1 + sigma);
        sinSigma = sin(sigma);
        cosSigma = cos(sigma);
        double deltaSigma = B * sinSigma * (cos2SigmaM + B / 4 * (cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM) - B / 6 * cos2SigmaM * (-3 + 4 * sinSigma * sinSigma) * (-3 + 4 * cos2SigmaM * cos2SigmaM)));
        sigmaP = sigma;
        sigma = dist / (b * A) + deltaSigma;
    }
    double tmp = sinU1 * sinSigma - cosU1 * cosSigma * cosAlpha1;
    double lat2 = atan2(sinU1 * cosSigma + cosU1 * sinSigma * cosAlpha1, (1 - f) * sqrt(sinAlpha * sinAlpha + tmp * tmp));
    double lambda = atan2(sinSigma * sinAlpha1, cosU1 * cosSigma - sinU1 * sinSigma * cosAlpha1);
    double C = f / 16 * cosSqAlpha * (4 + f * (4 - 3 * cosSqAlpha));
    double L = lambda - (1 - C) * f * sinAlpha * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM)));
    double* caculate = new double[2];
    caculate[0] = lon + deg(L);
    caculate[1] = deg(lat2);
    return caculate;
}

qint64 Util::getNextMinite(qint64 time)
{
    if (time % MINETE == 0)
    {
        return time;
    }
    else
    {
        return time - time % MINETE + MINETE;
    }
}

double Util::getAverage(double value1, double value2, qint64 offsetX, qint64 offsetY)
{
    qint64 offsetSum = offsetX + offsetY;
    if (offsetX < 0 || offsetY < 0 || offsetSum <= 0)
    {
        return 0;
    }

    return value1 + (value2 - value1) / (offsetSum) * offsetX;
}
