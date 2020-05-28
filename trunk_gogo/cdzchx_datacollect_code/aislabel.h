#ifndef AISLABEL_H
#define AISLABEL_H

#include <QLabel>
#include <QLabel>
#include <QPoint>
#include <QColor>
#include <QPaintEvent>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QWheelEvent>
#include <QMap>
#include <QPolygonF>

class aisLabel : public QLabel
{
    Q_OBJECT
public:
    explicit aisLabel(QWidget *parent = 0);

signals:
    void signalScaleLevel(int );
    void signalAisPolygon(QPolygonF);

public slots:
    void mousePressEvent(QMouseEvent *event);			//按下鼠标

    void wheelEvent(QWheelEvent *event);//重写滚轮事件

    void slotCreatBtnClicked();

    void drawTrackPixmap();

    void drawTrackName(QString str);

    void slotRedrawAisTrack(QMap<int,QPolygonF>);

    void delThePoy();
private:
    QPixmap mPix;
    QPolygonF aisPolygon;
};


#endif // AISLABEL_H
