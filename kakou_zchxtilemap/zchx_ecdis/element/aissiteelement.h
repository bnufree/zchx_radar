#ifndef AISSITEELEMENT_H
#define AISSITEELEMENT_H

#include "fixelement.h"

namespace qt {
class AisSiteElement : public FixElement<ZCHX::Data::ITF_AisSite>
{
public:
    explicit AisSiteElement(const ZCHX::Data::ITF_AisSite & data, zchxMapWidget* frame);

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

#endif // AISSITEELEMENT_H
