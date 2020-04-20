#include "scrollarea.h"
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>

ScrollArea::ScrollArea(QWidget* parent)
    : QScrollArea(parent)
    , mMoveStart(false)
{
    installEventFilter(this);
    horizontalScrollBar()->setStyleSheet("QScrollBar:horizontal"
                                         "{"
                                         "height:8px;"
                                         "background:rgba(0,0,0,0%);"
                                         "margin:0px,0px,0px,0px;"
                                         "padding-left:0px;"
                                         "padding-right:0px;"
                                         "}"
                                         "QScrollBar::handle:horizontal"
                                         "{"
                                         "height:8px;"
                                         "background:rgba(0,0,0,25%);"
                                         " border-radius:4px;"
                                         "min-height:20;"
                                         "}"
                                         "QScrollBar::handle:horizontal:hover"
                                         "{"
                                         "height:8px;"
                                         "background:rgba(0,0,0,50%);"
                                         " border-radius:4px;"
                                         "min-height:20;"
                                         "}"
                                         "QScrollBar::add-line:horizontal"
                                         "{"
                                         "height:0px;width:0px;"
                                         "subcontrol-position:bottom;"
                                         "}"
                                         "QScrollBar::sub-line:horizontal"
                                         "{"
                                         "height:0px;width:0px;"
                                         "subcontrol-position:top;"
                                         "}"
                                         "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal"
                                         "{"
                                         "background:rgba(0,0,0,10%);"
                                         "border-radius:4px;"
                                         "}"
                                         );
    verticalScrollBar()->setStyleSheet("QScrollBar:vertical"
                                       "{"
                                       "width:8px;"
                                       "background:rgba(0,0,0,0%);"
                                       "margin:0px,0px,0px,0px;"
                                       "padding-top:0px;"
                                       "padding-bottom:0px;"
                                       "}"
                                       "QScrollBar::handle:vertical"
                                       "{"
                                       "width:8px;"
                                       "background:rgba(0,0,0,25%);"
                                       " border-radius:4px;"
                                       "min-height:20;"
                                       "}"
                                       "QScrollBar::handle:vertical:hover"
                                       "{"
                                       "width:8px;"
                                       "background:rgba(0,0,0,50%);"
                                       " border-radius:4px;"
                                       "min-height:20;"
                                       "}"
                                       "QScrollBar::add-line:vertical"
                                       "{"
                                       "height:0px;width:0px;"
                                       "subcontrol-position:bottom;"
                                       "}"
                                       "QScrollBar::sub-line:vertical"
                                       "{"
                                       "height:0px;width:0px;"
                                       "subcontrol-position:top;"
                                       "}"
                                       "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical"
                                       "{"
                                       "background:rgba(0,0,0,10%);"
                                       "border-radius:4px;"
                                       "}"
                                       );
}

ScrollArea::~ScrollArea()
{

}

bool ScrollArea::eventFilter(QObject *obj, QEvent *evt)
{
    if (evt->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = (QMouseEvent*) evt;
        if ((mouseEvent->buttons() & Qt::LeftButton)) {
            if (!mMoveStart) {
                mMoveStart = true;
                mContinuousMove =false;
                mMousePoint = mouseEvent->globalPos();
            }

            else {
                QPoint mousePoint = mouseEvent->globalPos();

                int x_offset = mousePoint.y() - mMousePoint.y();
                int y_offset = mousePoint.x() - mMousePoint.x();
                mContinuousMove = true;

                setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);//开启滚动条
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

                QScrollBar *vScrollBar=verticalScrollBar();
                QScrollBar *hScrollBar=horizontalScrollBar();

                vScrollBar->setValue(vScrollBar->value() - x_offset);
                hScrollBar->setValue(hScrollBar->value() - y_offset);

                mMousePoint = mousePoint;
            }
            return true;
        }
    }
    else if (evt->type() == QEvent::MouseButtonRelease) {
        mMoveStart = false;
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//关闭滚动条
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    return QObject::eventFilter(obj, evt);
}

void ScrollArea::wheelEvent(QWheelEvent *event)//重写滚轮事件
{

}
