#ifndef AIDTONAVIGATIONELEMENT_H
#define AIDTONAVIGATIONELEMENT_H

#include "fixelement.h"

namespace qt {
class AidtoNavigationElement : public FixElement<ZCHX::Data::ITF_AidtoNavigation>
{
public:
    explicit AidtoNavigationElement(const ZCHX::Data::ITF_AidtoNavigation & data, zchxMapWidget* frame);

    void updateGeometry(QPointF, qreal){}
    void drawElement(QPainter *painter);
    void setParent(Element* ele);
    Element* getParent();
    void clicked(bool isDouble);
    void showToolTip(const QPoint& pos);
private:
    Element     *mParent;
};

}

#endif // AIDTONAVIGATIONELEMENT_H
