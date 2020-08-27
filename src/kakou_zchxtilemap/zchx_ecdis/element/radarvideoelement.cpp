#include "radarvideoelement.h"
#include "zchxmapframe.h"
#include "map_layer/zchxmaplayermgr.h"
#include <QPainter>

using namespace qt;

RadarVideoImageElement::RadarVideoImageElement(const ZCHX::Data::ITF_RadarVideoImage& data, zchxMapWidget* frame)
    :Element(data.lat, data.lon, frame, ZCHX::Data::ELE_RADAR_VIDEOGLOW, ZCHX::LAYER_RADARVIDEO)
{
    setData(data);
}

const ZCHX::Data::ITF_RadarVideoImage &RadarVideoImageElement::data() const
{
    return m_data;
}

void RadarVideoImageElement::setData(const ZCHX::Data::ITF_RadarVideoImage &dev)
{
    QMutexLocker locker(&m_mutex);
    m_data = dev;

    setIsUpdate(true);
}

void RadarVideoImageElement::drawOutline(QPainter *painter, const QPointF& center, double in, double out)
{
    if(!painter) return;
    PainterPair chk(painter);
    painter->setPen(QColor(200,200,100,200));
    painter->drawEllipse(center, out, in);
}

void RadarVideoImageElement::drawElement(QPainter *painter)
{
    if(!painter || !mView->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADARVIDEO) || !mView->framework()) return;

    QMutexLocker locker(&m_mutex);

    ZCHX::Data::LatLon ll0 = ZCHX::Utils::distbear_to_latlon(data().lat, m_data.lon, m_data.distance, 0);
    ZCHX::Data::LatLon ll90 = ZCHX::Utils::distbear_to_latlon(data().lat, m_data.lon, m_data.distance, 90);
    ZCHX::Data::LatLon ll180 = ZCHX::Utils::distbear_to_latlon(data().lat, m_data.lon, m_data.distance, 180);
    ZCHX::Data::LatLon ll270 = ZCHX::Utils::distbear_to_latlon(data().lat, m_data.lon, m_data.distance, 270);
    ZCHX::Data::Point2D pos0 = mView->framework()->LatLon2Pixel(ll0);
    ZCHX::Data::Point2D pos90 = mView->framework()->LatLon2Pixel(ll90);
    ZCHX::Data::Point2D pos180 = mView->framework()->LatLon2Pixel(ll180);
    ZCHX::Data::Point2D pos270 = mView->framework()->LatLon2Pixel(ll270);

    double dMinDrawX1 = fmin(pos0.x, pos90.x);
    double dMinDrawY1 = fmin(pos0.y, pos90.y);
    double dMaxDrawX1 = fmax(pos0.x, pos90.x);
    double dMaxDrawY1 = fmax(pos0.y, pos90.y);
    double dMinDrawX2 = fmin(pos180.x, pos270.x);
    double dMinDrawY2 = fmin(pos180.y, pos270.y);
    double dMaxDrawX2 = fmax(pos180.x, pos270.x);
    double dMaxDrawY2 = fmax(pos180.y, pos270.y);

    double dMinDrawX = fmin(dMinDrawX1, dMinDrawX2);
    double dMinDrawY = fmin(dMinDrawY1, dMinDrawY2);
    double dMaxDrawX = fmax(dMaxDrawX1, dMaxDrawX2);
    double dMaxDrawY = fmax(dMaxDrawY1, dMaxDrawY2);
    double dWidth = qAbs(dMaxDrawX-dMinDrawX);
    double dHeight = qAbs(dMaxDrawY-dMinDrawY);

    QPointF centerPos = mView->framework()->LatLon2Pixel(data().lat, data().lon).toPointF();
    double dRadiusIn = dWidth/2.0;
    double dRadiusOut = qSqrt(dRadiusIn*dRadiusIn + dRadiusIn*dRadiusIn);
    //绘制外圈
    drawOutline(painter, centerPos, dRadiusOut, dRadiusOut);
    QRectF objRect(0, 0, dWidth, dHeight);
    objRect.moveCenter(centerPos);
    //绘制回波图片
    double angleFromNorth = mView->framework()->GetRotateAngle(); //计算当前正北方向的方向角
    PainterPair chk(painter);
    double translateX = objRect.x() + objRect.width()/2.0;
    double translateY = objRect.y() + objRect.height()/2.0;
    painter->translate(translateX, translateY);
    painter->rotate((int)(angleFromNorth) % 360);
    painter->translate(-translateX,-translateY);

    if (data().image.size() > 0 )
    {
        QPixmap objPixmap;
        objPixmap.loadFromData(data().image);

        painter->drawPixmap(centerPos.x() - dWidth / 2, centerPos.y() - dHeight / 2, dWidth, dHeight, objPixmap);
    }
}

