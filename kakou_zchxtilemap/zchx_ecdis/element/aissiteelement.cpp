#include "aissiteelement.h"
#include "zchxmapframe.h"
#include "map_layer/zchxmaplayermgr.h"
#include <QPainter>

using namespace qt ;

AisSiteElement::AisSiteElement(const ZCHX::Data::ITF_AisSite &data, zchxMapWidget* frame)
    :FixElement<ZCHX::Data::ITF_AisSite>(data, ZCHX::Data::ELE_AIS_SITE, ZCHX::LAYER_AIS_SITE, frame)
    ,mParent(0)
{
}

void AisSiteElement::setParent(Element *ele)
{
    mParent = ele;
}

Element* AisSiteElement::getParent()
{
    return mParent;
}

void AisSiteElement::drawElement(QPainter *painter)
{
    if(!isDrawAvailable(painter)) return;
    if(mParent) return; //当前目标悬挂在其他图元上不显示
    //开始显示
    int curScale = this->framework()->GetDrawScale();
    QPointF pos = this->framework()->LatLon2Pixel(data().getLat(), data().getLon()).toPointF();
    QPixmap image = ZCHX::Utils::getImage(":/aidtoNavigation/Basic shape.png", Qt::black, curScale);;

    QRectF rect(0, 0, image.width(), image.height());
    rect.moveCenter(pos);
    painter->drawPixmap(rect.toRect(), image);
}

void AisSiteElement::clicked(bool isDouble)
{
    if(!mView) return;
    if(isDouble) {
    } else {
    }
}

void AisSiteElement::showToolTip(const QPoint &pos)
{
}

