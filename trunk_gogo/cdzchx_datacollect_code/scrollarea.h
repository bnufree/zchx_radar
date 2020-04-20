#ifndef SCROLLAREA_H
#define SCROLLAREA_H

#include <QScrollArea>
#include <QPoint>

class ScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    ScrollArea(QWidget* parent =NULL);
    ~ScrollArea();

protected:
    bool eventFilter(QObject *obj, QEvent *evt);
    void wheelEvent(QWheelEvent *event);//重写滚轮事件

 private:
    bool mMoveStart;
    bool mContinuousMove;
    QPoint mMousePoint;

};
#endif // SCROLLAREA_H
