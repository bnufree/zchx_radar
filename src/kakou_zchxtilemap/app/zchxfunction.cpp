#include "zchxfunction.h"
#include <QPointF>
#include <QLine>


Latlon mercatorToLatlon(const Mercator& mct)
{
    double x = mct.mX/EARTH_HALF_CIRCUL_LENGTH * 180.0;
    double y = mct.mY/EARTH_HALF_CIRCUL_LENGTH * 180.0;
    y = 180/GLOB_PI*(2*atan(exp(y*GLOB_PI/180.0))-GLOB_PI/2.0);
    return Latlon(y, x);
}

Mercator latlonToMercator(double lat, double lon)
{
    return latlonToMercator(Latlon(lat, lon));
}

Mercator latlonToMercator(const Latlon& ll)
{
    //qDebug()<<"wgs:"<<wgs84.mLon<<wgs84.mLat;
    double mx = ll.lon * EARTH_HALF_CIRCUL_LENGTH / 180;
    double my = log(tan((90 + ll.lat) * GLOB_PI / 360)) / (GLOB_PI / 180);
    my = my * EARTH_HALF_CIRCUL_LENGTH / 180;

    return Mercator(mx, my);
}

bool zchxTargetPredictionLine::isValid() const
{
    return length() >= 1.0;
}

void zchxTargetPredictionLine::makePridictionArea()
{
    mPredictionArea.clear();
    mPredictionAreaLL.clear();
//    if(!isValid()) return;
    //计算直线的角度
    double angle = atan2(mEnd.mY - mStart.mY, mEnd.mX - mStart.mX);
    QLineF line(mStart.mX, mStart.mY, mEnd.mX, mEnd.mY);
    QLineF low = line.translated(mWidth * 0.5 * sin(angle), -mWidth *0.5 * cos(angle));
    QLineF high = line.translated(-mWidth * 0.5* sin(angle), mWidth *0.5 * cos(angle));
    //将墨卡托转换成经纬度, 保存对应的经纬度点列
    if(mType == Prediction_Area_Rectangle)
    {
        mPredictionArea.append(low.p1());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));
        mPredictionArea.append(low.p2());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));
        mPredictionArea.append(high.p2());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));
        mPredictionArea.append(high.p1());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));
        mPredictionArea.append(low.p1());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));

    } else
    {
        mPredictionArea.append(line.p1());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));
        mPredictionArea.append(low.p2());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));
        mPredictionArea.append(high.p2());
        mPredictionAreaLL.append(mercatorToLatlon(Mercator(mPredictionArea.last())));
    }
}

void zchxTargetPredictionLine::setPridictionType(int type)
{
    if(mType != type)
    {
        mType = type;
        makePridictionArea();
    }
}

void zchxTargetPredictionLine::setPridictionWidth(int width)
{
    if(width != mWidth)
    {
        mWidth = width;
        makePridictionArea();
    }
}


bool zchxTargetPredictionLine::isPointIn(const Mercator &point)
{
    bool sts = mPredictionArea.containsPoint(point.toPointF(), Qt::OddEvenFill);
    return sts;
}

PNTPOSTION zchxTargetPredictionLine::pointPos(double& dist_to_line, double& dist_div_line, const Mercator &point)
{
    //检查目标直线是不是一个点的情况
    if(!isValid()) return POS_UNDETERMINED;

    //检查目标点是否在线段的端点上
    if(point == mStart)
    {
        dist_to_line = 0.0;
        dist_div_line = 0.0;
        return POS_ON_VERTEX;
    } else if(point == mEnd)
    {
        dist_to_line = 0.0;
        dist_div_line = length();
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
    Mercator p0 = point;
    Mercator p1 = mStart;
    Mercator p2 = mEnd;
    //检查目标点的坐标是否在线段外
    double max_x = p1.mX < p2.mX ? p2.mX : p1.mX;
    double min_x = p1.mX < p2.mX ? p1.mX : p2.mX;
    double max_y = p1.mY < p2.mY ? p2.mY : p1.mY;
    double min_y = p1.mY < p2.mY ? p1.mY : p2.mY;
    if(p0.mX < min_x || p0.mX > max_x || p0.mY < min_y || p0.mY > max_y)
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
        dist_to_line = 0.0;
        dist_div_line = point.distanceToLine(mStart, mStart);
        if(dist_div_line < 1 || fabs(dist_div_line - length()) < 1)
        {
            return POS_ON_VERTEX;
        }
        return POS_ON_LINE;
    } else
    {
        //点在线的两边
        //1)计算点到起点的距离
        double start_dis = point.distanceToLine(mStart, mStart);
        //2)计算到直线的距离
        dist_to_line = distanceToMe(point);
        //3)勾股定理计算分割距离
        dist_div_line = sqrt(start_dis * start_dis - dist_to_line * dist_to_line);

        return cross_product > DOUBLE_EPS? POS_RIGHT:POS_LEFT;
    }
    return POS_UNDETERMINED;
}


PNTPOSTION zchxTargetPredictionLine::pointPos(double& dist_to_line, double& dist_div_line, double lat, double lon)
{
    return pointPos(dist_to_line, dist_div_line, latlonToMercator(Latlon(lat, lon)));
}


