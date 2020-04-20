#include "glogfunction.h"
#include <QCryptographicHash>
#include <QPointF>
#include <QString>
#include <math.h>
#include <QRegularExpression>
#include <QTextCodec>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QtMath>


#define    REAL_PLAN_DIFF_THRESHOLD             10                  //实时和计划的设定值差异的阈值(比分比)
#define    PNT2DISTANCELINE                     500

static const uchar aucCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40
};

static const uchar aucCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
    0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
    0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
    0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
    0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
    0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
    0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80, 0x40
};

ushort GlogFunction::CRCModbus16( uchar* pucFrame, USHORT usLen )
{
    uchar ucCRCHi = 0xFF;
    uchar ucCRCLo = 0xFF;
    int iIndex;
    while( usLen-- )
    {
        iIndex = ucCRCLo ^ *( pucFrame++ );
        ucCRCLo = ( uchar )( ucCRCHi ^ aucCRCHi[iIndex] );
        ucCRCHi = aucCRCLo[iIndex];
    }
    return ( ushort )( ucCRCHi << 8 | ucCRCLo );

}



double GlogFunction::toDeg(double rad)
{
    double pi = GLOB_PI;

    return rad*180/pi;
}

double GlogFunction::ang2Cir(double angle)
{
    double pi=GLOB_PI;
    return (pi/180)*angle;
}

int  MyCross(double X1,double Y1,double X2,double Y2,double X3,double Y3,double X4,double Y4,double &x,double &y)
{
     double k1,k2;
     if((X1==X2)&&(Y1==Y2)&&(Y3==Y4)&&(X3==X4))
     {
         return 0;
     }
     if((X1==X2)&&(Y1==Y2))
     {
       if((X3==X4)&&(X1==X3))
       {
          x=X1;
          y=Y1;
          return 1;
       }
       else if(Y2==((X2-X3)*(Y4-Y3)/(X4-X3)+Y3))
       {
          x=X1;
          y=Y1;
          return 1;
       }
    }
     if((Y3==Y4)&&(X3==X4))
     {
       if((X2==X1)&&(X1==X3))
       {
         x=X3;
         y=Y3;
         return 2;
       }
       else if(Y3==((X3-X1)*(Y2-Y1)/(X2-X1)+Y1))
       {
         x=X3;
         y=Y3;
         return 2;
       }
     }
     if(X1!=X2)
     {
       k1=(Y2-Y1)/(X2-X1);
       if(X3!=X4)
       {
         k2=(Y4-Y3)/(X4-X3);
         if(k1==k2)
         {
             return 0;
         }
         x=(Y3-Y1-k2*X3+X1*k1)/(k1-k2);
         y=k1*(x-X1)+Y1;
         if(((X1-x)*(X2-x)<0||(X1-x)*(X2-x)==0)&&((Y1-y)*(Y2-y)<0||(Y1-y)*(Y2-y)==0))
           return 1;
         if(((X3-x)*(X4-x)<0||(X3-x)*(X4-x)==0)&&((Y3-y)*(Y4-y)<0||(Y3-y)*(Y4-y)==0))
           return 2;
         if((X3-x)*(X4-x)>0&&(Y3-y)*(Y4-y)>0&&(X1-x)*(X2-x)>0&&(Y1-y)*(Y2-y)>0)
           return 3;
      }
      if(X3==X4)
      {
        x=X3;
        y=k1*(X3-X1)+Y1;
        return 1;
      }
     }
     else
     {
       if(X3!=X4)
       {
         k2=(Y4-Y3)/(X4-X3);
         x=double(X1);
         y=k2*(X1-X3)+Y3;
         return 2;
       }
       if(X3==X4)
       {
          return 0;
       }
     }
     return 0;
}

Line Line::GetLine(Point ptSource, Point ptDestination)
{
    Line lTemp;
    lTemp.a = ptSource.m_pointY - ptDestination.m_pointY;
    lTemp.b = ptDestination.m_pointX - ptSource.m_pointX;
    lTemp.c = ptSource.m_pointX*ptDestination.m_pointY - ptDestination.m_pointX*ptSource.m_pointY;
    return lTemp;
}

Point Line::GetCrossPoint(Line l1, Line l2)
{
    Point pTemp;
    double D;
    D = l1.a*l2.b - l2.a*l1.b;
    Point p;
    pTemp.m_pointX = (l1.b*l2.c - l2.b*l1.c)/D;
    pTemp.m_pointY = (l1.c*l2.a - l2.c*l1.a)/D;
    return pTemp;
}


GlogFunction* GlogFunction::m_instance = NULL;
GlogFunction *GlogFunction::instance()
{
    if(m_instance == 0)
    {
        m_instance =  new GlogFunction();
    }
    return m_instance;
}

Wgs84LonLat GlogFunction::mercatorToWgs84LonLat(const Mercator& mercator)
{
    double x = mercator.mX/20037508.34*180.0;
    double y = mercator.mY/20037508.34*180.0;
    y= 180/GLOB_PI*(2*atan(exp(y*GLOB_PI/180.0))-GLOB_PI/2.0);
    return Wgs84LonLat(x, y);
}

Mercator GlogFunction::wgs84LonlatToMercator(const Wgs84LonLat& wgs84 )
{
    double x = wgs84.mLon * 20037508.34 / 180;
    double y = log(tan((90 + wgs84.mLat) * GLOB_PI / 360)) / (GLOB_PI / 180);
    y = y * 20037508.34 / 180;

    return Mercator(x, y);
}

double GlogFunction::calcTension1(double dWaterDepth, double dBaseWaterWeight, double dKpSegment, double dCableSegment)
{
    if(dKpSegment<0.001||dCableSegment<0.001)
    {
        return 0;
    }
    double dValueS = (dCableSegment-dKpSegment)/(dKpSegment);
    if(dValueS<0.00001)
    {
        return dWaterDepth*dBaseWaterWeight;
    }
    double dResult = dWaterDepth*dBaseWaterWeight*(1.0+1.0/(3.0*dValueS));
    return dResult;

}

double GlogFunction::calcTension2(double dWaterDepth, double dBaseWaterWeight, double dBaseTransFriction, double dKpSegment, double dCableSegment, double dWaterIntoAngle, double dDeltaTension)
{
    if(dKpSegment<0.001||dCableSegment<0.001)
    {
        return 0;
    }
    double dValueS = (dCableSegment-dKpSegment)/(dKpSegment);
    double dValueL = dCableSegment;
    double dValueL0 = dCableSegment/sin(ang2Cir(dWaterIntoAngle));
    double dSetaS = 1.0/(cos(1.0/sqrt(1.0+(dValueL0/(dBaseTransFriction*dValueL))*(dValueL0/(dBaseTransFriction*dValueL)))));
    double dAlfa1 = 1.0/(1.0-cos(dSetaS));
    double dResult = dAlfa1*dValueS*dBaseTransFriction*dBaseWaterWeight*dWaterDepth+dDeltaTension;
    return dResult;
}

GlogFunction::~GlogFunction()
{
    if(m_instance)
    {
        delete m_instance;
        m_instance = NULL;
    }
}

GlogFunction::GlogFunction(QObject *parent)
{

}


double GlogFunction::degToRad(double d)
{
    return ((d * GLOB_PI) / 180.0);
}

double GlogFunction::radToDeg(double r)
{
    return ((r * 180.0) / GLOB_PI);
}

/*-------------------------------------------
    *
    * 已知两点坐标，求两点连线的方位角
    *
    ---------------------------------------------*/
double GlogFunction::calcAzimuth(double lon1, double lat1, double lon2, double lat2)
{
    double arvLat,antPos;
    double pi= GLOB_PI;
    arvLat=(lat1+lat2)/2;
    if((lat1-lat2)==0){
        if(lon1 == lon2)
        {
            antPos = 0;
        } else if(lon2 > lon1)
        {
            antPos=90;
        } else
        {
            antPos = 270;
        }
    }else{
        antPos=atan((lon1-lon2)*cos(ang2Cir(arvLat))/(lat1-lat2))*180/pi;
    }

    if(lat1>lat2){
        antPos+=180;
    }

    if(antPos<0){
        antPos+=360;
    }
    //方位角保留三位小数
    antPos*=1000;
    antPos=int(antPos+0.5);
    antPos/=1000;

    return antPos;
}


double GlogFunction::getDistanceDeg(double lat1, double lon1, double lat2, double lon2)
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
        // PI is added to return azimuth from P2 to P1
        *course2 = atan2( cosU1 * sinLambda, - sinU1 * cosU2 + cosU1 * sinU2 * cosLambda ) + GLOB_PI;
    }

    return s;

    //return get_distance_rad(degToRad(lat1), degToRad(lon1), degToRad(lat2), degToRad(lon2));
}

double GlogFunction::getDistanceDeg(const Wgs84LonLat &p1, const Wgs84LonLat &p2)
{
    return getDistanceDeg(p1.mLat, p1.mLon, p2.mLat, p2.mLon);
}

double GlogFunction::strCastLoc(const QString &str1)
{
    double ret = 0.0;

    QString str = str1.trimmed();
    QRegExp reg1("^(-?\\d+)(\\.\\d+)?$");
    if(3 == str.split(' ').size())//如果是格式为 度 分
    {
        QStringList temp = str.split(' ');
        ret = ret + temp.at(0).toDouble();
        ret = ret + temp.at(1).toDouble()/60;
    }
    else

    {
        if(reg1.exactMatch(str))
        {
            bool ok = false;
            double temp = str.toDouble(&ok);
            if(ok)
            {
                temp = temp / 100.00000;
                ret = (int)temp+(temp-(int)temp) *100.0000 / 60.00000;
            }
        }
        else
        {
            QString tmp_str[5];
            int index = 0;

            for (int i = 0; i < str.size(); ++i) {
                if ( (str.at(i) >= QChar('0') && str.at(i) <= QChar('9')) || str.at(i) == QChar('.'))
                {
                    tmp_str[index] += str.at(i);
                }
                else
                {
                    //qDebug() << tmp_str[index];
                    if(++index>=3)
                    {
                        break;
                    }
                }
            }

            ret = tmp_str[0].toDouble() + tmp_str[1].toDouble()/60 + tmp_str[2].toDouble()/3600;

            if ( (-1 != str.indexOf("S")) || (-1 != str.indexOf("W")) )
            {
                ret *= -1;
            }
        }

    }
    return ret;
}
//
QString GlogFunction::latLon2String(double coordinate)
{
    const double num = 60;
    int deg = (int)coordinate;
    double tmp = (coordinate -deg) * num;
    int min = (int)tmp;
    int sed = (tmp - min) * num;

    QString coordinateString = QString::number(deg)+("°")+QString::number(min)+("′")+QString::number(sed,'f',2)+("″");
    return coordinateString;
}
QString GlogFunction::latLon2String_new(double coordinate)
{
    const double num = 60;
    int deg = (int)coordinate;
    double tmp = (coordinate -deg) * num;
    int min = (int)tmp;
    double sed = (tmp - min) * num;
    QString coordinateString = QString::number(deg)+("°")+QString::number(tmp,'f',4)+("′");
    return coordinateString;
}

QString GlogFunction::latLon2String(double coordinate, bool latflag, int mode)
{
    //首先判断经纬度数值的符号,确定前端的字符(N/S, W/E)
    QString mark = "";
    int wkval = (int)(floor(coordinate));
    if(wkval < 0)
    {
        mark = latflag ? "S" : "W";
    } else
    {
        mark = latflag ? "N" : "E";
    }
    if(mode == 1)
    {
        int deg = (int)coordinate;
        double min = (coordinate-deg) * 60;
        double decimal = min - (int)min;
        return QString("").sprintf("%s%d %02d.%04d", mark.toStdString().data(), deg, (int)min, (int)(round(decimal *10000)));
    }
    return QString("").sprintf("%s%.6f", mark.toStdString().data(), coordinate);


}

QString GlogFunction::latLon2StringDMS(double coordinate, bool latflag)
{
    //首先判断经纬度数值的符号,确定前端的字符(N/S, W/E)
    QString mark = "";
    int wkval = (int)(floor(coordinate));
    if(wkval < 0)
    {
        mark = latflag ? "S" : "W";
    } else
    {
        mark = latflag ? "N" : "E";
    }

    int deg = (int)coordinate;
    double min = (coordinate-deg) * 60;
    double decimal = (min - (int)min) * 60;
    return QString("").sprintf("%s%d°%d'%.3f\"", mark.toStdString().data(), deg, (int)min, decimal);
}

LLStu GlogFunction::latLon2String_split(const double &coordinate)
{
    const double num = 60;
    int deg = (int)coordinate;
    double tmp = (coordinate -deg) * num;
    int min = (int)tmp;
    double sed = (tmp - min) * num;
    //    int deg = (int)coordinate;
    //    int min = (int)((coordinate-deg)*60);
    //    int sed = (int)(((coordinate - deg)*60 - min ) * 60);
    LLStu stu;
    stu.d = deg;
    stu.m = min;
    stu.s = sed;
    return stu;
}

double GlogFunction::latLon2Double(int d, int m, int s)
{
    //    qDebug()<<"dd:"<<d<<",mm:"<<m<<",ss:"<<s;

    const double num = 60;
    const double num1 = 3600;
    return d + (m / num) + (s / num1);
}

double GlogFunction::getDistance(double X1, double Y1, double X2, double Y2)
{
    double DistanceResult = 0;
    double PowValue = pow(X2-X1,2)+pow(Y2-Y1,2);
    DistanceResult = sqrt(PowValue);
    return DistanceResult;
}

double GlogFunction::getAngle(double X1, double Y1, double X2, double Y2)
{
    double Angle;
    if(fabs(X2)==fabs(X1))
    {
        if(Y2<Y1)
        {
           Angle = 180;
        }
        else if(Y2>=Y1)
        {
           Angle = 0;
        }
    }
    else if(fabs(Y2)==fabs(Y1))
    {
        if(X2<X1)
        {
           Angle = 270;
        }
        else if(X2>X1)
        {
           Angle = 90;
        }
        else
        {
           Angle = 0;
        }
    }
    else
    {
        if(X2<X1 && Y2 <Y1)
        {
            double TanValue = fabs(Y2-Y1)/fabs(X2-X1);
            Angle = radToDeg(atan(TanValue))+180;
        }
        else if(X2<X1 && Y2 >Y1)
        {
            double TanValue = fabs(Y2-Y1)/fabs(X2-X1);
            Angle = radToDeg(atan(TanValue))+270;
        }
        else if(X2>X1 && Y2 <Y1)
        {
            double TanValue = fabs(Y2-Y1)/fabs(X2-X1);
            Angle = radToDeg(atan(TanValue))+90;
        }
        else if(X2>X1 && Y2 >Y1)
        {
            double TanValue = fabs(Y2-Y1)/fabs(X2-X1);
            Angle = 90-radToDeg(atan(TanValue));
        }
    }
    return Angle;
}

bool GlogFunction::CalculatXY(double X1, double Y1, double distance, double angle, double &x_out, double &y_out)
{
    if(angle<90)
    {
        double radValue = GlogFunction::instance()->degToRad(angle);         //角度转弧度
        double OnTheEdge = sin(radValue)*distance;    //对边
        double AdjacentEdges = sin(radValue)*distance;//邻边
        x_out = X1+AdjacentEdges;
        y_out = Y1+OnTheEdge;
    }
    else if(angle==90)
    {
        x_out = X1+distance;
        y_out = Y1;
    }
    else if(angle>90 &&angle<180)
    {
        double radValue = GlogFunction::instance()->degToRad(angle-90);         //角度转弧度
        double OnTheEdge = sin(radValue)*distance;    //对边
        double AdjacentEdges = sin(radValue)*distance;//邻边
        x_out = X1+AdjacentEdges;
        y_out = Y1+OnTheEdge;
    }
    else if(angle == 180)
    {
        x_out = X1;
        y_out = Y1-distance;
    }
    else if(angle>180 && angle<270)
    {
        double radValue = GlogFunction::instance()->degToRad(angle-180);         //角度转弧度
        double OnTheEdge = sin(radValue)*distance;    //对边
        double AdjacentEdges = sin(radValue)*distance;//邻边
        x_out = X1-AdjacentEdges;
        y_out = Y1-OnTheEdge;
    }
    else if(angle == 270)
    {
        x_out = X1- distance;
        y_out = Y1;
    }
    else if(angle>270 && angle<360)
    {
        double radValue = GlogFunction::instance()->degToRad(angle-270);         //角度转弧度
        double OnTheEdge = sin(radValue)*distance;    //对边
        double AdjacentEdges = sin(radValue)*distance;//邻边
        x_out = X1-AdjacentEdges;
        y_out = Y1-OnTheEdge;
    }
    return true;
}

QString GlogFunction::secToTime(const int &durationSeconds)
{
    int hours = durationSeconds /(60*60);
    int leftSeconds = durationSeconds % (60*60);
    int minutes = leftSeconds / 60;
    int seconds = leftSeconds % 60;
    return QString("%1:%2:%3").arg(hours).arg(minutes).arg(seconds);
}

/**
 * @brief GlogFunction::md5
 * @param str
 * @return
 * 转换成md5
 */
QString GlogFunction::md5(const QString &str)
{
    QString m = str;
    QByteArray hash_byte_array = QCryptographicHash::hash(m.toLocal8Bit(), QCryptographicHash::Md5);
    return hash_byte_array.toHex();

}

//***********************************************************************
//Description: NMEA格式数据校验和计算
//inputs: string str  NMEA格式数据字符'$'和'*'之间字符异或
//***********************************************************************
std::string GlogFunction::cmp_chksum(std::string str)
{
//    return true;
//    size_t pos = str.find('*');
//    // 没找到*符号 或者 接收缓存字符长度不够--5 char is *hhCRLF
//    if(std::string::npos == pos || str.length() != pos+5)
//    {
//        qDebug() << "string len error";
//        return false;
//    }
    int pos = str.length();
    if(pos < 6)
    {
        return "err1";
    }
    //计算字符串校验和是否正确
    int c_cal = 0;
    for(int i=1; i<pos; i++)
    {
        c_cal ^= str[i];
    }
   // qDebug()<<"==========================================>>";
   // qDebug()<<str.c_str()<<"cal = " << c_cal << QString::number(c_cal, 16);
   // qDebug()<<"<<==========================================";
    return str + "*" + QString::number(c_cal,16).toStdString();
}

//***********************************************************************
//Description: acii hex->int convert
//***********************************************************************
int GlogFunction::from_hex(char a)
{
    if (a >= 'A' && a <= 'F')
        return a - 'A' + 10;
    else if (a >= 'a' && a <= 'f')
        return a - 'a' + 10;
    else
        return a - '0';
}


/*通过与前一个经纬度点的距离和方位求出该点的经纬度
**lat1-前一个纬度
**lon1-前一个经度
**dist-距离
**brng-方位
**lat_out-输出纬度
**lon_out-输出经度*/
void GlogFunction::distbear_to_latlon(double lat1, double lon1, double dist, double brng, double &lat_out, double &lon_out)
{


    brng = degToRad(brng);

    lat1=degToRad(lat1);
    lon1=degToRad(lon1);
    double R=6378000.0; //Earth radius 6372795.0
    double lat2 = asin( sin(lat1)*cos(dist/R) + cos(lat1)*sin(dist/R)*cos(brng) );
    double lon2 = lon1 + atan2(sin(brng)*sin(dist/R)*cos(lat1),cos(dist/R)-sin(lat1)*sin(lat2));

    lat_out=radToDeg(lat2);
    lon_out=radToDeg(lon2);
}

//数字的分组显示(1,000,000)
QString GlogFunction::int2grp(int num)
{
    QString mark = num < 0? "-":"";
    int wknum = num;
    if(num < 0) wknum *= (-1);
    QStringList reslist;
    do{
        int b = 0;
        if(wknum >= 1000) b = 3;
        reslist.insert(0, QString("").sprintf("%0*d",b, wknum % 1000));
        wknum = wknum / 1000;
    }while(wknum);

    return mark + reslist.join(",");
}

QString GlogFunction::double2grp(double num)
{
    int intval = int(num);
    double deci = num - intval;
    QString intstr = int2grp(intval);
    QString decistr = QString("").sprintf("%.2f", deci).right(2);
    return QString("").sprintf("%s.%s", intstr.toStdString().data(), decistr.toStdString().data());
}
//将N10 59.085467等转换为double型的经纬度
double GlogFunction::latLon2Double(const QString& latlonstr)
{
    QString wkstr = latlonstr;
    int positive = 1;
    //首先判断是否NWSE标记经纬度
    if(latlonstr.contains(QRegularExpression("[NWSE]")))
    {
        if(!latlonstr.contains(QRegularExpression("[NE]")))
        {
            positive = -1;
        }

        wkstr.remove(QRegularExpression("[NWSE]"));
    }

    QStringList valist = wkstr.split(QRegularExpression(" "));
    if(valist.length() < 2) return 0.00;
    int deg = valist[0].toInt();
    double decimal = valist[1].toDouble() / 60.0;
    return deg + decimal;
}

double GlogFunction::latLonStrToDouble(const QString &sLonLatStr)
{
    QString wkstr = sLonLatStr;
    double dResult;
    int positive = 1;
    //首先判断是否NWSE标记经纬度
    if(sLonLatStr.contains(QRegularExpression("[NWSE]")))
    {
        if(!sLonLatStr.contains(QRegularExpression("[NE]")))
        {
            positive = -1;
        }

        wkstr.remove(QRegularExpression("[NWSE]"));
    }
    bool bLonDeg = wkstr.contains("°");
    bool bLonMin = wkstr.contains("′");
    bool bLonSec = wkstr.contains("″");
    if(!bLonDeg&&!bLonMin&&!bLonSec)//度格式
    {
        dResult = wkstr.toDouble();
        dResult = dResult*positive;
        return dResult;
    }
    if(bLonDeg&&bLonMin&&!bLonSec)//度分
    {
        double dDeg = 0;
        double dMin = 0;
        QString sTemp = wkstr;
        int uDegIndex = sTemp.indexOf("°");
        if(uDegIndex>0)
        {
            QString sDegStr = (sTemp.left(uDegIndex));
            dDeg = sDegStr.trimmed().toDouble();
            sTemp = sTemp.remove(0,uDegIndex+1);
        }
        int uMinIndex = sTemp.indexOf("′");
        if(uMinIndex>0)
        {
            QString sMinStr = (sTemp.left(uMinIndex));
            dMin = sMinStr.trimmed().toDouble();
        }
        dResult = dDeg+(dMin/60.0);
        dResult = dResult*positive;
        return dResult;
    }
    if(bLonDeg&&bLonMin&&bLonSec)//度分秒
    {
        double dDeg = 0;
        double dMin = 0;
        double dSec = 0;
        QString sTemp = wkstr;
        int uDegIndex = sTemp.indexOf("°");
        if(uDegIndex>0)
        {
            QString sDegStr = (sTemp.left(uDegIndex));
            dDeg = sDegStr.trimmed().toDouble();
            sTemp = sTemp.remove(0,uDegIndex+1);
        }
        int uMinIndex = sTemp.indexOf("′");
        if(uMinIndex>0)
        {
            QString sMinStr = (sTemp.left(uMinIndex));
            dMin = sMinStr.trimmed().toDouble();
            sTemp = sTemp.remove(0,uMinIndex+1);
        }
        int uSecIndex = sTemp.indexOf("″");
        if(uSecIndex>0)
        {
            QString sSecStr = (sTemp.left(uSecIndex));
            dSec = sSecStr.trimmed().toDouble();
        }
        dResult = dDeg+(dMin/60.0)+(dSec/3600);
        dResult = dResult*positive;
        return dResult;
    }
}

int GlogFunction::fmtstr2Int(const QString &str)
{
    //1,000,000*
    QString wkstr = str;
    return wkstr.remove(QRegularExpression("[,\*]")).toInt();

}

double GlogFunction::fmtstr2Double(const QString &str)
{
    //1,000.000*
    QString wkstr = str;
    return wkstr.remove(QRegularExpression("[,\*]")).toDouble();
}

QString GlogFunction::GBK2UTF8(const QString &src)
{
    QTextCodec *gbk = QTextCodec::codecForName("GB18030");
    QTextCodec *utf8 = QTextCodec::codecForName("UTF8");
    return utf8->fromUnicode(gbk->toUnicode(src.toStdString().c_str()));
}

QString GlogFunction::UTF82GBK(const QString &src)
{
    QTextCodec *gbk = QTextCodec::codecForName("GB18030");
    QTextCodec *utf8 = QTextCodec::codecForName("UTF8");
    return gbk->fromUnicode(utf8->toUnicode(src.toStdString().c_str()));
}

double      GlogFunction::calIncluedAng(const QVector2D &v1, const QVector2D &v2)
{
    return acos(QVector2D::dotProduct(v1, v2) / (v1.length() * v2.length()));
}

Wgs84LonLat GlogFunction::calProjectionPnt(const Wgs84LonLat &target,\
                                           const Wgs84LonLat &line_start,\
                                           const Wgs84LonLat &line_end)
{
    //将坐标值转换到平面坐标系进行
    Mercator p0 = wgs84LonlatToMercator(target);
    Mercator p1 = wgs84LonlatToMercator(line_start);
    Mercator p2 = wgs84LonlatToMercator(line_end);
 //   qDebug()<<"SRC:"<<p0.mX<<p0.mY<<p1.mX<<p1.mY<<p2.mX<<p2.mY;
    //计算向量P1P0和P1P2的夹角
    QVector2D v10(p0.mX - p1.mX, p0.mY - p1.mY);
    QVector2D v12(p2.mX - p1.mX, p2.mY - p1.mY);
 //   qDebug()<<"VEC:"<<v10<<v12;
    double inc_angle = calIncluedAng(v10,v12);
//    qDebug()<<"incled angle:"<<inc_angle;
    //计算起点到投影点的距离
    double length = v10.length() * cos(inc_angle);
    //计算直线的角度
    double ang = atan2(v12.y(), v12.x());
//    qDebug()<<"len:"<<length<<" angle:"<<ang;
    //计算投影点的坐标
    return mercatorToWgs84LonLat(Mercator(p1.mX + length * cos(ang), p1.mY + length * sin(ang)));
}

//这里只是近似估算的直线距离
double GlogFunction::calDisOfPnt2Line(double x, double y, double p1x, double p1y, double p2x, double p2y)
{
    //算三点构成的两个向量的夹角。距离d = |a| * sin(@)
    double a1 = calcAzimuth(p1x, p1y, p2x, p2y);
    double a2 = calcAzimuth(p1x, p1y, x, y);
    double suba = degToRad(fabs(a2-a1));
    return  getDistanceDeg(p1y, p1x, y, x) * sin(suba);
}

//这里采用投影的方法算出投影点再求距离
double GlogFunction::calDisOfPnt2Line2(double x, double y, double p1x, double p1y, double p2x, double p2y)
{
    //算出投影点
    Wgs84LonLat project = calProjectionPnt(Wgs84LonLat(x,y), Wgs84LonLat(p1x, p1y), Wgs84LonLat(p2x, p2y));
    return getDistanceDeg(y, x, project.mLat, project.mLon);
}

double GlogFunction::calDisOfPntDivLine(double x, double y, double p1x, double p1y, double p2x, double p2y)
{
    //算三点构成的两个向量的夹角。距离d = |a| * sin(@)
    double a1 = calcAzimuth(p1x, p1y, p2x, p2y);
    double a2 = calcAzimuth(p1x, p1y, x, y);
    double suba = degToRad(fabs(a2-a1));
    return  getDistanceDeg(p1y, p1x, y, x) * cos(suba);
}

double GlogFunction::calDisOfPntDivLine2(double x, double y, double p1x, double p1y, double p2x, double p2y)
{
    Wgs84LonLat project = calProjectionPnt(Wgs84LonLat(x,y), Wgs84LonLat(p1x, p1y), Wgs84LonLat(p2x, p2y));
    return getDistanceDeg(p1y, p1x, project.mLat, project.mLon);
}

bool  GlogFunction::checkDouble(double real, double plan, double limit, NUM_COMPARE_MODE mode)
{
    if(mode == COMPARE_RELATIVE)
        return fabs(real - plan) <= limit;
    else if(mode == COMPARE_MINUS)
        return (real - plan) <= limit;
    else if(mode == COMPARE_PLUS)
        return (real - plan) >= limit;

    return true;
}

bool  GlogFunction::checkInt(qint64 real, qint64 plan, double limit, NUM_COMPARE_MODE mode)
{
    if(mode == COMPARE_RELATIVE)
        return abs(real - plan) <= limit;
    else if(mode == COMPARE_MINUS)
        return (real - plan) <= limit;
    else if(mode == COMPARE_PLUS)
        return (real - plan) >= limit;

    return true;
}


QString GlogFunction::FFColor2RGB(const QString& src)
{
    QString wkstr = src;
    if(wkstr.contains("#"))
    {
        wkstr = wkstr.remove(QRegularExpression("[#]"));
    }

    QStringList res;
    if(wkstr.length() == 6)
    {
        for(int i=0; i<6; i = i+2)
        {
            res.append(QString::number(QByteArray(wkstr.mid(i, 2).toStdString().data()).toInt(0, 16)));
        }
    } else
    {
        res<<"0"<<"0"<<"0";
    }

    return res.join(",");

}

QString GlogFunction::RGBColor2FF(const QString& src)
{
    QStringList srclist = src.split(QRegularExpression("[, ]"));
    QStringList res;
    res.append("#");

    if(srclist.length() == 3)
    {
        for(int i=0; i<3; i++)
        {
            QString wkstr =  QString::number(srclist[i].toInt() & 0xFF, 16 );
            if(wkstr.length() == 1)
            {
                wkstr.insert(0, "0");
            }
            res.append(wkstr);
        }
    } else
    {
        res<<"00"<<"00"<<"00";
    }

    return res.join("");
}

QColor GlogFunction::colorFromRGBString(const QString &src)
{
    int r, g, b;
    QStringList list = src.split(",");
    if(list.length() >= 3)
    {
        r = list[0].toInt();
        g = list[1].toInt();
        b = list[2].toInt();
    }
    return QColor(r, g, b);
}

#if 0
double GlogFunction::calculateCableWaterEntryAngle(const double dShipSpeed, const double cableDiameter, const double cableWaterWeight, const double constant)
{

    double parameterOne = 2*9.8*cableWaterWeight;
    double parameterTwo = constant*cableDiameter*1024*9.8;
    double declineSpeed = sqrt(parameterOne/parameterTwo);
    double dWaterEntryAngle = atan2(declineSpeed, dShipSpeed)*57.2956;
    return dWaterEntryAngle;
}

#endif

double GlogFunction::convertSpeedMS2Knots(double speed)
{
    return speed / GLOB_SPEED_MS2KNOT;
}

double GlogFunction::convertSpeedMS2DegreeKnots(double speed)
{
    return convertSpeedMS2Knots(speed) * GLOB_DEGPERRAD;
}

double GlogFunction::calculateCableWaterEntryAngle(const double dShipSpeed, const double dCableSinkSpeed)
{
    //先假定入水角比较小的情况
    double WaterAngle = 90.0;
    if(dShipSpeed > 0.00001)
    {
        WaterAngle = convertSpeedMS2DegreeKnots(dCableSinkSpeed) / convertSpeedMS2Knots(dShipSpeed);
        if(WaterAngle >=  10.0)
        {
            //使用一元二次方程计算角度
            //Vship^2 * cosa^2 + Vcable^2*Cosa - Vship2 = 0
            double a = dShipSpeed * dShipSpeed;
            double b = dCableSinkSpeed * dCableSinkSpeed;
            double c = dShipSpeed * dShipSpeed * (-1);
            double x = (sqrt(b*b-4*a*c) - b) * 0.5 / a;
            WaterAngle = acos(x) * GLOB_DEGPERRAD;
        }
    }

    return WaterAngle;
}

double GlogFunction::calShipSpeed(double sink_speed, double cable_angle)
{
    //Vsink^2 * cosa = Vship^2 * Sina^2
    double a = degToRad(cable_angle);
    return sqrt(sink_speed * sink_speed * cos(a) / (sin(a) * sin(a)));
}

double GlogFunction::calculateCableWaterEntryAngleAtan2(const double dShipSpeed, const double dCableSinkSpeed)
{
    //先假定入水角比较小的情况
    return atan2(dCableSinkSpeed, dShipSpeed) * GLOB_DEGPERRAD;
}

//这里注意沉降速度计算 水密度为1025kg/m3
//船速m/s　海缆直径ｍ海缆湿重kg/m 阻力系数1.2不能乱设
double GlogFunction::NewCalculateCableWaterEntryAngle(const double dShipSpeed, const double cableDiameter, const double cableWaterWeight, const double constant, const double FluifDensity)
{
    double parameterOne = 2*9.8*cableWaterWeight;
    double parameterTwo = constant*cableDiameter*FluifDensity/**9.8*/;
    double declineSpeed = sqrt(parameterOne/parameterTwo);
    double dWaterEntryAngle = atan2(declineSpeed, dShipSpeed)*57.29578;//弧度转度
//    double value1 = pow(declineSpeed,4)+4*pow(dShipSpeed,4);
//    double value2 = sqrt(value1)-pow(declineSpeed,2);
//    double value3 = value2/(2*pow(dShipSpeed,2));
//    double dWaterEntryAngle = acos(value3);
    return dWaterEntryAngle;
}

double GlogFunction::calculateCableSinkSpeed(const double cableDiameter, \
                                             const double cableWaterWeight,\
                                             const double constant,\
                                             const double dWaterDensity)
{
    double dUpValue = 2.0 * GLOB_GRAVITY_ACCE * cableWaterWeight;
    double dDownValue = constant * cableDiameter * dWaterDensity;
    double dSinkSpeed = sqrt(dUpValue/dDownValue);
    return dSinkSpeed;
}

//秒数转马凯的时间显示格式互转
int     GlogFunction::msecs2MakaiDay(qint64 msecs)
{
    int secs = msecs / 1000;
    return secs / (3600 * 24) + 1;
}

QString GlogFunction::msecs2MakaiStringHHMMSS(qint64 msecs)
{
    qint64 secs = msecs / 1000;
    qint64 sec = secs % 60;
    qint64 minute = (secs / 60) % 60;
    qint64 hour = (secs /3600);

    return QString("").sprintf("%02d:%02d:%02d",hour, minute, sec);
}

QString GlogFunction::msecs2MakaiStringDDHHMMSS(qint64 msecs)
{
    //day,hh:mm:ss
    qint64 secs = msecs / 1000;
    qint64 sec = secs % 60;
    qint64 minute = (secs / 60) % 60;
    qint64 hour = (secs /3600) % 24;
    qint64 day = secs / (3600 * 24) + 1;

    return QString("").sprintf("%d,%02d:%02d:%02d", day, hour, minute, sec);
}

QString GlogFunction::msecs2MakaiString(qint64 msecs)
{
    //检查秒数是否大于1天
    if(msecs >= 3600*1000*24)
    {
        return msecs2MakaiStringDDHHMMSS(msecs);
    }
    return msecs2MakaiStringHHMMSS(msecs);
}

QString GlogFunction::msecs2MakaiStringOutput(qint64 msecs)
{
    //检查秒数是否大于1天
    if(msecs >= 3600*1000*24)
    {
        return msecs2MakaiStringDDHHMMSSOutPut(msecs);
    }
    return msecs2MakaiStringHHMMSS(msecs);
}

QString GlogFunction::msecs2MakaiStringDDHHMMSSOutPut(qint64 msecs)
{
    return "\"" + msecs2MakaiStringDDHHMMSS(msecs)+"\"";
}

qint64 GlogFunction::makaiStringDDHHMMSS2MSecs(const QString& str)
{
    //10,00:00:00
    qint64 total_secs = 0;
    QRegExp reg("([0-9]{1,}),([0-9]{2}):([0-9]{2}):([0-9]{2})");
    int index = reg.indexIn(str, 0);
    if(index >=0)
    {
        qint64 secs = reg.cap(4).toInt();
        qint64 minute = reg.cap(3).toInt();
        qint64 hour = reg.cap(2).toInt();
        qint64 day = reg.cap(1).toInt() - 1;

        total_secs = day * 3600 *24 + hour * 3600 + minute * 60 + secs;
    }

    return total_secs * 1000;
}

qint64 GlogFunction::makaiStringHHMMSS2MSecs(const QString& str)
{
    //100:00:00
    qint64 total_secs = 0;
    QRegExp reg("([0-9]{2}):([0-9]{2}):([0-9]{2})");
    int index = reg.indexIn(str, 0);
    if(index >=0)
    {
        qint64 secs = reg.cap(3).toInt();
        qint64 minute = reg.cap(2).toInt();
        qint64 hour = reg.cap(1).toInt();
        total_secs =  hour * 3600 + minute * 60 + secs;
    }

    return total_secs * 1000;
}

qint64 GlogFunction::makaiString2MSecs(const QString &str)
{
    //检查是否含有","
    if(str.contains(","))
    {
        return makaiStringDDHHMMSS2MSecs(str);
    }

    return makaiStringHHMMSS2MSecs(str);
}

bool GlogFunction::isPntNear(double& dis, const QPointF &refer, const QPointF &test, double limit)
{
    dis = getDistanceDeg(refer.y(), refer.x(), test.y(), test.x());
    return dis < limit;
}

bool GlogFunction::isPntAhead(const QPointF &target, const QPointF &p1, const QPointF &p2)
{
    //首先判断点是否在起点P1法线方向的右侧
    //计算线段的方位角α1和起点和到待测点的方位角α2，如果两者的差值在90的范围内，则是在右侧，否则不是
    double a1 = calcAzimuth(p1.x(), p1.y(), p2.x(), p2.y());
    double a2 = calcAzimuth(p1.x(), p1.y(), target.x(), target.y());
    double sub = a2 - a1;
    if(sub > 180)
    {
        sub -= 360;
    } else if(sub < -180)
    {
        sub += 360;
    }

//    qDebug()<<a1<<"  "<<a2<<" "<<sub;

    return fabs(sub) <= 90;

}

bool GlogFunction::isPntOnLine(const QPointF &target, const QPointF &p1, const QPointF &p2, int &pos,  double& a1, double& a2)
{
    //同一个点的情况
    if(getDistanceDeg(p1.y(), p1.x(), p2.y(), p2.x()) < 1 ) return false;
    //检查点是否在有效区域内（线段的范围）
    bool sts1 = isPntAhead(target, p1, p2);
    bool sts2 = isPntAhead(target, p2, p1);
    if(!(sts1 && sts2)) return false;
    //计算线段的方位角α1和起点和到待测点的方位角α2，如果α2>α1,则待测点在右侧，若α2<α1,则在左侧，否则精准的在线段上
    a1 = calcAzimuth(p1.x(), p1.y(), p2.x(), p2.y());
    a2 = calcAzimuth(p1.x(), p1.y(), target.x(), target.y());
    double sub = a2 - a1;
    //当前点线段上
    if(fabs(sub) < 0.0001)
    {
        pos = 0;
    } else
    {
        if(sub < 0) sub += 360;
        if(sub < 180)
        {
            pos = 2;
        } else
        {
            pos = 1;
        }
    }

    return true;
}

bool GlogFunction::isSamePoint(const Wgs84LonLat &p1, const Wgs84LonLat &p2)
{
    return p1==p2;
}

PNTPOSTION GlogFunction::pointPosOfLine(double& pnt2line_dis, \
                                        double& pntDivline_dis, \
                                        const Wgs84LonLat &target, \
                                        const Wgs84LonLat &start, \
                                        const Wgs84LonLat &end)
{
    //检查目标直线是不是一个点的情况
    double length = getDistanceDeg(start.mLat, start.mLon, end.mLat, end.mLon);
    if(length < 1 )
    {
        return POS_UNDETERMINED;
    }
    //检查目标点是否在线段的端点上
    if(isSamePoint(target, start))
    {
        pnt2line_dis = 0.0;
        pntDivline_dis = 0.0;
        return POS_ON_VERTEX;
    } else if(isSamePoint(target, end))
    {
        pnt2line_dis = 0.0;
        pntDivline_dis = length;
        return POS_ON_VERTEX;
    }
    //开始判断点的位置关系
    //如果想判断一个点是否在线段上，那么要满足以下两个条件：
    //（1）（Q - P1） * （P2 - P1）= 0；
    //（2）Q在以P1，P2为对角顶点的矩形内；
    //第一点通俗点理解就是要求Q、P1、P2三点共线；当第一个满足后，就应该考虑是否\
    //会出现Q在P1P2延长线或反向延长线这种情况。此时第     二个条件就对Q点的横纵\
    //坐标进行了限制，要求横纵坐标要在P1P2两点的最小值和最大值之间，也就是说保证\
    //了Q在P1P2之间。
    //叉积的结果还是一个向量，二维向量的叉积是垂直于两个向量形成的平面的一个向\
    //量。这行公式实际上求的是标量。
    //计算是这样的，对于向量a（x1，y1），b（x2，y2）
    //他们的叉积a×b=x1y2-y1x2
    //如果向量AP×AB的叉积为正，则向量AP在向量AB的顺时针方向，反之为逆时针方向，
    //当两个向量的乘积为0的时候，A，B，P三点共线
    //将点转换成平面的点
    Mercator p0 = wgs84LonlatToMercator(target);
    Mercator p1 = wgs84LonlatToMercator(start);
    Mercator p2 = wgs84LonlatToMercator(end);
    //检查目标点的坐标是否在线段外
    double max_x = p1.mX < p2.mX ? p2.mX : p1.mX;
    double min_x = p1.mX < p2.mX ? p1.mX : p2.mX;
    if(p0.mX < min_x || p0.mX > max_x)
    {
        //点在线段外
        return POS_UNDETERMINED;
    }

    //计算向量P1P0和P1P2
    QVector2D v10(p0.mX - p1.mX, p0.mY - p1.mY);
    QVector2D v12(p2.mX - p1.mX, p2.mY - p1.mY);
    //计算向量的叉积
    double cross_product = v10.x() * v12.y() - v10.y() * v12.x();
    //点在线段上
    if(fabs(cross_product) <= DOUBLE_EPS)
    {
        pnt2line_dis = 0.0;
        pntDivline_dis = getDistanceDeg(target, start);
        if(pntDivline_dis < 1 || fabs(pntDivline_dis - length) < 1)
        {
            return POS_ON_VERTEX;
        }
        return POS_ON_LINE;
    } else
    {
        //算出投影点
        Wgs84LonLat project = calProjectionPnt(target, start, end);
        pnt2line_dis = getDistanceDeg(target, project);
        pntDivline_dis = getDistanceDeg(start, project);
        if(pnt2line_dis <= PNT2DISTANCELINE)
        {
            return cross_product > DOUBLE_EPS? POS_RIGHT:POS_LEFT;
        }
    }
    return POS_UNDETERMINED;
}

bool GlogFunction::calPlowPostion(double tow_wrie_out, double water_depth, double ship_course, double ship_kp, double ship_lon, double ship_lat, double &plow_kp, double &plow_lon, double &plow_lat)
{
    //采用勾股定理计算犁设备的KP值
    if(tow_wrie_out < water_depth) return false;
    double sub_kp = sqrt(tow_wrie_out * tow_wrie_out - water_depth * water_depth);
    plow_kp = ship_kp - sub_kp;
    //计算犁设备的经纬度
    distbear_to_latlon(ship_lat, ship_lon, sub_kp, 180-ship_course, plow_lat, plow_lon);
    return true;

}

void GlogFunction::addPointIntoList(std::vector<std::pair<double, double> > &path, double lat, double lon)
{
    if(path.size() <= 1)
    {
        path.push_back(std::pair<double, double>(lat, lon));
    } else
    {
        //计算当前点和前两点的方位角。如果方位角相同，就更新前一个点。如果角度不相同，则直接添加
        std::pair<double, double> p1 = path.at(path.size() - 2);
        std::pair<double, double> p2 = path.at(path.size() - 1);
        double angle1 = calcAzimuth(p1.second, p1.first, lon, lat);
        double angle2 = calcAzimuth(p2.second, p2.first, lon, lat);
        if(fabs(angle1 - angle2) < 5)
        {
            //设定阈值为5，小于5就是一条直线上，否则就是ac
            path.pop_back();
        }
        path.push_back(std::pair<double, double>(lat, lon));
    }
}

QByteArray GlogFunction::short2Bytes(short val, int mode)
{
    QByteArray res;
    res.resize(2);
    if(mode == HIGH_FIRST)
    {
        res[0] = (val >> 8 ) & 0xFF;
        res[1] = val & 0xFF;
    } else
    {
        res[0] = val & 0xFF;
        res[1] = (val >> 8) & 0xff;
    }

    return res;
}

QByteArray GlogFunction::int2Bytes(int val, int mode)
{
    QByteArray res;
    res.resize(4);
    if(mode == HIGH_FIRST)
    {
        res[0] = (val >> 24 ) & 0xFF;
        res[1] = (val >> 16 ) & 0xFF;
        res[2] = (val >> 8 ) & 0xFF;
        res[3] = val & 0xFF;
    } else
    {
        res[0] = val & 0xFF;
        res[1] = (val >> 8) & 0xff;
        res[2] = (val >> 16 ) & 0xFF;
        res[3] = (val >> 24 ) & 0xFF;
    }

    return res;
}

short GlogFunction::bytes2Short(const QByteArray& bytes, int mode)
{
    short val = 0;
    if(bytes.size() != 2) return 0;
    if(mode == HIGH_FIRST)
    {
        val = bytes[1] & 0xFF;
        val |= ((bytes[0] & 0xFF) << 8);
    } else
    {
        val = bytes[0] & 0xFF;
        val |= ((bytes[1] & 0xFF) << 8);
    }
    return val;
 }

//传入4个字节，前两字节为数值，后两字节为小数位数
double GlogFunction::byte2ShortDouble(const QByteArray& val, int mode)
{
    double res = 0.0;
    if(val.size() >= 2)
    {
        QByteArray shortArray  = val.mid(0, 2);
        short decimalVal = 0;
        short shortVal = bytes2Short(shortArray, mode);
        if(val.size() >= 4)
        {
            QByteArray decimalArray = val.mid(2, 2);
            decimalVal = bytes2Short(decimalArray, mode);
        }

        res = shortVal * qPow(0.1, decimalVal);
    }
    return res;
}

int GlogFunction::byte2Int(const QByteArray &bytes, int mode)
{
    int res = 0;
    if(bytes.size() >= 4)
    {
        if(mode == LOW_FIRST)
        {
            res = bytes[0] & 0x000000FF;
            res |= ((bytes[1] << 8) & 0x0000FF00);
            res |= ((bytes[2] << 16) & 0x00FF0000);
            res |= ((bytes[3] << 24) & 0xFF000000);
        } else
        {

            res = bytes[3] & 0x000000FF;
            res |= ((bytes[2] << 8) & 0x0000FF00);
            res |= ((bytes[1] << 16) & 0x00FF0000);
            res |= ((bytes[0] << 24) & 0xFF000000);
        }
    }

    return res;
}

