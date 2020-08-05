#ifndef ZCHXRADAROPTWIDGET_H
#define ZCHXRADAROPTWIDGET_H

#include <QWidget>

namespace Ui {
class zchxRadarOptWidget;
}

enum AdjustMode{
    SMALL_ADJUST_MODE = 0x01,  //仅仅10以内调整
    MIDE_ADJUST_MODE = 0x02,   //10或者100为精度调整
    LARGE_ADJUST_MODE = 0x04,  //50以上精度调整
    FULL_ADJUST_MODE = SMALL_ADJUST_MODE | MIDE_ADJUST_MODE | LARGE_ADJUST_MODE,  //全部

};

class zchxRadarOptWidget : public QWidget
{
    Q_OBJECT

public:
    explicit zchxRadarOptWidget(QWidget *parent = 0);
    ~zchxRadarOptWidget();
    void setRange(int min, int max) {mMin = min; mMax = max;}
    void setAuto(bool sts);
    void setAutoBtnAvailable(bool sts);
    void set10BtnAvailale(bool sts);
    void setSource(int v) {mSourcID = v;}
    void setType(int v) {mType = v;}
    void setWidth(int w);
    void setAdjustMode(int mode);

signals:
    void signalClose();
    void signalConfigChanged(int);
public slots:
    void setCurrentVal(int val);
    void setServerValue(int val);

private slots:
    void on_btn_auto_clicked();

    void on_btn_10_plus_clicked();

    void on_btn_plus_clicked();

    void on_btn_minus_clicked();

    void on_btn_10_minus_clicked();

    void on_btn_close_clicked();

    void on_btn_50_minus_clicked();

    void on_btn_100_minus_clicked();

    void on_btn_1000_minus_clicked();

    void on_btn_50_plus_clicked();

    void on_btn_100_plus_clicked();

    void on_btn_1000_plus_clicked();

private:
    Ui::zchxRadarOptWidget *ui;
    int                     mMax;
    int                     mMin;
    bool                    mIsAutoMode;
    int                     mCur;
    int                     mSourcID;
    int                     mType;
    int                     mAdjusMode;
    bool                    mAutoBtnVisible;
};

#endif // ZCHXRADAROPTWIDGET_H
