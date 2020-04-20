#include "aidtonavigationelement.h"
#include "zchxmapframe.h"
#include "map_layer/zchxmaplayermgr.h"
#include <QPainter>

using namespace qt ;

AidtoNavigationElement::AidtoNavigationElement(const ZCHX::Data::ITF_AidtoNavigation &data, zchxMapWidget* frame)
    :FixElement<ZCHX::Data::ITF_AidtoNavigation>(data, ZCHX::Data::ELE_AIDTO_NAVIGATION, ZCHX::LAYER_AIS_SITE, frame)
    ,mParent(0)
{
}

void AidtoNavigationElement::setParent(Element *ele)
{
    mParent = ele;
}

Element* AidtoNavigationElement::getParent()
{
    return mParent;
}

void AidtoNavigationElement::drawElement(QPainter *painter)
{
    if(!isDrawAvailable(painter)) return;
    if(mParent) return; //当前目标悬挂在其他图元上不显示
    //开始显示
    int curScale = framework()->GetDrawScale();
    QPointF pos = framework()->LatLon2Pixel(data().getLat(), data().getLon()).toPointF();
    QString iconName = ":/aidtoNavigation/Basic shape.png";
    switch (data().atonType)
    {
    case ZCHX::Data::ATON_TYPE_PORT_HAND_MARK:
        iconName = ":/aidtoNavigation/port hand mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_STARBOARD_HAND_MARK:
        iconName = ":/aidtoNavigation/Starboard hand mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_BEACON_CARDINAL_N:
    case ZCHX::Data::ATON_TYPE_CARDINAL_MARK_N:
        iconName = ":/aidtoNavigation/North cardinal mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_BEACON_CARDINAL_E:
    case ZCHX::Data::ATON_TYPE_CARDINAL_MARK_E:
        iconName = ":/aidtoNavigation/East cardinal mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_BEACON_CARDINAL_S:
    case ZCHX::Data::ATON_TYPE_CARDINAL_MARK_S:
        iconName = ":/aidtoNavigation/South cardinal mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_BEACON_CARDINAL_W:
    case ZCHX::Data::ATON_TYPE_CARDINAL_MARK_W:
        iconName = ":/aidtoNavigation/West cardinal mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_ISOLATED_DANGER:
        iconName = ":/aidtoNavigation/Isolated danger mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_SAFE_WATER:
        iconName = ":/aidtoNavigation/sate water mark.png";
        break;
    case ZCHX::Data::ATON_TYPE_SPECIAL_MARK:
        iconName = ":/aidtoNavigation/special mark.png";
        break;
    default:
        break;
    }

    if (iconName.isEmpty())
    {
        return;
    }

    QPixmap image = ZCHX::Utils::getImage(iconName, Qt::black, curScale);

    QRectF rect(0, 0, image.width(), image.height());
    rect.moveCenter(pos);
    painter->drawPixmap(rect.toRect(), image);

    if (!data().name.isEmpty() && curScale >= 10)
    {
        painter->drawText(pos.x(), pos.y() + 30, data().name);;
    }
}

void AidtoNavigationElement::clicked(bool isDouble)
{
    if(!mView) return;
    if(isDouble) {
    } else {
    }
}

void AidtoNavigationElement::showToolTip(const QPoint &pos)
{
    QString base_text = QObject::tr("助航名称: ")+data().name+"\n";
    base_text += QObject::tr("MMSI: ")+QString::number(data().mmsi)+"\n";
    base_text += QObject::tr("助航类型: ")+ZCHX::Utils::getAtonTypeName(data().atonType)+"\n";
    QToolTip::showText(pos,base_text);
}
