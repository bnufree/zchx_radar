#ifndef RADARTESTWINDOW_H
#define RADARTESTWINDOW_H

#include <QMainWindow>
#include "qt/mainwindow_1.h"
#include "radarrecvthread.h"

namespace Ui {
class RadarTestWindow;
}

class QLabel;

class RadarTestWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit RadarTestWindow(QWidget *parent = 0);
    ~RadarTestWindow();
    void addMapLayer(const QString &name, const QString &displayName, bool visible);
public slots:
    void slotRecvRadarList(const TrackPointList& list);
    void slotTimeOut();
    void slotRecvCurPos(double, double);
    void slotDistanceMeasureEnable(bool sts);
    void slotMove();

private:
    Ui::RadarTestWindow *ui;
    MainWindow_1  *m_pEcdisWin;
    QLabel* label;
};

#endif // RADARTESTWINDOW_H
