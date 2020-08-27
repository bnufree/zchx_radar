#ifndef RADARVIDEOELE_H
#define RADARVIDEOELE_H

#include "IDrawElement.hpp"
#include <QtCore>

namespace qt {
class RadarVideoImageElement : public Element
{
public:
    explicit RadarVideoImageElement(const ZCHX::Data::ITF_RadarVideoImage& data, zchxMapWidget* frame);

    const ZCHX::Data::ITF_RadarVideoImage &data() const;
    void setData(const ZCHX::Data::ITF_RadarVideoImage& dev);
    void drawElement(QPainter *painter);
    void drawOutline(QPainter *painter, const QPointF& center, double in, double out);
    std::string name () const {return m_data.name.toStdString();}

private:
    ZCHX::Data::ITF_RadarVideoImage  m_data;
    QMutex m_mutex;
};

}

#endif // RADARVIDEOELE_H
