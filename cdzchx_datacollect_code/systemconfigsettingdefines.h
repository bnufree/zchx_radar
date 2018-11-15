#ifndef SYSTEMCONFIGSETTINGDEFINES_H
#define SYSTEMCONFIGSETTINGDEFINES_H

#include <QString>
#include <QStringList>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QList>
#include <QIODevice>
#include <QLabel>
#include <QSerialPort>
#include <QDebug>

enum COMTYPE
{
    COM_NONE = -1,
    COM_SURFACE_HMR = 0,
    COM_GPS,
    COM_USBL,
    COM_UNDER_HMR,
    COM_UNDER_4017_1,
    COM_UNDER_4017_2,
    COM_SURFACE_4017,
    COM_METER_COUNTER,
    COM_NAVI_DEV,
    COM_CABLE_DEV,
    COM_DP_DEV,
};

typedef struct tagComDevParameters
{
    QIODevice::OpenMode             mOpenMode;
    QString         mName;          //串口名
    int             mBaudRate;      //波特率
    QString         mMessageIdentifier;     //消息格式
    bool            mStatus;        //是否启用
    QString             mMessageNum1;    //地址
    QString             mMessageNum2;
    QString         mTopic;   //其他描述
    int             mParity;
    int             mStopBit;
    int             mDataBit;


    tagComDevParameters()
    {
        mOpenMode = QIODevice::ReadOnly;
        mName = "";
        mBaudRate = QSerialPort::Baud4800;
        mStatus = false;
        mMessageNum1 = "";
        mMessageNum2 = "";
        mTopic = "";
        mMessageIdentifier.clear();
        mParity = QSerialPort::NoParity;
        mStopBit = QSerialPort::OneStop;
        mDataBit = QSerialPort::Data8;
    }

    bool operator ==(const tagComDevParameters& other)
    {
        return this->mName == other.mName &&
                this->mBaudRate == other.mBaudRate;
    }
}COMDEVPARAM;

typedef     QList<COMDEVPARAM>      ComDevParamList;

typedef struct tagDevAnalyticalParam
{
    int id;
    int equipmentId;
    QString chineseName;
    QString name;
    QString code;
    QString channel;
    double  deviation;
    double deviationCoefficient;
    QString remark;
    bool    sts;

    tagDevAnalyticalParam()
    {
        id = -1;
        equipmentId = -1;
        chineseName = "";
        name = "";
        code = "";
        channel = "";
        deviation = 0;
        deviationCoefficient = 0;
        remark = "";
        sts = false;
    }
}DEVANALYPARAM;

class Widget4017
{
public:
    Widget4017()
    {
        mKey = "";
        mFunctionLabel = 0;
        mChannelComboBox = 0;
        mOffsetSpinBox = 0;
        mOffsetCoeffSpinBox = 0;
    }

    Widget4017(const QString& key, QLabel* pLabel = 0, QComboBox* pComBox = 0, QDoubleSpinBox* p1 = 0, QDoubleSpinBox* p2 = 0)
    {
        mKey = key;
        mFunctionLabel = pLabel;
        mChannelComboBox = pComBox;
        mOffsetSpinBox = p1;
        mOffsetCoeffSpinBox = p2;
        if(mChannelComboBox)
        {
            if(mChannelComboBox->count() == 0)
            {
                for(int i=0 ;i<8; i++)
                {
                    mChannelComboBox->addItem(QString("CH%1").arg(i));
                }
            }
        }
    }

    void setCurValue(bool isChecked, const QString& text, double offset, double offset_coeff)
    {
        //if(mFunctionLabel) mFunctionCheckBox->setChecked(isChecked);
        if(mChannelComboBox) mChannelComboBox->setCurrentText(text);
        if(mOffsetSpinBox) mOffsetSpinBox->setValue(offset);
        if(mOffsetCoeffSpinBox) mOffsetCoeffSpinBox->setValue(offset_coeff);
    }

    void setCurValue(const DEVANALYPARAM& param)
    {
        //if(mFunctionCheckBox) mFunctionCheckBox->setChecked(param.sts);
        if(mChannelComboBox) mChannelComboBox->setCurrentText(param.channel);
        if(mOffsetSpinBox) mOffsetSpinBox->setValue(param.deviation);
        if(mOffsetCoeffSpinBox) mOffsetCoeffSpinBox->setValue(param.deviationCoefficient);
    }

    QString getChineseName()
    {
        if(mFunctionLabel) return mFunctionLabel->text();
        return QString();
    }

    bool getFuncStatus()
    {
        return true;
    }

    double getOffset()
    {
        if(mOffsetSpinBox) return mOffsetSpinBox->value();
        return 0;
    }

    double getOffsetCoeff()
    {
        if(mOffsetCoeffSpinBox) return mOffsetCoeffSpinBox->value();
        return 0.0;
    }

    QString getChannelName()
    {
        if(mChannelComboBox) return mChannelComboBox->currentText();
        return QString();
    }

    QString getKey()
    {
        return mKey;
    }

private:
    QLabel   *mFunctionLabel;
    QComboBox   *mChannelComboBox;
    QDoubleSpinBox  *mOffsetSpinBox;
    QDoubleSpinBox  *mOffsetCoeffSpinBox;
    QString     mKey;

};

class WidgetComSetting
{
public:
    WidgetComSetting(const QString& key, QCheckBox* pCheckBox = 0, QComboBox* pComName = 0, QComboBox* pBaudRate = 0, QLineEdit* pMsgHead =0, QComboBox* pParity = 0, QComboBox* pDataBit = 0, QComboBox *pStopBit = 0, QLineEdit* pMsgNum1 = 0, QLineEdit* pMsgNum2 = 0)
    {
        mKey = key;
        mFunctionCheckBox = pCheckBox;
        mComNameBox = pComName;
        mBaudrateBox = pBaudRate;
        mMsgHead = pMsgHead;
        mParityBox = pParity;
        mDataBitBox = pDataBit;
        mStopBitBox = pStopBit;
        mMsgNumBox1 = pMsgNum1;
        mMsgNumBox2 = pMsgNum2;
    }

    WidgetComSetting()
    {
        mKey = "";
        mFunctionCheckBox = 0;
        mComNameBox = 0;
        mBaudrateBox = 0;
        mMsgHead = 0;
        mParityBox = 0;
        mStopBitBox = 0;
        mDataBitBox = 0;
        mMsgNumBox1 = 0;
        mMsgNumBox2 = 0;
    }

    void setCurValue(bool isChecked, const QString& com_name, const QString& baudrata_name, const QString& msgnum_name1, const QString& msgnum_name2, const QString& head, int parity, int databit, int stopbit)
    {
        if(mFunctionCheckBox) mFunctionCheckBox->setChecked(isChecked);
        if(mComNameBox) mComNameBox->setCurrentText(com_name);
        if(mBaudrateBox) mBaudrateBox->setCurrentText(baudrata_name);
        if(mMsgNumBox1) mMsgNumBox1->setText(msgnum_name1);
        if(mMsgNumBox2) mMsgNumBox2->setText(msgnum_name2);
        if(mMsgHead) mMsgHead->setText(head);
        if(mDataBitBox)
        {
            for(int i=0; i<mDataBitBox->count(); i++)
            {
                if(mDataBitBox->itemData(i).toInt() == databit)
                {
                    mDataBitBox->setCurrentIndex(i);
                    break;
                }
            }
        }
        if(mParityBox)
        {
            for(int i=0; i<mParityBox->count(); i++)
            {
                if(mParityBox->itemData(i).toInt() == parity)
                {
                    mParityBox->setCurrentIndex(i);
                    break;
                }
            }
        }

        if(mStopBitBox)
        {
            for(int i=0; i<mStopBitBox->count(); i++)
            {
                if(mStopBitBox->itemData(i).toInt() == stopbit)
                {
                    mStopBitBox->setCurrentIndex(i);
                    break;
                }
            }
        }

    }

    void setCurValue(const COMDEVPARAM& param)
    {
        qDebug()<<"set param:"<<param.mTopic;
        if(mFunctionCheckBox) mFunctionCheckBox->setChecked(param.mStatus);
        if(mComNameBox) mComNameBox->setCurrentText(param.mName);
        if(mBaudrateBox) mBaudrateBox->setCurrentText(QString::number(param.mBaudRate));
        if(mMsgNumBox2) mMsgNumBox2->setText(param.mMessageNum2);
        if(mMsgNumBox1) mMsgNumBox1->setText(param.mMessageNum1);
        if(mMsgHead) mMsgHead->setText(param.mMessageIdentifier);
        if(mDataBitBox)
        {
            for(int i=0; i<mDataBitBox->count(); i++)
            {

                qDebug()<<i<<mDataBitBox->itemData(i).toInt()<<param.mDataBit;

                if(mDataBitBox->itemData(i).toInt() == param.mDataBit)
                {
                    mDataBitBox->setCurrentIndex(i);
                    break;
                }
            }
        }
        //qDebug()<<"parity box:"<<mParityBox;
        if(mParityBox)
        {
            for(int i=0; i<mParityBox->count(); i++)
            {
                //qDebug()<<i<<mParityBox->itemData(i).toInt()<<param.mParity;
                if(mParityBox->itemData(i).toInt() == param.mParity)
                {
                    mParityBox->setCurrentIndex(i);
                    break;
                }
            }
        }

        if(mStopBitBox)
        {
            for(int i=0; i<mStopBitBox->count(); i++)
            {
                if(mStopBitBox->itemData(i).toInt() == param.mStopBit)
                {
                    mStopBitBox->setCurrentIndex(i);
                    break;
                }
            }
        }
    }

    QString getChineseName()
    {
        if(mFunctionCheckBox) return mFunctionCheckBox->text();
        return QString();
    }

    bool getFuncStatus()
    {
        if(mFunctionCheckBox) return mFunctionCheckBox->isChecked();
        return false;
    }

    QString getMsgNum1()
    {
        if(mMsgNumBox1) return mMsgNumBox1->text();
        return "";
    }

    QString getMsgNum2()
    {
        if(mMsgNumBox2) return mMsgNumBox2->text();
        return "";
    }

    int getBaudrate()
    {
        if(mBaudrateBox) return mBaudrateBox->currentText().toInt();
        return -1;
    }

    QString getComName()
    {
        if(mComNameBox) return mComNameBox->currentText();
        return QString();
    }

    QString getKey()
    {
        return mKey;
    }

    QString getMsgHead()
    {
        if(mMsgHead) return mMsgHead->text().trimmed();
        return QString();
    }

    int getStopBit()
    {
        if(mStopBitBox) return mStopBitBox->currentData().toInt();
        return 0;
    }

    int getParity()
    {
        if(mParityBox) return mParityBox->currentData().toInt();
        return 0;
    }

    int getDataBit()
    {
        if(mDataBitBox) return mDataBitBox->currentData().toInt();
    }

private:
    QCheckBox   *mFunctionCheckBox;
    QComboBox   *mComNameBox;
    QComboBox   *mBaudrateBox;
    QLineEdit   *mMsgNumBox1;
    QLineEdit   *mMsgNumBox2;
    QLineEdit   *mMsgHead;
    QString     mKey;
    QComboBox   *mParityBox;
    QComboBox   *mDataBitBox;
    QComboBox   *mStopBitBox;

};


//status 0:Normal
struct  HMR3000
{
    double  head;
    int    head_status;
    double  pitch;
    int    pitch_status;
    double  roll;
    int    roll_status;
};

//配置参数等的设定
//水下传感器的阈值设定
#define     UNDER_WATER_THRESHOLD               "UnderWaterThreshold"
#define     UNDER_WATER_MAX_PITCH               "under_water_max_pitch"             //最大纵倾
#define     UNDER_WATER_MAX_ROLL                "under_water_max_roll"              //最大横倾
#define     UNDER_WATER_MAX_DEPTH               "under_water_max_depth"
#define     UNDER_WATER_MIN_HEIGHT              "under_water_min_height"
#define     UNDER_WATER_MAX_POWER1              "under_water_max_power1"
#define     UNDER_WATER_MAX_POWER2              "under_water_max_power2"
#define     UNDER_WATER_MAX_POWER3              "under_water_max_power3"

//水面传感器的阈值设定
#define     SURFACE_WATER_THRESHOLD             "SurfaceWaterThreshold"
#define     SURFACE_WATER_MAX_PITCH             "surface_water_max_pitch"             //最大纵倾
#define     SURFACE_WATER_MAX_ROLL              "surface_water_max_roll"              //最大横倾
#define     SURFACE_WATER_MAX_DEPTH             "surface_water_max_depth"
#define     SURFACE_WATER_MAX_POWER1            "surface_water_max_power1"
#define     SURFACE_WATER_MAX_POWER2            "surface_water_max_power2"
#define     SURFACE_WATER_MAX_POWER3            "surface_water_max_power3"

//拖体参数设定
#define     TOW_BODY_SET                "TowBodyParam"
#define     FORWARD_LENGTH              "forward_length"
#define     BACKWARD_LENGTH             "backward_length"
#define     BOOTS_LENGTH                "boots_body_length"
#define     TOW_BODY_RANGE              "tow_body_range"
#define     BOOTS_HEIGHT                "boots_height"

//其他参数设定
#define     OTHER_PARAM_SET             "OtherSet"
#define     METER_COUNTER_COEFF         "meter_counter_coeff"
#define     METER_COUNTER_INIT          "meter_counter_init_value"
#define     REPORT_STEP                 "report_step"
#define     GPS_LONOFFSET                  "gps_lon_deviation"
#define     GPS_LATOFFSET                  "gps_lat_deviation"
#define     GPS_SHIP_LENGTH                  "gps_ship_length_deviation"
#define     GPS_SHIP_AHEAD              "ship_head_ahead"
#define     STORAGE_TIME                "storage_time"
#define     DISPLAY_TIME                "display_time"
#define     SURFACE_HMR_OFFSET          "surface_hmr_deviation"
#define     UNDER_HMR_OFFSET            "under_hmr_deviation"
#define     TENSION_COEFF               "tension_coeff"
#define     SPEED_TIME_GAP              "speed_time_gap"


//通讯COM设定
#define     MSG_COM_SET                 "COM"
#define     MSG_SURFACE_HMR             "surface_hmr"
#define     MSG_GPS                     "gps"
#define     MSG_USBL                    "usbl"
#define     MSG_UNDER_HMR               "under_hmr"
#define     MSG_UNDER_4017             "under_4017"
#define     MSG_SURFACE_4017            "surface_4017"
#define     MSG_METER_COUNTER           "meter_counter"
#define     MSG_NAVI_DEV                "navigation_device"
#define     MSG_CABLE_DEV               "cable_device"
#define     MSG_DP_DEV                  "dp"
#define     MSG_DP_UPLOAD_DEV           "dp_upload"
#define     MSG_TENSION_DEV             "tension"

//水下4017的参数
#define     UNDER_WATER_4017_1_SET      "UnderWater4017_1"
#define     UNDER_WATER_4017_2_SET      "UnderWater4017_2"
#define     SURFACE_WATER_4017_SET      "SurfaceWater4017"
#define     DEPTH_4017                  "depth"
#define     BOOTS_ANGLE_4017            "angle"
#define     TOUCHDOWN_4017_1            "touchdown1"
#define     TOUCHDOWN_4017_2            "touchdown2"
#define     TOUCHDOWN_4017_3            "touchdown3"
#define     TOUCHDOWN_4017_4            "touchdown4"
#define     PULL_4017_1                 "pull1"
#define     PULL_4017_2                 "pull2"
#define     PULL_4017_3                 "pull3"
#define     LEFT_PUMP_4017              "left_pump"
#define     RIGHT_PUMP_4017             "right_pump"



#endif // SYSTEMCONFIGSETTINGDEFINES_H
