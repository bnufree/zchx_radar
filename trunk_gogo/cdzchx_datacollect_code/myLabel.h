#ifndef MYLABEL_H
#define MYLABEL_H

#include <QLabel>
#include <QWheelEvent>

class myLabel : public QLabel
{
    Q_OBJECT
public:
    explicit myLabel(QWidget *parent = 0);

signals:
    void signalScaleLevel(int );

public slots:
//    void mousePressEvent(QMouseEvent *event);			//按下鼠标

//    void dragEnterEvent(QDragEnterEvent *event);		//拖动进入

//    void dragMoveEvent(QDragMoveEvent *event);			//拖动

//    void dropEvent(QDropEvent *event);				//放下
      void wheelEvent(QWheelEvent *event);//重写滚轮事件

private:
      int wid, hei;
};

#endif // MYLABEL_H
