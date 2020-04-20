#include "zchxradarctrlbtn.h"
#include "zchxradaroptwidget.h"
#include "ais_radar/zchxradardataserver.h"

zchxRadarCtrlBtn::zchxRadarCtrlBtn(QWidget *parent) : QPushButton(parent)
{
    mType = -1;
    mMin = -1;
    mMax = INT_MAX;
    mServer = 0;
    mOptWidget = 0;
    mAutoAvailable = false;
    mAdjustMode = MIDE_ADJUST_MODE;
    connect(this, SIGNAL(clicked(bool)), this, SLOT(btnClicked()));
}

void zchxRadarCtrlBtn::setValue(int value)
{
    if(mOptWidget) mOptWidget->setCurrentVal(value);
    QString text = QString::number(value);
    switch (mType) {
    case SCAN_SPEED:
        text = getScanSpeedString(value);
        break;
    case NOISE_REJECTION:
    case TARGET_BOOST:
        text = getHLOString(value);
        break;
    case INTERFERENCE_REJECTION:
    case LOCAL_INTERFERENCE_REJECTION:
    case TARGET_SEPARATION:
        text = getHMLOString(value);
        break;
    case TARGET_EXPANSION:
        text = getOpenString(value);
        break;
    case POWER:
        text = getPowerString(value);
        break;
    default:
        break;
    }
    if(mAutoAvailable && value == -1)
    {
        text = QStringLiteral("自动");
    }

    setText(text);
    mCur = value;
}

void zchxRadarCtrlBtn::btnClicked()
{
    if(mAdjustMode != SMALL_ADJUST_MODE)
    {
        if(!parentWidget()) return;
        if(!mOptWidget)
        {
            mOptWidget = new zchxRadarOptWidget();
            connect(mOptWidget, SIGNAL(signalClose()), this, SLOT(slotBtnClose()));
            connect(mOptWidget, SIGNAL(signalConfigChanged(int)), this, SLOT(slotConfigChanged(int)));
        }
        //将目标移动到当前位置下方
        mOptWidget->setAdjustMode(mAdjustMode);
        mOptWidget->setAutoBtnAvailable(mAutoAvailable);
        mOptWidget->setRange(mMin, mMax);
        mOptWidget->setCurrentVal(mCur);
        mOptWidget->setWidth(this->width());
        QRect rect(QPoint(0,0), mOptWidget->size());
        QPoint pos = parentWidget()->mapToGlobal(this->geometry().bottomLeft());
        rect.moveTopLeft(pos);
        mOptWidget->setGeometry(rect);
        mOptWidget->show();
    } else
    {
        if(mMax <= 0) return;
        slotConfigChanged((mCur + 1) % (mMax+1));
    }

}

void zchxRadarCtrlBtn::slotBtnClose()
{
    if(mOptWidget)
    {
        delete mOptWidget;
        mOptWidget = 0;
    }
}

void zchxRadarCtrlBtn::setServer(ZCHXRadarDataServer *server)
{
    mServer = server;
    if(mServer)
    {
        mServer->getControlValueRange((INFOTYPE)mType, mMin, mMax);
        mAutoAvailable = mServer->getControlAutoAvailable((INFOTYPE)mType);
    }
}

void zchxRadarCtrlBtn::slotConfigChanged(int value)
{
    if(mServer)
    {
        mServer->setControlValue((INFOTYPE)mType, value);
    }
}

QString zchxRadarCtrlBtn::getPowerString(int sts)
{
    if(sts == POWER_STANDBY) return QStringLiteral("待机");
    if(sts == POWER_TRANSMIT) return QStringLiteral("传输");
    if(sts == POWER_OFF) return QStringLiteral("断开");
    return QStringLiteral("未知");
}

QString zchxRadarCtrlBtn::getScanSpeedString(int sts)
{
    qDebug()<<"speed sts:"<<sts;
    if(sts == SPEED_NORMAL) return QStringLiteral("正常");
    if(sts == SPEED_FAST) return QStringLiteral("快速");
    return QStringLiteral("未知");

}

QString zchxRadarCtrlBtn::getHLOString(int sts)
{
    if(sts == HLO_LOW) return QStringLiteral("低");
    if(sts == HLO_HIGH) return QStringLiteral("高");
    if(sts == HLO_OFF) return QStringLiteral("关闭");
    return QStringLiteral("未知");
}

QString zchxRadarCtrlBtn::getHMLOString(int sts)
{
    if(sts == HMLO_LOW) return QStringLiteral("低");
    if(sts == HMLO_HIGH) return QStringLiteral("高");
    if(sts == HMLO_MIDDLE) return QStringLiteral("中");
    if(sts == HMLO_OFF) return QStringLiteral("关闭");
    return QStringLiteral("未知");
}

QString zchxRadarCtrlBtn::getOpenString(int sts)
{
    if(sts == OFF) return QStringLiteral("关闭");
    if(sts == ON) return QStringLiteral("开启");
    return QStringLiteral("未知");
}


