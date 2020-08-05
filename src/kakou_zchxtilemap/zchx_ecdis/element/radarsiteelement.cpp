#include "radarsiteelement.h"
#include "zchxmapframe.h"
#include "map_layer/zchxmaplayermgr.h"
#include <QPainter>

using namespace qt ;

RadarSiteElement::RadarSiteElement(const ZCHX::Data::ITF_RadarSite &data, zchxMapWidget* frame)
    :FixElement<ZCHX::Data::ITF_RadarSite>(data, ZCHX::Data::ELE_RADAR_SITE, ZCHX::LAYER_RADAR_SITE, frame)
    ,mParent(0)
{
}

void RadarSiteElement::setParent(Element *ele)
{
    mParent = ele;
}

Element* RadarSiteElement::getParent()
{
    return mParent;
}

void RadarSiteElement::drawElement(QPainter *painter)
{
    if(!isDrawAvailable(painter)) return;
    if(mParent) return; //当前目标悬挂在其他图元上不显示
    //开始显示
    int curScale = this->framework()->GetDrawScale();
    QPointF pos = this->framework()->LatLon2Pixel(data().getLat(), data().getLon()).toPointF();
    QPixmap image = ZCHX::Utils::getImage(":/sdasid/bak/XlzVo.png", Qt::black, curScale);;

    QRectF rect(0, 0, image.width(), image.height());
    rect.moveCenter(pos);
    painter->drawPixmap(rect.toRect(), image);
}

void RadarSiteElement::clicked(bool isDouble)
{
    if(!mView) return;
    if(isDouble) {
    } else {
    }
}

