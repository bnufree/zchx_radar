#include "zchxdrawtool.h"
#include "zchxmapframe.h"


namespace qt {
zchxDrawTool::zchxDrawTool(zchxMapWidget* w, int type, QObject *parent) : QObject(parent)
  ,mWidget(w)
  ,mType(type)
  ,mEle(0)
  ,mIsStartMove(0)
{

}

zchxDrawTool::~zchxDrawTool()
{
    qDebug()<<" draw tool release now!"<<this->metaObject()->className();
}


void zchxDrawTool::show(QPainter* painter)
{
    return;
}

void zchxDrawTool::startDraw()
{
    clearPoints();
}

ZCHX::Data::LatLon zchxDrawTool::pix2ll(const QPointF &pnt)
{
    ZCHX::Data::LatLon ll = mWidget->framework()->Pixel2LatLon(ZCHX::Data::Point2D(pnt));
    return ll;
}

QPointF zchxDrawTool::ll2pix(ZCHX::Data::LatLon ll)
{
    return mWidget->framework()->LatLon2Pixel(ll).toPointF();
}

QPolygonF zchxDrawTool::ll2pix(const QList<ZCHX::Data::LatLon> &lls)
{
    QPolygonF poly;
    foreach (ZCHX::Data::LatLon ll, lls) {
        poly.append(ll2pix(ll));
    }
    return poly;
}

QPolygonF zchxDrawTool::pixPnts()
{
    return ll2pix(mPnts);
}

void zchxDrawTool::appendPoint(const QPointF& pnt)
{
    //检查是否与前一个点相同,相同就不添加
    ZCHX::Data::LatLon ll = pix2ll(pnt);
    if(mPnts.size() > 0 && mPnts.last() == ll) return;
    mPnts.append(ll);
}

int zchxDrawTool::getPointSize()
{
    return mPnts.size();
}

void zchxDrawTool::endDraw()
{
    clearPoints();
    if(mWidget) mWidget->releaseDrawStatus();
}

void zchxDrawTool::clearPoints()
{
    mPnts.clear();
}

bool zchxDrawTool::isReady()
{
    if(!mWidget || !mWidget->framework() || mPnts.empty()) return false;
    return true;
}

QList<QAction*> zchxDrawTool::getRightMenuActions(const QPoint& pt)
{
    QList<QAction*> list;
    list.append(addAction(tr("重新开始"),this, SLOT(startDraw()), 0));
    if(mWidget){
        list.append(addAction(tr("结束"), mWidget, SLOT(releaseDrawStatus()), 0));
    }
    return list;
}

QAction* zchxDrawTool::addAction(const QString &text, const QObject *obj, const char* slot, void* userData)
{
    if(!obj || !slot || strlen(slot) == 0) return 0;

    QAction *result(new QAction(text, this));
    if(QString(slot).contains("(bool)"))
        QObject::connect(result, SIGNAL(triggered(bool)), obj, slot);
    else
        QObject::connect(result,SIGNAL(triggered()),obj,slot);

    if(userData) result->setData(QVariant::fromValue(userData));
    return result;
}

void zchxDrawTool::setElement(MoveElement *ele)
{
    mEle = ele;
}

MoveElement* zchxDrawTool::element()
{
    return mEle;
}


}




