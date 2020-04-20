#ifndef AIDTONAVIGATIONTRACEELEMENT_H
#define AIDTONAVIGATIONTRACEELEMENT_H

#include "fixelement.h"

namespace qt {
class AidtoNavigationTraceElement : public FixElement<ZCHX::Data::ITF_AidtoNavigationTrace>
{
public:
    explicit AidtoNavigationTraceElement(const ZCHX::Data::ITF_AidtoNavigationTrace & data, zchxMapWidget* frame);

    void updateGeometry(QPointF, qreal){}
    void drawElement(QPainter *painter);
    void setParent(Element* ele);
    Element* getParent();
    void clicked(bool isDouble);
private:
    Element     *mParent;
};

}

#endif // AIDTONAVIGATIONTRACEELEMENT_H
