#ifndef UTIL_H
#define UTIL_H

#include <QObject>

#define KA 1.852
#define MINETE 60000
#define GLOB_PI  (3.14159265358979323846)
#define PIC_PATH "/Screenshot/"
const double EARTH_R = 6378.137;
const double E_ABOUT = 2.71828;

class Util : public QObject
{
    Q_OBJECT
public:
    explicit Util(QObject *parent = 0);

    static QByteArray HexStringToByteArray(QString HexString);
    static QByteArray intToByte16(int i);
    static double degToRad(double d);
    static double radToDeg(double r);
    static double getDistanceDeg(double lat1, double lon1, double lat2, double lon2);
    static double* computerThatLonLat(double lon, double lat, double dist, double brng);
    static qint64 getNextMinite(qint64 time);
    static double rad(double d) {return d * GLOB_PI / 180.0;}
    static double deg(double x) {return x * 180 / GLOB_PI;}
    static double getAverage(double value1, double value2, qint64 offsetX, qint64 offsetY);

signals:

public slots:
};

#endif // UTIL_H
