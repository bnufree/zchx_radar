#ifndef ZCHXVIDEORECTS_H
#define ZCHXVIDEORECTS_H

#include <QObject>
#include <QThread>
#include "zchxfunction.h"
#include "zchxradarcommon.h"
#include <QPolygonF>

#define MAX_LINE_NUM 2048
#define MAX_CELL_NUM 512

class zchxVideoRects : public QObject
{
    Q_OBJECT
public:
    explicit zchxVideoRects(int m_uSourceID ,QObject *parent = 0);
    ~zchxVideoRects();
signals:
    void signalSendTrackNodes(QMap<int, QList<TrackNode>>);
    void signalDealViedoTrack(QList<TrackNode>,int);
    void signalShowTheLastPot(QList<QPointF>,QList<QPointF> );
public slots:
    void slotDealViedoTrack(QList<TrackNode>);
    void analysisVideoPieceSlot(QMap<int,RADAR_VIDEO_DATA>,double);
    bool inLimitAreaForTrack(const double dLat, const double dLon);
    void analysisLonLatTOPolygon(const QString sFileName, QList<QPolygonF> &landPolygon, QList<QPolygonF> &seaPolygon);
    void readRadarLimitFormat();
    bool isFinishProcess();
private:
    void recursionSearchProcess(int i, QList<TrackNode> &nodeList, int searchTable[MAX_LINE_NUM][MAX_CELL_NUM], QList<TrackNode> &list);
    void nextRecursion(int nextX, int nextY, QList<TrackNode> &nodeList, int searchTable[MAX_LINE_NUM][MAX_CELL_NUM], QList<TrackNode> &list);

    double m_dCentreLon;
    double m_dCentreLat;
    QThread m_workThread;

    bool finish;//判断识别函数是否运行完成.

    //限制区域
    bool m_bLimit;//是否设置限制区域
    QList<QPolygonF> m_landPolygon;
    QList<QPolygonF> m_seaPolygon;
    QString m_limit_file;//读取限制区域文件
    QPolygonF land_limit;//区域限制点集合
    QPolygonF sea_limit;//区域限制点集合
    double track_max_radius = 10000; //雷达回波块大小识别区间
    double track_min_radius = 0;
    QString str_radar;
    int increaseNum;//增加的点数
    int g_searchTable[MAX_LINE_NUM][MAX_CELL_NUM];
    double rangeFactor;
    int sizeOfnodes;
    int num;
};

#endif // ZCHXVIDEORECTS_H
