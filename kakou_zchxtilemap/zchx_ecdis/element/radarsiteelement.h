#ifndef RADARSITEELE_H
#define RADARSITEELE_H

#include "fixelement.h"

namespace qt {
class RadarSiteElement : public FixElement<ZCHX::Data::ITF_RadarSite>
{
public:
    explicit RadarSiteElement(const ZCHX::Data::ITF_RadarSite & data, zchxMapWidget* frame);

    void updateGeometry(QPointF, qreal){}
    void drawElement(QPainter *painter);
    void setParent(Element* ele);
    Element* getParent();
    void clicked(bool isDouble);
private:
    Element     *mParent;
};

}

#endif // RADARSITEELE_H
