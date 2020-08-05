#ifndef ZCHXRADARCTRLBTN_H
#define ZCHXRADARCTRLBTN_H

#include <QPushButton>
#include "zchxradaroptwidget.h"

class zchxRadarOptWidget;
class ZCHXRadarDataServer;

enum PowerStatus{
    POWER_STANDBY = 0,
    POWER_TRANSMIT,
    POWER_OFF,
};

enum ScanSpeed{
    SPEED_NORMAL = 0,
    SPEED_FAST,
};

enum HLOStatusData{
    HLO_OFF = 0,
    HLO_LOW = 1,
    HLO_HIGH = 2,
};

enum HMLOStatusData{
    HMLO_OFF = 0,
    HMLO_LOW = 1,
    HMLO_MIDDLE = 2,
    HMLO_HIGH = 3,
};

enum OpenStatus
{
    OFF= 0,
    ON = 1,
};


class zchxRadarCtrlBtn : public QPushButton
{
    Q_OBJECT
public:
    explicit zchxRadarCtrlBtn(QWidget *parent = 0);    
    void    setServer(ZCHXRadarDataServer* server);
    void    setType(int type) {mType = type;}
    void    setAdjustmode(int sts) {mAdjustMode = sts;}
    int     getValue() const {return mCur;}

private:
    QString getPowerString(int sts);
    QString getScanSpeedString(int sts);
    QString getHLOString(int sts);
    QString getHMLOString(int sts);
    QString getOpenString(int sts);
private:
    void    setAutoAvailable(bool sts) {mAutoAvailable = sts;}
signals:

public slots:
    void setValue(int value);
    void btnClicked();
    void slotBtnClose();
    void slotConfigChanged(int);
private:
    zchxRadarOptWidget*  mOptWidget;
    bool                 mAutoAvailable;
    ZCHXRadarDataServer* mServer;
    int                  mMax;
    int                  mMin;
    int                  mType;
    int                  mCur;
    int                  mAdjustMode;
};

#endif // ZCHXRADARCTRLBTN_H
