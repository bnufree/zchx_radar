#ifndef RADARRECTELE_H
#define RADARRECTELE_H

#include "IDrawElement.hpp"
#include <QtCore>

struct      zchxPolygon
{
    QPolygon polygon;
    QColor   color;
    zchxPolygon (const QPolygon& poly, const QColor& rcolor) {polygon = poly; color = rcolor;}
};

typedef     QList<zchxPolygon>  QPolygonList;

namespace qt {
class RadarRectGlowElement : public Element
{
public:
    explicit RadarRectGlowElement(const ZCHX::Data::ITF_RadarRect& data, zchxMapWidget* frame);

    const ZCHX::Data::ITF_RadarRect &data() const;
    void setData(const ZCHX::Data::ITF_RadarRect& rect);
    void drawElement(QPainter *painter);
    void drawOutline(QPainter *painter, const QPointF& center, double in, double out);
    std::string name () const {return mRect.getName().toLatin1().data();}

private:
    int  getRectHight(double refer_dis);
    void calculateRect(double& angle, double& length, const ZCHX::Data::ITF_RadarRectDef &rect);
    void drawRadarTracks(QPainter* painter);
    void calculateSkipRect(ZCHX::Data::ITF_RadarRectDefList& list, const ZCHX::Data::ITF_RadarRectDef& pre, const ZCHX::Data::ITF_RadarRectDef& cur);
    void drawRadarRect(QPainter* painter,
                       const QColor& targetColor,
                       const QColor& edgeColor,
                       const QColor& backgroundColor,
                       const ZCHX::Data::ITF_RadarRectDef &rect,
                       bool  round = false,
                       QPolygon* side = 0);
    void drawRadarRect(const QPointF & leftTopPos, const QPointF & bottomRightPos, QPainter *painter);
    bool isRadarDisplayByTargetSize(const ZCHX::Data::ITF_RadarRect & data,
                                    int targetSizeIndex);
    bool isRadarDisplayByTraceLen(const ZCHX::Data::ITF_RadarRect & data,
                                  int traceLenIndex );
    bool isRadarDisplayByContinueTime(const ZCHX::Data::ITF_RadarRect & data,
                                      int continueTimeIndex);
    QPixmap  scaledAndAlphaPixmap(const QPixmap& source, int alpha, int target_width = 0, int target_height = 0);
    QPixmap  drawPixmap(const QColor& edgeColor, const QColor& brushColor, const QPolygon& poly, int width, int height);
    void     drawPolygon(QPainter* painter, const QColor& edgeColor, const QColor& brushColor, const QPolygon& poly, double lat, double lon);

    ZCHX::Data::ITF_RadarRect mRect;
    static  int             mMaxRectLength;
    static  int             mMaxGapOf2Rect;
    QList<ZCHX::Data::LatLon>           mInsertPointList;
    QList<ZCHX::Data::LatLon>           mOrigonPointList;
    double                  mHistoryPixelLength;
    double                  mMaxRectLenthAtZoom;            //目标在当前等级下的最大长度
    QMutex m_mutex;
};

}

#endif // RADARRECTELE_H
