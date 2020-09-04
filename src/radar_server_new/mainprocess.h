#ifndef MAINPROCESS_H
#define MAINPROCESS_H

#include <QObject>
#include <QJsonDocument>
#include "zchxmsgcommon.h"

#define MainProc        MainProcess::instance()
class ZCHXAnalysisAndSendRadar;

class MainProcess : public QObject
{
    Q_OBJECT
private:
    explicit MainProcess(QObject *parent = 0);
private:
    void    initConfig();
public:
    static MainProcess* instance();
    void    start();
    bool    isStart() const {return mStartFlag;}
    bool     processFilterAreaMsg(int cmd, const zchxMsg::filterArea& area);
    void    apendRadarAnalysisServer(int site_id, ZCHXAnalysisAndSendRadar* server);

signals:

private:
    static  MainProcess* m_pInstance;
    bool    mStartFlag;
    QJsonDocument       mCfgDoc;
    QMap<int, ZCHXAnalysisAndSendRadar*>        mRadarAnalysisMap;


};

#endif // MAINPROCESS_H
