#include "zchxdrawlocalmarktool.h"
#include "zchxmapframe.h"
#include "localmarkdlg.h"

using namespace qt;
using namespace ZCHX::Data;

zchxDrawLocalMarkTool::zchxDrawLocalMarkTool(zchxMapWidget* w, QObject *parent) : zchxDrawTool(w, qt::eTool::DRAWLOCALMARK, parent)
{

}

void zchxDrawLocalMarkTool::appendPoint(const QPointF &pnt)
{
    ZCHX::Data::LatLon ll = pix2ll(pnt);
    if(mPnts.size() == 0) {
        mPnts.append(ll);
    } else {
        mPnts[0] = ll;
    }

}

void zchxDrawLocalMarkTool::endDraw()
{
    if(!isReady()) return ;
    LatLon ll = mPnts.last();
    LocalMarkDlg d;
    d.setLocalMarkPos(ll.lon, ll.lat);
    d.move(QCursor::pos().x() - d.width() / 2, QCursor::pos().y() - d.height() / 2 );
    if(d.exec() == QDialog::Accepted)
    {
        ZCHX::Data::ITF_LocalMark mark;
        d.getLocalMardData(mark);
        emit mWidget->signalCreateLocalMark(mark);
    }
    zchxDrawTool::endDraw();
    return;
}
