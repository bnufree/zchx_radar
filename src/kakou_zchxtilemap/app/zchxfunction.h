#ifndef ZCHXFUNCTION_H
#define ZCHXFUNCTION_H
#include <math.h>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QTime>
#include <QVector2D>
#include <QGeoCoordinate>
#include <QPolygonF>


#define RADIUS 6372795.0            //Earth radius 6371000
#define Rc  6378137
#define	Rj  6356725


#define         GLOB_PI                                 (3.14159265358979323846)
#define         DOUBLE_EPS                              0.000001
#define         EARTH_HALF_CIRCUL_LENGTH                20037508.3427892

struct Mercator;
struct Latlon;

Latlon mercatorToLatlon(const Mercator& mct);
Mercator latlonToMercator(const Latlon& ll);
Mercator latlonToMercator(double lat, double lon);

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

    Mercator(const QPointF& pnt)
    {
        mX = pnt.x();
        mY = pnt.y();
    }

    Mercator offset(double dx, double dy) const
    {
        return Mercator(mX +dx, mY + dy);
    }

    bool operator ==(const Mercator& other) const
    {
        return distanceToLine(other, other) < 1.0;
    }

    double distanceToPoint(double lat, double lon) const
    {
        return distanceToPoint(latlonToMercator(lat, lon));
    }

    double distanceToPoint(const Mercator& other) const
    {
        return distanceToLine(other, other);
    }

    double distanceToLine(const Mercator& line_start, const Mercator& line_end) const
    {
        QVector2D now(mX, mY);
        QVector2D start(line_start.mX, line_start.mY);
        QVector2D direction(line_end.mX - line_start.mX, line_end.mY - line_start.mY);
        direction.normalize();
        return now.distanceToLine(start, direction);
    }

    QPointF toPointF() const
    {
        return QPointF(mX, mY);
    }

    static double distance(const Mercator& m1, const Mercator& m2)
    {
        return m1.distanceToPoint(m2);
    }

    static double distance(double lat1, double lon1, double lat2, double lon2)
    {
        return distance(latlonToMercator(lat1, lon1), latlonToMercator(lat2, lon2));
    }

    static double angle(double lat1, double lon1, double lat2, double lon2)
    {
        QGeoCoordinate p1(lat1, lon1);
        return p1.azimuthTo(QGeoCoordinate(lat2, lon2));
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



enum PNTPOSTION{
    POS_UNDETERMINED = -1,
    POS_ON_LINE = 0,        //点在直线上
    POS_LEFT,//左侧
    POS_RIGHT,//右侧
    POS_AHEAD,//前方
    POS_AFTER,//后方
    POS_ON_VERTEX,

};

enum PredictionAreaType{
    Prediction_Area_Rectangle = 0,
    Prediction_Area_Triangle,
};


class      zchxTargetPredictionLine
{
private:
    Mercator    mStart;
    Mercator    mEnd;
    QPolygonF   mPredictionArea;
    QList<Latlon>   mPredictionAreaLL;
    double      mWidth;
    int         mType;
private:
    void makePridictionArea();
public:
    zchxTargetPredictionLine() {
        mWidth = 0;
        mType = Prediction_Area_Rectangle;
    }
    zchxTargetPredictionLine(Latlon start, Latlon end, double width, int type)
    {
        mStart = latlonToMercator(start);
        mEnd = latlonToMercator(end);
        mWidth = width;
        mType = type;
        makePridictionArea();
    }

    zchxTargetPredictionLine(double start_lat, double start_lon, double end_lat, double end_lon, double width, int type)
    {
//        zchxTargetPredictionLine(Latlon(start_lat, start_lon), Latlon(end_lat, end_lon), width, type);
        mStart = latlonToMercator(start_lat, start_lon);
        mEnd = latlonToMercator(end_lat, end_lon);
        mWidth = width;
        mType = type;
        makePridictionArea();
    }
    void     setPridictionWidth(int width);
    void     setPridictionType(int type);

    bool    isPointIn(const Mercator& point);
    bool    isPointIn(double lat, double lon) {return isPointIn(latlonToMercator(lat, lon));}
    bool    isPointIn(Latlon ll) {return(isPointIn(latlonToMercator(ll)));}

    QList<Latlon>   getPredictionArea() const {return mPredictionAreaLL;}

    double distanceToMe(const Mercator& point) const
    {
        return point.distanceToLine(mStart, mEnd);
    }

    double distanceToMe(double lat, double lon) const
    {
        return distanceToMe(latlonToMercator(Latlon(lat, lon)));
    }

    double length() const
    {
        return mStart.distanceToLine(mEnd, mEnd);
    }

    bool isValid() const;

    PNTPOSTION pointPos(double& dist_to_line, double& dist_div_line, double lat, double lon);
    PNTPOSTION pointPos(double& dist_to_line, double& dist_div_line, const Mercator& point);


};

#endif // ZCHXFUNCTION_H
