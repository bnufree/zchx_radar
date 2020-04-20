#ifndef DRAWAISTRACK_H
#define DRAWAISTRACK_H

#include <QWidget>
#include <QObject>
#include <QThread>
#include <QMap>
#include <QPointF>
#include "./ais_radar/zchxfunction.h"
#include <QPolygonF>

class drawaistrack : public QObject
{
    Q_OBJECT
public:
    explicit drawaistrack(QObject *parent = 0);
    ~drawaistrack();
signals:
    void signalDrawAisTrack(int, double, double);
    void signalGetPixmap(QPixmap, int, int);
    void signalSendAisPix(QPixmap);

public slots:
    void slotDrawAisTrack(int, double, double);
    void drawTrackPixmap();
    void slotGetPixmap(QPixmap, int, int);

private:
    QThread m_threadWork;
    QPixmap pix;
    int xLevel = 850, yLevel = 600, aLevel = 1;

};

#endif // DRAWAISTRACK_H
