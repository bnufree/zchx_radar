#include "zchxdrawangletool.h"
#include "zchxmapframe.h"

using namespace qt;
using namespace ZCHX::Data;
zchxDrawAngleTool::zchxDrawAngleTool(zchxMapWidget* w, QObject *parent) : zchxDrawTool(w, qt::eTool::DRAWDIRANGLE, parent)
{

}

void zchxDrawAngleTool::appendPoint(const QPointF &point)
{
    ZCHX::Data::LatLon ll = pix2ll(point);
    if(mPnts.size() < 2){
        mPnts.append(ll);
    } else {
        mPnts[1] = ll;
    }
}

void zchxDrawAngleTool::show(QPainter *painter)
{
    if(!isReady() || mPnts.size() < 2) return;
    PainterPair chk(painter);
    painter->setRenderHint(QPainter::Antialiasing);
    QFont font;
    font.setPixelSize(14);
    painter->setFont(font);

    //计算角度
    QPolygonF points = pixPnts();
    double dr = zchxMapDataUtils::getDistancePixel(points[0], points[1]);
    double angle4TrueNorth = zchxMapDataUtils::DegToRad((mWidget->framework()->GetRotateAngle()));
    double antArc = zchxMapDataUtils::TwoVectorsAngle(points[0], Point2D(points[0].x(),points[0].y() - dr), points[1]);
    antArc = zchxMapDataUtils::AngleIn2PI(antArc - angle4TrueNorth);

    //计算两点之间的地球距离
    LatLon start_latlon = mPnts[0];
    LatLon end_latlon = mPnts[1];
    double distance = zchxMapDataUtils::DistanceOnEarth(start_latlon, end_latlon) / 1852.000;
    double startAng = angle4TrueNorth / GLOB_PI * 180;
    double lenAng = antArc / GLOB_PI * 180;
    QRectF rectangle(points[0].x() - dr, points[0].y() - dr, dr * 2, dr * 2);
    int startAngle = (90 - startAng)* 16;
    int spanAngle = (-lenAng) * 16;
    painter->setPen(QPen(Qt::red,2));
    painter->drawPie(rectangle, startAngle, spanAngle);
    if (lenAng < 180)
    {
        painter->drawText(points[1] + QPointF(10,10), QString("∠%1°").arg(lenAng));
        painter->drawText(points[1] + QPointF(10,25), QString("distance:%1 nmi").arg(distance));
    }
    else
    {
        painter->drawText(points[1] - QPointF(120,25), QString("∠%1°").arg(lenAng));
        painter->drawText(points[1] - QPointF(120,10), QString("distance:%1 nmi").arg(distance));
    }

}
