#include "comdatamgr.h"
#include "globlefun/glogfunction.h"
#include "Log.h"
#include "common.h"
//#include <QDebug>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

ComDataMgr::ComDataMgr(QObject *parent) : QObject(parent),\
    mParser(new ComParser)
{
    this->moveToThread(&mWorkThread);
    mWorkThread.start();
}

ComDataMgr::~ComDataMgr()
{
    stop();
}

void ComDataMgr::start()
{
    QStringList startComList = mDevComParamsMap.keys();
    foreach (QString key, startComList) {
        COMDEVPARAM dev = mDevComParamsMap.value(key);
        bool open_new_com = false;
        //如果对应的串口已经启动
        if(mStartedComList.contains(key))
        {
            //串口已经启动
            ComDataWorker *recv = mStartedComList.value(key, NULL);
            if(recv)
            {
                //检查参数是否发生了变化，如果有变化就停止串口工作，再重新开始检查是否开始
                if(recv->param() == dev)
                {
                    //参数值相同，不做任何处理
                    continue;
                } else
                {
                    //参数值不相同,停止串口
                    stopComWork(recv);
                    if(dev.mStatus)
                    {
                        //串口仍然继续打开
                        open_new_com = true;
                    }
                }
            }
        } else if(dev.mStatus)
        {
            open_new_com = true;
        }

        //如果串口没有选择停止，根据新的参数启动
        if(!open_new_com) continue;
        //开启串口，读取
        ComDataWorker *recv = new ComDataWorker(dev);
        connect(recv,SIGNAL(signalReciveComData(QString,QString,qint64,QByteArray)), this,SLOT(slotRecvComData(QString,QString,qint64,QByteArray)));
        connect(recv, SIGNAL(signalSerialPortErrorStr(QString)), this, SIGNAL(signalSendLogMsg(QString)));
        if(recv->open())
        {
            emit signalSendLogMsg(tr("com opend success(%1:%2). openmode:%3, parity:%4, databits:%5, stopbits:%6").
                                  arg(dev.mTopic).arg(dev.mName).arg(dev.mOpenMode).arg(dev.mParity).arg(dev.mDataBit).arg(dev.mStopBit));
        }
        mStartedComList[dev.mTopic] = recv;

    }

}



void ComDataMgr::stop()
{
    foreach (ComDataWorker * data, mStartedComList.values()) {
        if(data)
        {
            data->stop();
            delete data;
        }
    }
    mWorkThread.quit();
    mWorkThread.wait(5);
}

void ComDataMgr::stopComWork(ComDataWorker *data)
{
    if(!data) return;
    data->stop();
    mStartedComList.remove(data->param().mTopic);
    delete data;
}

void ComDataMgr::setComDevParams(const QMap<QString, COMDEVPARAM>& map)
{
    //检查各个参数的更新情况
    foreach (QString key, map.keys()) {
        bool status_change = false;
        if(!mDevComParamsMap.contains(key)){
            status_change = true;
        } else
        {
            if(mDevComParamsMap[key].mStatus != map[key].mStatus)
            {
                status_change = true;
            }
        }
        cout<<"status_change && mParser"<<(status_change && mParser);
        if(status_change && mParser)
        {
            mParser->signalRecvComStatusChange(key, map[key].mStatus);
        }
    }

    //新参数重新更新串口的查询命令
    mDevComParamsMap = map;
    foreach (QString key, mDevComParamsMap.keys()) {
        //先更新设备窗口的情况
        COMDEVPARAM& dev = mDevComParamsMap[key];
        if(dev.mOpenMode == QIODevice::ReadWrite)
        {
            dev.mQueryCmd[0] = dev.mDevAddr;
            short crc = GlogFunction::instance()->CRCModbus16((uchar*)dev.mQueryCmd.left(6).constData(), 6);
            //int mode = (key == COM_WATER_DEV? HIGH_FIRST : LOW_FIRST);
            int mode = LOW_FIRST;
            QByteArray crc_code = GlogFunction::instance()->short2Bytes(crc, mode);
            dev.mQueryCmd[6] = crc_code[0];
            dev.mQueryCmd[7] = crc_code[1];
        }
        cout<<dev.mTopic<<dev.mQueryCmd.toHex();
    }
    //开始串口连接
    start();
}


void ComDataMgr::slotRecvComData(const QString& comName, const QString& topic, qint64 time, const QByteArray& recv)
{
    ComDataWorker *job = qobject_cast<ComDataWorker*> (sender());
    if(!job) return;

    bool change2HexFlag = true;
    QString saveContent;
    //解析数据，发送给接收到的客户端
    if(COM_GPS_DEV == topic)  change2HexFlag = false;
    //转发到列表显示接收到的数据日志
    if(change2HexFlag)
    {
        saveContent.append(comCmd2String(recv));
    } else {
        saveContent = QString::fromStdString(recv.constData());
    }

    QByteArray cmd = job->param().mQueryCmd;
    if(cmd.size() > 0)
    {
        saveContent.append("||");
        saveContent.append(comCmd2String(cmd));
    }
    emit signalSendRecvedContent(time, comName, saveContent);
    LOG(topic, (char*)saveContent.toStdString().data());
    //开始解析
    if(mParser)
    {
        QByteArray head;
        head.resize(3);
        head[0] = job->param().mDevAddr;
        head[1] = job->param().mFuncCode;
        head[2] = job->param().mDataBitLen;
        int one_reply_length = job->param().mRetCmdLength;
        mParser->signalRecvComData(head, one_reply_length, topic, time, recv);
    }
    return;
}


QString ComDataMgr::comCmd2String(const QByteArray &array)
{
    QByteArray bytes = array.toHex().toUpper();
    QString res;
    for(int i=0; i<bytes.size(); i=i+2)
    {
        if(i != 0)
        {
            res.append(" ");
        }
        res.append(bytes.mid(i, 2));
    }

    return res;
}





