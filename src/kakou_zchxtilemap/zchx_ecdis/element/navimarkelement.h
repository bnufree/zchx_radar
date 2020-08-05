#ifndef NAVIMARKELEMENT_H
#define NAVIMARKELEMENT_H

#include "fixelement.h"

namespace qt {
class NaviMarkElement : public FixElement<ZCHX::Data::ITF_NaviMark>
{
public:
    explicit NaviMarkElement(const ZCHX::Data::ITF_NaviMark & data, zchxMapWidget* frame);

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

#endif // NAVIMARKELEMENT_H
