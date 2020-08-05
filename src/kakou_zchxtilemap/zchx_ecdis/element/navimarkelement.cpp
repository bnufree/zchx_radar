#include "navimarkelement.h"
#include "zchxmapframe.h"
#include "map_layer/zchxmaplayermgr.h"
#include <QPainter>

using namespace qt ;

NaviMarkElement::NaviMarkElement(const ZCHX::Data::ITF_NaviMark &data, zchxMapWidget* frame)
    :FixElement<ZCHX::Data::ITF_NaviMark>(data, ZCHX::Data::ELE_NAVIMARK, ZCHX::LAYER_NAVIMARK, frame)
    ,mParent(0)
{
}

void NaviMarkElement::setParent(Element *ele)
{
    mParent = ele;
}

Element* NaviMarkElement::getParent()
{
    return mParent;
}

void NaviMarkElement::drawElement(QPainter *painter)
{
    if(!isDrawAvailable(painter)) return;
    if(mParent) return; //当前目标悬挂在其他图元上不显示
    //开始显示
    int curScale = this->framework()->GetDrawScale();
    QPointF pos = this->framework()->LatLon2Pixel(data().getLat(), data().getLon()).toPointF();

    QPixmap image;
    if (data().colorType == 1)
    {
        image = ZCHX::Utils::getImage(":/navimark_green.png", Qt::black, curScale);;
    }
    else if (data().colorType == 2)
    {
        image = ZCHX::Utils::getImage(":/navimark_red.png", Qt::black, curScale);;
    }
    else
    {
        return;
    }

    QRectF rect(0, 0, image.width(), image.height());
    rect.moveCenter(pos);
    painter->drawPixmap(rect.toRect(), image);
}

void NaviMarkElement::clicked(bool isDouble)
{
    if(!mView) return;
    if(isDouble) {
    } else {
    }
}

void NaviMarkElement::showToolTip(const QPoint &pos)
{
}

