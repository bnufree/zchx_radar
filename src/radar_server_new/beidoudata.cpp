#include "beidoudata.h"
#include "ui_beidoudata.h"
//#include <QDebug>
#include <QWidget>
#include "profiles.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

beidouData::beidouData(int id,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::beidouData),
    m_pTcpSocket(NULL)
{
    ui->setupUi(this);
    this->setWindowTitle("北斗数据");
    str_bd = QString("beidou_") + QString::number(id);
    setWindowFlags(Qt::WindowStaysOnTopHint);
    ui->user_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"user").toString());
    ui->search_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"search").toString());
    ui->type_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"type").toString());
    ui->key_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"key").toString());
    ui->jd_lineEdit->setText( Utils::Profiles::instance()->value(str_bd,"jd").toString());
    ui->urgency_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"urgency").toString());
    ui->more_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"more").toString());
    ui->heighttype_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"heighttype").toString());
    ui->length_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"length").toString());
    ui->lon_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"lon").toString());
    ui->lat_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"lat").toString());
    ui->height_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"height").toString());
    ui->hour_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"hour").toString());
    ui->min_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"min").toString());
    ui->sec_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"sec").toString());
    ui->mes_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"mes").toString());
    //接入
    ui->ip_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"Ip").toString());
    ui->port_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"Port").toString());
    ui->reconnect_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"Rec").toString());
    //输出
    ui->output_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"Output").toString());
    ui->flag_lineEdit->setText(Utils::Profiles::instance()->value(str_bd,"Flag").toString());
    m_sIP = Utils::Profiles::instance()->value(str_bd,"Ip").toString();
    m_uPort = Utils::Profiles::instance()->value(str_bd,"Port").toInt();
    m_uBDSendPort = Utils::Profiles::instance()->value(str_bd,"Output").toInt();
    mDataTimeOut = Utils::Profiles::instance()->value(str_bd, "Rec").toInt();
    m_sBDTopic = Utils::Profiles::instance()->value(str_bd,"Flag").toString();

    mAisHeartTimer = new QTimer();
    mAisHeartTimer->setInterval(60*1000);
    connect(mAisHeartTimer, SIGNAL(timeout()), this, SLOT(slotCheckAisRecv()));

    connect(this,SIGNAL(starProcess()),this,SLOT(init()));
    moveToThread(&mWorkThread);
    mWorkThread.start();

}

beidouData::~beidouData()
{
    delete ui;
    delete m_pTcpSocket;
    m_pTcpSocket = 0;
}

void beidouData::aisToBeidou(double lon, double lat)
{
    beidouArray.clear();
    //数据头 $DWXX 5字节
    beidouArray += "$DWXX";
    //长度 16 2字节
    int len;
    int h = ui->height_lineEdit->text().toInt();
    if(h == 1)
        len = 25+5;
    else
        len = 26+5;
    char e = '0';
    beidouArray += e;
    char l = len;
    beidouArray += l;
    //用户地址 24 3字节
    char ad = ui->user_lineEdit->text().toInt();
    beidouArray += ad;
    beidouArray += ad;
    beidouArray += ad;
    //信息类别 8 1字节
    QString s = "00";
    QString line;
    //-类别
    line = ui->type_lineEdit->text();
    s += line;
    //-密钥
    line = ui->key_lineEdit->text();
    s += line;
    //-精度
    line = ui->jd_lineEdit->text();
    s += line;
    //-紧急定位
    line = ui->urgency_lineEdit->text();
    s += line;
    //-多值解
    line = ui->more_lineEdit->text();
    s += line;
    //-高程类型
    line = ui->heighttype_lineEdit->text();
    s += line;
//    cout<<"s"<<s;
    int asc = er2shi(s);
    char c = asc;
    beidouArray += c;
    //查询地址 24 3字节
    ad = ui->search_lineEdit->text().toInt();
    beidouArray += ad;
    beidouArray += ad;
    beidouArray += ad;
    //定位时刻 32 4字节
    int hour = ui->hour_lineEdit->text().toInt();
    int min = ui->min_lineEdit->text().toInt();
    int sec = ui->sec_lineEdit->text().toInt();
    int mes = ui->mes_lineEdit->text().toInt();
    char t = hour;
    beidouArray += t;
    t = min;
    beidouArray += t;
    t = sec;
    beidouArray += t;
    t = mes;
    beidouArray += t;
    //大地经度 32 4字节
    praseLonLat(lon);
    //大地纬度32 4字节
    praseLonLat(lat);
    //大地高程数据 3或者4字节   //当“高程类型”为“1”时，H 参数变为 24bit 无符号数，ζH 参数自动取消。
    ad = ui->height_lineEdit->text().toInt();
    if(h == 1)
    {
        beidouArray += ad;
        beidouArray += ad;
        beidouArray += ad;
    }
    else
    {
        beidouArray += ad;
        beidouArray += ad;
        beidouArray += ad;
        beidouArray += ad;
    }
    //校验码
    char xym = checkxor(beidouArray);
//    cout<<"size"<<beidouArray.size()<<xym<<crc;
    beidouArray += xym;
//    cout<<"CheckXor"<<crc<<CheckXor(beidouArray);
//    cout<<"cahr"<<beidouArray;
//    cout<<"size"<<beidouArray.size();
    //QString str(beidouArray);
    ui->beidou_lineEdit->setText(beidouArray);
    if(1)//prt
    {
        QDir dir;
        dir.cd("../");  //进入某文件夹
        if(!dir.exists("北斗数据"))//判断需要创建的文件夹是否存在
        {
            dir.mkdir("北斗数据"); //创建文件夹
        }
        QString file_name ="../北斗数据/北斗数据_" + QString::number(1) + ".txt";
        QFile file(file_name);//创建文件对象
        bool isOk = file.open(QIODevice::Text |QIODevice::WriteOnly |QIODevice::Append);

        if(true == isOk)
        {
            file.write(beidouArray);
            file.write("\n");
        }
        file.close();
    }
}

void beidouData::init()
{
//    cout<<"init()";
    if(m_pTcpSocket)
    {
        m_pTcpSocket->abort();
        delete m_pTcpSocket;
        m_pTcpSocket = 0;
    }
    m_pTcpSocket = new QTcpSocket();
    connect(m_pTcpSocket,SIGNAL(readyRead()),this,SLOT(updateServerProgress()));
    m_pTcpSocket->connectToHost(m_sIP,m_uPort);

    mLastRecvBdDataTime = QDateTime::currentMSecsSinceEpoch();
    initZmq();
}

void beidouData::updateServerProgress()
{
    if(m_pTcpSocket == NULL)
    {
        return;
    }
    QByteArray aisArray = m_pTcpSocket->readAll();
    dealBdData(aisArray);
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive ais data,size = %1").arg(aisArray.size());
    //emit signalSendRecvedContent(utc,"AIS_RECEIVE",sContent);
    mLastRecvBdDataTime = QDateTime::currentMSecsSinceEpoch();
}

void beidouData::dealBdData(QByteArray bdArray)
{
//    cout<<"开始解析数据";
//    cout<<"aisArray"<<bdArray.size()<<bdArray;
    //cout<<"拼接数据"<<mAisData;
    static int num = 0;
    const unsigned char* data = (const unsigned char*)(bdArray.constData());
    char sHeader = '$';
    qint64 uBeginPos = 0;
    qint64 uEndPos = 0;
    qint64 Index = 1;
    QByteArray sBdCell;
    ITF_BDList objBdList;
    while (true) {
        uBeginPos = bdArray.indexOf(sHeader,uEndPos);

        char le = data[uBeginPos+6];
        int len = le;
        uEndPos = bdArray.indexOf(sHeader,uBeginPos+1);
//        cout<<"uBeginPos"<<uBeginPos<<"uEndPos"<<uEndPos<<"len"<<len;

        if(uBeginPos<0) {
            //cout<<"后面没有包头了";
            break;
        }
        num++;
        sBdCell = bdArray.mid(uBeginPos,uEndPos-uBeginPos);//找出单条ais数据
//        cout<<"sBdCell"<<sBdCell<<sBdCell.size();
        /*int i = 0;
        while(i<30)
        {
            //9
            uchar checkx = sBdCell[i];
            cout<<"sBdCell["<<i<<"]"<<checkx;
            i++;
        }*/
        ITF_BD BD;
        int mHour = sBdCell[14];
        int mMin = sBdCell[15];
        int mSec = sBdCell[16];
        int mMes = sBdCell[17];
//        cout<<"mHour"<<mHour<<"mMin"<<mMin<<"mSec"<<mSec<<mMes;
        BD.set_hour(QTime::currentTime().hour());
        BD.set_minute(QTime::currentTime().minute());
        BD.set_second(QTime::currentTime().second());
        BD.set_millisecond(QTime::currentTime().msec());
        double deg = sBdCell[18];
        double min = sBdCell[19];
        double sed = sBdCell[20];
        double mes = sBdCell[21];
        double lon = deg+(min/60)+(sed/60/60)+(mes/10/60/60);
        BD.set_lon(lon);
        deg = sBdCell[22];
        min = sBdCell[23];
        sed = sBdCell[24];
        mes = sBdCell[25];
        double lat = deg+(min/60)+(sed/60/60)+(mes/10/60/60);
        BD.set_lat(lat);
        //编号
        if(num < 10)
        {
            QString idStr = "B000000"+QString::number(num);
            BD.set_id(idStr.toStdString());
//            cout<<"idStr"<<idStr;
        }
        else if(num < 100)
        {
            QString idStr = "B00000"+QString::number(num);
            BD.set_id(idStr.toStdString());
//            cout<<"idStr"<<idStr;
        }
        else if(num < 1000)
        {
            QString idStr = "B0000"+QString::number(num);
            BD.set_id(idStr.toStdString());
//            cout<<"idStr"<<idStr;
        }
        else if(num < 10000)
        {
            QString idStr = "B000"+QString::number(num);
            BD.set_id(idStr.toStdString());
//            cout<<"idStr"<<idStr;
        }
        else if(num < 100000)
        {
            QString idStr = "B00"+QString::number(num);
            BD.set_id(idStr.toStdString());
//            cout<<"idStr"<<idStr;
        }
        else if(num < 1000000)
        {
            QString idStr = "B0"+QString::number(num);
            BD.set_id(idStr.toStdString());
//            cout<<"idStr"<<idStr;
        }
        else if(num == 1000000)
        {
            num = 1;
        }
//        cout<<"lon"<<QString::number(lon, 'f',10) <<"lat"<<QString::number(lat, 'f',10);
        ITF_BD * temp = objBdList.add_bd();
        temp->CopyFrom(BD);
   }
   if(objBdList.bd_size() > 0)
   {
       qint64 utc = QDateTime::currentMSecsSinceEpoch();
       objBdList.set_utc(utc);
       QByteArray sendData;
       sendData.resize(objBdList.ByteSize());
       objBdList.SerializePartialToArray(sendData.data(),sendData.size());

       QString sTopic = m_sBDTopic;
       QByteArray sTopicArray = sTopic.toUtf8();
       QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
       zmq_send(m_pAISLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
       zmq_send(m_pAISLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
       zmq_send(m_pAISLisher, sendData.data(), sendData.size(), 0);
   }
}

void beidouData::slotCheckAisRecv()
{
    //1分钟未接收到数据就重连
    if(QDateTime::currentMSecsSinceEpoch() - mLastRecvBdDataTime > mDataTimeOut *60 *1000 )
    {
        //emit signalSocketMsg(QString("ZCHXAisDataServer: receive data timeout. reconnect now."));
        init();
    }
}

//经纬度转换
void beidouData::praseLonLat(double f)
{
    int deg = (int)f;
    int min = (int)((f-deg)*60);
    int sed = (int)(((f - deg)*60 - min ) * 60);
    int mes = (int)(((((f - deg)*60 - min ) * 60)-sed)*10);
//    cout<<"f"<<f<<deg<<min<<sed<<mes<<QString(deg)+"度"+QString(min)+"分"+QString(sed)+"秒"+QString(mes)+"0.1秒";
    char cdeg = deg;
    char cmin = min;
    char csed = sed;
    char cmes = mes;
    QByteArray mArray;
    mArray += cdeg;
    mArray += cmin;
    mArray += csed;
    mArray += cmes;
    //cout<<"cahr"<<mArray<<mArray.toHex()<<mArray.size();
    beidouArray += mArray;
}

//二进制转十进制
int beidouData::er2shi(QString s)
{
    int num = 0;
    for(int i = 7; i > -1; i--)
    {
        if(s[i] == '1')
        {
//            cout<<"i"<<i;
            switch(i)
            {
                case 7:
                    num += 1;
                    break;
                case 6:
                    num += 2;
                    break;
                case 5:
                    num += 4;
                    break;
                case 4:
                    num += 8;
                    break;
                case 3:
                    num += 16;
                    break;
                case 2:
                    num += 32;
                    break;
                case 1:
                    num += 64;
                    break;
                case 0:
                    num += 128;
                    break;
            }
//            cout<<"num"<<num;
        }
    }
    return num;
}

//保存按钮
void beidouData::on_pushButton_clicked()
{
    beidouArray.clear();
    //数据头 $DWXX 5字节
    beidouArray += "$DWXX";
    //长度 16 2字节
    int len;
    int h = ui->heighttype_lineEdit->text().toInt();
    if(h == 1)
        len = 25+5;
    else
        len = 26+5;
    char e = '0';
    beidouArray += e;
    char l = len;
    beidouArray += l;
    //用户地址 24 3字节
    char ad = ui->user_lineEdit->text().toInt();
    beidouArray += ad;
    beidouArray += ad;
    beidouArray += ad;
    //信息类别 8 1字节
    QString s = "00";
    QString line;
    //-类别
    line = ui->type_lineEdit->text();
    s += line;
    //-密钥
    line = ui->key_lineEdit->text();
    s += line;
    //-精度
    line = ui->jd_lineEdit->text();
    s += line;
    //-紧急定位
    line = ui->urgency_lineEdit->text();
    s += line;
    //-多值解
    line = ui->more_lineEdit->text();
    s += line;
    //-高程类型
    line = ui->heighttype_lineEdit->text();
    s += line;
//    cout<<"s"<<s;
    int asc = er2shi(s);
    char c = asc;
    beidouArray += c;
    //查询地址 24 3字节
    ad = ui->search_lineEdit->text().toInt();
    beidouArray += ad;
    beidouArray += ad;
    beidouArray += ad;
    //定位时刻 32 4字节
    int hour = ui->hour_lineEdit->text().toInt();
    int min = ui->min_lineEdit->text().toInt();
    int sec = ui->sec_lineEdit->text().toInt();
    int mes = ui->mes_lineEdit->text().toInt();
    char t = hour;
    beidouArray += t;
    t = min;
    beidouArray += t;
    t = sec;
    beidouArray += t;
    t = mes;
    beidouArray += t;
    //大地经度 32 4字节
    QString s1 = ui->lon_lineEdit->text();
    praseLonLat(s1.toDouble());
    //大地纬度32 4字节
    QString s2 = ui->lat_lineEdit->text();
    praseLonLat(s2.toDouble());
    //大地高程数据 3或者4字节   //当“高程类型”为“1”时，H 参数变为 24bit 无符号数，ζH 参数自动取消。
    ad = ui->height_lineEdit->text().toInt();
    if(h == 1)
    {
//        cout<<"h"<<h;
        beidouArray += ad;
        beidouArray += ad;
        beidouArray += ad;
    }
    else
    {
//        cout<<"h"<<h;
        beidouArray += ad;
        beidouArray += ad;
        beidouArray += ad;
        beidouArray += ad;
    }
    //校验码
    char xym = checkxor(beidouArray);
//    cout<<"size"<<beidouArray.size()<<xym<<crc;
    beidouArray += xym;
//    cout<<"CheckXor"<<crc<<CheckXor(beidouArray);
//    cout<<"char"<<beidouArray;
//    cout<<"size"<<beidouArray.size();
    //QString str(beidouArray);
    ui->beidou_lineEdit->setText(beidouArray.toHex());
    if(1)//prt
    {
        QDir dir;
        dir.cd("../");  //进入某文件夹
        if(!dir.exists("北斗数据"))//判断需要创建的文件夹是否存在
        {
            dir.mkdir("北斗数据"); //创建文件夹
        }
        QString file_name ="../北斗数据/北斗数据_" + QString::number(1) + ".txt";
        QFile file(file_name);//创建文件对象
        bool isOk = file.open(QIODevice::Text |QIODevice::WriteOnly |QIODevice::Append);

        if(true == isOk)
        {
            file.write(beidouArray);
            file.write("\n");
        }
        file.close();
    }
    Utils::Profiles::instance()->setValue(str_bd,"user",ui->user_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"search",ui->search_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"type",ui->type_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"key",ui->key_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"jd",ui->jd_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"urgency",ui->urgency_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"more",ui->more_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"heighttype",ui->heighttype_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"length",ui->length_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"lon",ui->lon_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"lat",ui->lat_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"height",ui->height_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"hour",ui->hour_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"min",ui->min_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"sec",ui->sec_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"mes",ui->mes_lineEdit->text());
    //接入
    Utils::Profiles::instance()->setValue(str_bd,"Ip",ui->ip_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"Port",ui->port_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"Rec",ui->reconnect_lineEdit->text());
    //输出
    Utils::Profiles::instance()->setValue(str_bd,"Output",ui->output_lineEdit->text());
    Utils::Profiles::instance()->setValue(str_bd,"Flag",ui->flag_lineEdit->text());

    dealBdData(beidouArray);

}

//校验码
bool beidouData::CheckXor(QByteArray data)
{
    //cout<<"data"<<data;
    QByteArray Array = data.mid(0, data.size());
    Array = Array.trimmed();
    //cout<<"Array"<<Array;
    QByteArray checkArray = Array.mid(0,Array.size() - 1);
    //cout<<"checkArray"<<checkArray;
    uchar checkx = checkxor(checkArray);
//    cout<<"checkx"<<checkx;

    QByteArray h;
    h.setNum(checkx,16);
    //cout<<"h.toUpper()"<<h.toUpper();
    QByteArray x = Array.mid(data.size()-1,1);
    crc = h.toUpper();
//    cout<<"h:"<<h.toUpper()<<"x:"<<x<<"crc"<<crc;
    return (h.toUpper() == x.toUpper())?true:false;
}

uchar beidouData::checkxor(QByteArray data)
{
    uchar checkSum = 0;
    for(int i=0;i<data.size();i++) {
        QChar byte = data.at(i);
        uchar cel = byte.cell();
        checkSum ^= cel;
    }
    return checkSum;
}

void beidouData::initZmq()
{
    m_pAISContext = zmq_ctx_new();
    m_pAISLisher= zmq_socket(m_pAISContext, ZMQ_PUB);

    //监听zmq
    QString monitorAisUrl = "inproc://monitor.bdclient";
    zmq_socket_monitor (m_pAISLisher, monitorAisUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pMonitorThread = new ZmqMonitorThread(m_pAISContext, monitorAisUrl, 0);
    connect(m_pMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pMonitorThread, SIGNAL(finished()), m_pMonitorThread, SLOT(deleteLater()));
    m_pMonitorThread->start();

    QString sIPport = QString("tcp://*:%1").arg(m_uBDSendPort);
    zmq_bind(m_pAISLisher, sIPport.toLatin1().data());

}
