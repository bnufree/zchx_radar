#ifndef RADARPATHELEMENT_H
#define RADARPATHELEMENT_H

#include "IDrawElement.hpp"
#include <QtCore>


namespace qt {
class RadarPathElement : public Element
{
public:
    explicit RadarPathElement(const ZCHX::Data::ITF_RadarRouteNode& data, zchxMapWidget* frame);

    const ZCHX::Data::ITF_RadarRouteNode &data() const;
    void setData(const ZCHX::Data::ITF_RadarRouteNode& rect);
    void drawElement(QPainter *painter);
    std::string name () const {return mRoutePath.getName().toLatin1().data();}

private:

    ZCHX::Data::ITF_RadarRouteNode mRoutePath;
    QMutex      m_mutex;
};

}

#endif // RADARPATHELEMENT_H
