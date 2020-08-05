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
    COM_GPS,
    COM_WLM,//水位计
    COM_NHN,
    COM_DDM,
    COM_ORP,
    COM_RDO,
    COM_ZS,
};

enum COM_CONTENT_TYPE{
    COM_CONTENT_UNDEF = 0,
    COM_CONTENT_TEXT,
    COM_CONTENT_BINARY,
};

typedef struct tagComDevParameters
{
    QIODevice::OpenMode             mOpenMode;
    QString         mName;          //串口名
    int             mBaudRate;      //波特率
    bool            mStatus;        //是否启用
    uchar           mDevAddr;    //地址
    uchar           mFuncCode;   //功能码
    uchar           mDataBitLen; //应答命令中有效数据长度
    uchar           mRetCmdLength;      //应答命令长度
    int             mParity;
    int             mStopBit;
    int             mDataBit;
    QByteArray      mQueryCmd;
    QString         mTopic;


    tagComDevParameters()
    {
        mOpenMode = QIODevice::ReadWrite;
        mName = "";
        mBaudRate = QSerialPort::Baud9600;
        mStatus = false;
        mDevAddr = 0;
        mFuncCode = 0;
        mDataBitLen = 0;
        mParity = QSerialPort::NoParity;
        mStopBit = QSerialPort::OneStop;
        mDataBit = QSerialPort::Data8;
        mQueryCmd.clear();
        mTopic = "";
    }

    bool operator ==(const tagComDevParameters& other)
    {
        return this->mOpenMode == other.mOpenMode &&
               this->mName == other.mName &&
               this->mBaudRate == other.mBaudRate &&
               this->mStatus == other.mStatus &&
               this->mDevAddr == other.mDevAddr &&
               this->mParity == other.mParity &&
               this->mStopBit == other.mStopBit &&
               this->mDataBit == other.mDataBit;
    }
}COMDEVPARAM;

typedef     QList<COMDEVPARAM>      ComDevParamList;

class WidgetComSetting
{
public:
    WidgetComSetting(const QString& key, QCheckBox* pCheckBox, QComboBox* pComName,\
                     QComboBox* pBaudRate, QLineEdit* pDevAddr, QComboBox* pParity,\
                     QComboBox* pDataBit, QComboBox *pStopBit)
    {
        mKey = key;
        mFunctionCheckBox = pCheckBox;
        mComNameBox = pComName;
        mBaudrateBox = pBaudRate;
        mDevddr = pDevAddr;
        mParityBox = pParity;
        mDataBitBox = pDataBit;
        mStopBitBox = pStopBit;
    }

    WidgetComSetting()
    {
        mKey = "";
        mFunctionCheckBox = 0;
        mComNameBox = 0;
        mBaudrateBox = 0;
        mDevddr = 0;
        mParityBox = 0;
        mStopBitBox = 0;
        mDataBitBox = 0;
    }

    void setCurValue(bool isChecked, const QString& com_name, const QString& baudrata_name, const QString& devaddr,
                     const QString& head, int parity, int databit, int stopbit)
    {
        if(mFunctionCheckBox) mFunctionCheckBox->setChecked(isChecked);
        if(mComNameBox) mComNameBox->setCurrentText(com_name);
        if(mBaudrateBox) mBaudrateBox->setCurrentText(baudrata_name);
        if(mDevddr) mDevddr->setText(devaddr);
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
        if(mDevddr) mDevddr->setText(QString::number(param.mDevAddr));
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

    QString getDevAddr()
    {
        if(mDevddr) return mDevddr->text();
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
    QComboBox   *mParityBox;
    QComboBox   *mDataBitBox;
    QComboBox   *mStopBitBox;    
    QLineEdit   *mDevddr;
    QLabel      *mCmdLabel;
    QString     mKey;

};

#endif // SYSTEMCONFIGSETTINGDEFINES_H
