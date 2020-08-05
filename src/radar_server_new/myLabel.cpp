#include "myLabel.h"
#include <qdebug.h>
myLabel::myLabel(QWidget *parent) : QLabel(parent)
{
    this->resize(850, 600);
    wid = this->width();
    hei = this->height();
    qDebug()<<"wid hei"<<wid<<hei;
}

void myLabel::wheelEvent(QWheelEvent *event)
{
    static int i = 1;
    if(event->delta()>0){//如果滚轮往上滚
        ++i;
       this->resize(wid * i, hei * i);
       emit signalScaleLevel(i);
    }else{//同样的
        if(i > 1)//大于最小宽度
        {
            --i;
            this->resize(hei*i, hei * i);
        }
            emit signalScaleLevel(i);
    }
    qDebug()<<"wid hei"<<wid<<hei<<i<<this->width()<<this->height();
}
