#ifndef NEW_GETTRACK_H
#define NEW_GETTRACK_H

#include <QWidget>
#include <QPolygonF>
#include <QObject>
#include <QThread>
#include <QMap>
#include <QPointF>
#include "./ais_radar/zchxfunction.h"
#include <QPolygonF>

class new_gettrack : public QObject
{
    Q_OBJECT
public:
    explicit new_gettrack(QObject *parent = 0);
    ~new_gettrack();

signals:
    void new_gettrack_signal(Afterglow);//利用信号槽开启识别目标线程
    void show_newtrack_signal(QPolygonF);//发送目标显示信号


public slots:

    void new_gettrack_slot(const Afterglow& objAfterglow);//新目标识别算法
private:
    QThread m_workThread;
    QPolygonF yel_pot_1;//区域限制点集合
    QPolygonF yel_pot_2;//区域限制点集合
    QPolygonF yel_pot_3;//区域限制点集合
};

#endif // NEW_GETTRACK_H
