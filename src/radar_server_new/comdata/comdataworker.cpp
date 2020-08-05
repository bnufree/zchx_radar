#include "comdataworker.h"
//#include <QDebug>
#include <QDateTime>
#include "Log.h"

#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

ComDataWorker::ComDataWorker(const COMDEVPARAM& param):
    mQueryTimerPtr(0),
    mQueryMode(false),
    mSerialPortPtr(0),
    QObject(0)
{    
    mParam = param;
    mSerialPortPtr = new QSerialPort;
    mSerialPortPtr->setPortName(mParam.mName);
    connect(mSerialPortPtr,SIGNAL(readyRead()),this,SLOT(slotReadComData()));
    connect(mSerialPortPtr,SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(slotRecvSerialPortErr(QSerialPort::SerialPortError)));
    connect(mSerialPortPtr, SIGNAL(bytesWritten(qint64)), this, SLOT(slotRecvWriteInfo(qint64)));
}

bool ComDataWorker::open()
{
    if(!mSerialPortPtr) return false;

    //检查参考是否已经打开
    if(mSerialPortPtr->isOpen())
    {
        //关闭串口
        mSerialPortPtr->close();
    }
    //重新打开串口
    if(!mSerialPortPtr->open(mParam.mOpenMode))
    {
        return false;
    }

    mSerialPortPtr->setBaudRate(mParam.mBaudRate);
    mSerialPortPtr->setDataBits((QSerialPort::DataBits) mParam.mDataBit);
    mSerialPortPtr->setParity((QSerialPort::Parity)mParam.mParity);
    mSerialPortPtr->setStopBits((QSerialPort::StopBits)mParam.mStopBit);
    mSerialPortPtr->setFlowControl(QSerialPort::NoFlowControl);
    mSerialPortPtr->setRequestToSend(true);
    mSerialPortPtr->setDataTerminalReady(true);
    if(mParam.mOpenMode & QIODevice::WriteOnly)
    {
        //需要向串口发送数据
        setQueryMode(true);
    }
    return true;
}

void ComDataWorker::stop()
{
    if(mQueryTimerPtr)
    {
        mQueryTimerPtr->stop();
        mQueryTimerPtr->deleteLater();
    }

    if(mSerialPortPtr && mSerialPortPtr->isOpen())
    {
        mSerialPortPtr->clear();
        mSerialPortPtr->close();
    }

    mQueryTimerPtr = 0;
    mQueryMode = false;
}

ComDataWorker::~ComDataWorker()
{
    qDebug()<<__FILE__<<__FUNCTION__<<__LINE__;
    if(mSerialPortPtr) mSerialPortPtr->deleteLater();
    qDebug()<<__FILE__<<__FUNCTION__<<__LINE__;
}

void ComDataWorker::slotRecvSerialPortErr(QSerialPort::SerialPortError err)
{
    qDebug()<<__FILE__<<__FUNCTION__<<err;
    QString errmsg;
    switch (err) {
    case QSerialPort::DeviceNotFoundError:
        errmsg = QString("'%1' device not found").arg(mParam.mName);
        break;
    case QSerialPort::PermissionError:
        errmsg = QString("'%1' permission error").arg(mParam.mName);
        break;
    case QSerialPort::OpenError:
        errmsg = QString("'%1' opened error").arg(mParam.mName);
        break;
    case QSerialPort::ParityError:
        errmsg = QString("'%1' parity error").arg(mParam.mName);
        break;
    case QSerialPort::FramingError:
        errmsg = QString("'%1' framing error").arg(mParam.mName);
        break;
    case QSerialPort::BreakConditionError:
        errmsg = QString("'%1' break condition error").arg(mParam.mName);
        break;
    case QSerialPort::WriteError:
        errmsg = QString("'%1' write error").arg(mParam.mName);
        break;
    case QSerialPort::ReadError:
        errmsg = QString("'%1' read error").arg(mParam.mName);
        break;
    case QSerialPort::ResourceError:
        errmsg = QString("'%1' resource error").arg(mParam.mName);
        break;
    case QSerialPort::UnsupportedOperationError:
        errmsg = QString("'%1' unsupport operation error").arg(mParam.mName);
        break;
    case QSerialPort::UnknownError:
        errmsg = QString("'%1' unknown error").arg(mParam.mName);
        break;
    case QSerialPort::TimeoutError:
        errmsg = QString("'%1' timeout error").arg(mParam.mName);
        break;
    case QSerialPort::NotOpenError:
        errmsg = QString("'%1' not opened error").arg(mParam.mName);
        break;
    default:
        break;
    }

    if(errmsg.length())
    {
        emit signalSerialPortErrorStr(errmsg);
    }



}

void ComDataWorker::slotReadComData()
{
    //cout<<"串口已经接收到数据--------------------------";
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    emit signalReciveComData(mSerialPortPtr->portName(), mParam.mTopic, time, mSerialPortPtr->readAll());

}

void ComDataWorker::writeData(const QByteArray &bytes)
{
    if(!mSerialPortPtr) return;
    //qDebug()<<"write query command:"<<bytes.toHex().toUpper();
    mSerialPortPtr->write(bytes);
    mSerialPortPtr->waitForBytesWritten(3000);
}

void ComDataWorker::slotRecvWriteInfo(qint64 num)
{
    QString info = QString::fromStdString(mParam.mQueryCmd.toHex().toUpper().data());
    QString msg = tr("%1---写入信息(%2)--长度:%3").arg(mParam.mName).arg(info).arg(num);
    QSerialPort * port = qobject_cast<QSerialPort*> (sender());
    if(!port) return;
    emit signalReciveWriteMsg(port->portName(), QDateTime::currentMSecsSinceEpoch(), msg.toUtf8());
}

void ComDataWorker::setQueryMode(bool sts)
{
    mQueryMode = sts;
    if(mQueryMode)
    {
        //开启定时发送数据信息
        mQueryTimerPtr = new QTimer(0);
        mQueryTimerPtr->setInterval(3000);
        connect(mQueryTimerPtr, SIGNAL(timeout()), this, SLOT(slotSendQueryCmd()));
        mQueryTimerPtr->start();
    }
}

void ComDataWorker::slotSendQueryCmd()
{
    if(mQueryMode && mParam.mQueryCmd.length() > 0)
    {
        writeData(mParam.mQueryCmd);
    }
}
