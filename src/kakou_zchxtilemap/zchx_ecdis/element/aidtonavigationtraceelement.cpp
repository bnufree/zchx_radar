#include "aidtonavigationtraceelement.h"
#include "zchxmapframe.h"
#include "map_layer/zchxmaplayermgr.h"
#include <QPainter>

#define MAX_DIS 10000

using namespace qt ;

AidtoNavigationTraceElement::AidtoNavigationTraceElement(const ZCHX::Data::ITF_AidtoNavigationTrace &data, zchxMapWidget* frame)
    :FixElement<ZCHX::Data::ITF_AidtoNavigationTrace>(data, ZCHX::Data::ELE_AIDTO_NAVIGATION, ZCHX::LAYER_AIS_SITE, frame)
    ,mParent(0)
{
}

void AidtoNavigationTraceElement::setParent(Element *ele)
{
    mParent = ele;
}

Element* AidtoNavigationTraceElement::getParent()
{
    return mParent;
}

void AidtoNavigationTraceElement::drawElement(QPainter *painter)
{
    if(!isDrawAvailable(painter)) return;
    if(mParent) return; //当前目标悬挂在其他图元上不显示
//    int curScale = this->framework()->GetDrawScale();

    std::vector<std::pair<double,double>> tmp_path = data().path;
    if (tmp_path.size() <= 1)
    {
        return;
    }

    PainterPair chk(painter);
    painter->setPen(QPen(QColor(Qt::gray), 1, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    QPainterPath polygon;
    for(int i = 0; i < tmp_path.size(); ++i)
    {
        std::pair<double, double> ll = tmp_path[i];
        QPointF pos = this->framework()->LatLon2Pixel(ll.first, ll.second).toPointF();

        if(0 == i)
        {
            polygon.moveTo(pos);
        }
        else if (i > 0)
        {
            std::pair<double, double> lastll = tmp_path[i - 1];
            int dis = ZCHX::Utils::instance()->getDistanceDeg(lastll.first, lastll.second, ll.first, ll.second);
            if (dis > MAX_DIS)
            {
                painter->drawPath(polygon);

                polygon.moveTo(pos);
            }
            else
            {
                polygon.lineTo(pos);
            }
        }
    }

    painter->drawPath(polygon);
}

void AidtoNavigationTraceElement::clicked(bool isDouble)
{
    if(!mView) return;
    if(isDouble) {
    } else {
    }
}
