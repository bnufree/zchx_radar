#include "radarpathelement.h"
#include "zchxmapframe.h"
#include "map_layer/zchxmaplayermgr.h"
#include <QPainter>
#include "zchxutils.hpp"
#include "profiles.h"
#include "zchxMapDataUtils.h"

#define OFFSET_X 0.00014
#define OFFSET_Y 0.00014
#define MIN_SHOW_DIS 5

#define     CUT_FISRT_DIRECTLY          0

using namespace ZCHX::Data;

QString  pointfTostring(const QPointF& pnt)
{
    return QString("%1,%2").arg(pnt.x(), 0, 'f', 6).arg(pnt.y(), 0, 'f', 6);
}

QPointF pointfFromString(const QString& str)
{
    QStringList list = str.split(",", QString::SkipEmptyParts);
    if(list.size() != 2) return QPointF(0.0,0.0);
    return QPointF(list[0].toDouble(), list[1].toDouble());
}


namespace qt {
RadarPathElement::RadarPathElement(const ZCHX::Data::ITF_RadarRouteNode& data, zchxMapWidget* frame)
    :Element(data.mTop.center.lat, data.mTop.center.lon, frame, ZCHX::Data::ELE_RADAR_RADARPATH, ZCHX::LAYER_RADARPATH)
{
    setData(data);
}

const ITF_RadarRouteNode &RadarPathElement::data() const
{
    return mRoutePath;
}

void RadarPathElement::setData(const ZCHX::Data::ITF_RadarRouteNode &rect)
{    
    QMutexLocker locker(&m_mutex);

    mRoutePath = rect;
    setIsUpdate(true);
}


void RadarPathElement::drawElement(QPainter *painter)
{
    if(!painter || !mView->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADARPATH) || !mView->framework())
    {
        return;
    }

//    qDebug()<<"show rect now.."<<mView->framework()->getZoom();

    if(mView->framework()->Zoom() < 10) return;

    QMutexLocker locker(&m_mutex);

    if (getIsUpdate())
    {
        //在每一个顶点画圆圈，然后顶点之间划线，最后一个节点画预推区域
        //画顶点
        PainterPair chk(painter);
        painter->setPen(QColor(128, 128, 128, 128));
        painter->setBrush(QBrush(QColor(0, 0, 255, 100)));
        QPointF centerPos = mView->framework()->LatLon2Pixel(mRoutePath.mTop.center.lat, mRoutePath.mTop.center.lon).toPointF();
        QList<QPointF> circle_pnts_list;
        QList<QPolygonF> predictionAreaList;
        circle_pnts_list.append(centerPos);
        QMap<QString, QStringList> textMap;
        textMap[pointfTostring(centerPos)].append(QString("00-%1").arg(mRoutePath.mNum));
        for(int i=0; i<mRoutePath.mPaths.size(); i++)
        {
            QPolygonF path;
            path.append(centerPos);
            ITF_RadarRectDefList children = mRoutePath.mPaths[i];
            for(int k=0; k<children.size();k++)
            {
                ZCHX::Data::ITF_RadarRectDef child = children[k];
                QPointF curPos = mView->framework()->LatLon2Pixel(child.center.lat, child.center.lon).toPointF();
                path.append(curPos);
                circle_pnts_list.append(curPos);
                textMap[pointfTostring(curPos)].append(QString("    %1%2-%3-%4").arg(i+1).arg(k+1).arg(child.rectNumber).arg(mRoutePath.mNum));
                if(k==children.size()-1)
                {
                    //最后一个节点，画预推区域
                    QPolygonF poly;
                    for(int m=0; m<child.predictionArea.size();m++)
                    {
                        ZCHX::Data::LatLon block = child.predictionArea[m];
                        curPos = mView->framework()->LatLon2Pixel(block.lat, block.lon).toPointF();
                        poly.append(curPos);
                    }
                    predictionAreaList.append(poly);
                }
            }
            painter->drawPolyline(path);

        }
        if(mRoutePath.mPaths.size() == 0)
        {
            //没有子节点的情况
            QPolygonF poly;
            for(int m=0; m<mRoutePath.mTop.predictionArea.size();m++)
            {
                ZCHX::Data::LatLon block = mRoutePath.mTop.predictionArea[m];
                QPointF curPos = mView->framework()->LatLon2Pixel(block.lat, block.lon).toPointF();
                poly.append(curPos);
            }
            predictionAreaList.append(poly);
        }

        for(int i=0; i<circle_pnts_list.size();i++)
        {
            painter->drawEllipse(circle_pnts_list[i], 5, 5);
        }
        foreach (QString pnt, textMap.keys()) {
            painter->drawText(pointfFromString(pnt), textMap[pnt].join("   "));
        }

        for(int i=0; i<predictionAreaList.size();i++)
        {
            painter->drawPolygon(predictionAreaList[i]);
        }
    }

}


}

