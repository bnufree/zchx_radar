#include "aisbaseinfosetting.h"
#include "ui_aisbaseinfosetting.h"
#include <QTranslator>
#include <QMessageBox>
#include <QEvent>
#include <QColorDialog>
#include "common.h"
#include <QDebug>
#include "profiles.h"

#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

aisBaseInfoSetting::aisBaseInfoSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::aisBaseInfoSetting)
{
    ui->setupUi(this);
    setWindowTitle("助航信息设置");
    initAllTypeInfo();//初始化配置

    this->moveToThread(&mWorkThread);
    mWorkThread.start();
    connect(this,SIGNAL(startProcess()),this,SLOT(workSlot()));
}

aisBaseInfoSetting::~aisBaseInfoSetting()
{
    delete ui;
}

//保存
void aisBaseInfoSetting::on_modifyCommunicationSetup_clicked()
{
    int mNum = ui->tabWidget->currentIndex();
    cout<<"mNum"<<mNum;
    //保存各个页面数据
    saveAllTtpeInfo(mNum);
    //串口部分
    QString port = ui->PortBox->currentText();
    int Baud = ui->BaudBox->currentText().toInt();
    int BitNum = ui->BitNumBox->currentText().toInt();
    QString Parity = ui->ParityBox->currentText();
    int Stop = ui->StopBox->currentText().toInt();
    bool Send = ui->SendCheck->isChecked();
    Utils::Profiles::instance()->setValue("21_AIS","port", port );
    Utils::Profiles::instance()->setValue("21_AIS","Baud", Baud );
    Utils::Profiles::instance()->setValue("21_AIS","BitNum", BitNum );
    Utils::Profiles::instance()->setValue("21_AIS","Parity", Parity );
    Utils::Profiles::instance()->setValue("21_AIS","Stop", Stop );
    Utils::Profiles::instance()->setValue("21_AIS","Send", Send );

    int index = ui->type_comboBox->currentIndex();
    initComboBox(index);
    switch (index) {
    case 0:
        create3Str();
        break;
    case 1:
        create4Str();
        break;
    case 2:
        create5Str();
        break;
    case 3:
        create21Str();
        break;
    }
    openPort();//更新串口
    ui->lineEdit->setText(str.data());//展示生成的字符串
    flag = Send;
    if(mTimer && flag)
    {
        mTimer->stop();
        mTimer->start(mSendfrequency*1000);
    }
    else  mTimer->stop();
}

void aisBaseInfoSetting::workSlot()
{
    int index = ui->type_comboBox->currentIndex();
    initComboBox(index);
    switch (4) {//index
    case 0:
        create3Str();
        break;
    case 1:
        create4Str();
        break;
    case 2:
        create5Str();
        break;
    case 3:
        create21Str();
        break;
    }

    //-----------------------------------发送到串口部分-----------------------------------
    int Sendfrequency = ui->send_lineEdit->text().toInt();
    bool Send = ui->SendCheck->isChecked();
    flag = Send;
    mTimer = new QTimer();
    connect(mTimer, SIGNAL(timeout()), this, SLOT(sendData()));
    if(mTimer && flag)
    mTimer->start(mSendfrequency*1000);

    serial = new QSerialPort;
    openPort();

}

//发送数据
void aisBaseInfoSetting::sendData()
{

    if(flag)
    {
        cout<<"写数据到串口" ;
        serial->write(str.data());
    }
}

//生成第3类AIS字符串函数
void aisBaseInfoSetting::create3Str()
{
    mSendfrequency = ui->Sendfrequency3->text().toInt();
    //cout<<"消息为3的助航信息";
    int id= ui->id3->text().toInt();
    int repeatindicator= ui->repeatindicator3->text().toInt();
    int mmsi= ui->mmsi3->text().toInt();
    int navstatus = ui->navstatus3->text().toInt();//状态
    int ROTAIS = ui->ROTAIS3->text().toInt();//转向率
    float SOG = ui->SOG3->text().toFloat();//速度
    int positionAccuracy= ui->positionAccuracy3->text().toInt();//位置精准度
    double lon= ui->lon3->text().toDouble();
    double lat= ui->lat3->text().toDouble();
    float cog = ui->COG3->text().toFloat();
    float heading = ui->heading3->text().toFloat();
    int timestamp = ui->timestamp3->text().toInt();
    int specialIndicator = ui->specialIndicator3->text().toInt();
    int standby = ui->standby3->text().toInt();
    int RAIM = ui->RAIM3->text().toInt();
    int CommunicateStatus = ui->CommunicateStatus3->text().toInt();
    string channel = ui->channel3->text().toStdString();

    bitset<168> backward_bs;
    {
        //消息ID 6
        u_long temp = id;
        bitset<8> backward_id(temp);
        backward_bits(backward_bs, 0, 6, backward_id);
        //转发指示符 2
        temp = repeatindicator;
        bitset<8> backward_indicator(temp);
        backward_bits(backward_bs, 6, 2, backward_indicator);
        //MMSI编号 30
        temp = mmsi;
        bitset<32> backward_mmsi(temp);
        backward_bits(backward_bs, 8, 30, backward_mmsi);
        //导航状态 4
        temp = navstatus;
        bitset<8> backward_atontype(temp);
        backward_bits(backward_bs, 38, 4, backward_atontype);
        //旋转速率 8
        temp = ROTAIS;
        bitset<8> backward_ROTAIS(temp);
        backward_bits(backward_bs, 42,  8, backward_ROTAIS);
        //SOG 10
        temp = SOG * 10;
        bitset<32> backward_SOG(temp);
        backward_bits(backward_bs, 50, 10, backward_SOG);
        //位置准确度 1
        temp = positionAccuracy;
        bitset<8> backward_positionAccuracy(temp);
        backward_bits(backward_bs, 60, 1, backward_positionAccuracy);
        //经度 28
        float x_temp = lon * 600000.;
        bitset<32> lon(x_temp);
        backward_bits(backward_bs, 61, 28, lon);
        //纬度 27
        float y_temp = lat * 600000.;
        bitset<32> lat(y_temp);
        backward_bits(backward_bs, 89, 27, lat);
        //COG 12
        temp = cog * 10;
        bitset<32> backward_cog(temp);
        backward_bits(backward_bs, 116, 12, backward_cog);
        //实际航向 9
        temp = heading;
        bitset<32> backward_heading(temp);
        backward_bits(backward_bs, 128, 9, backward_heading);
        //时戳 6
        temp = timestamp;
        bitset<8> mtimestamp(temp);
        backward_bits(backward_bs, 137, 6, mtimestamp);
        //特定指示符 2
        temp = specialIndicator;
        bitset<8> backward_specialIndicator(temp);
        backward_bits(backward_bs, 143, 2, backward_specialIndicator);
        //备用 3
        temp = standby;
        bitset<8> backward_standby(temp);
        backward_bits(backward_bs, 145, 3, backward_standby);
        //RAIM标志 1
        temp = RAIM;
        bitset<8> backward_RAIM(temp);
        backward_bits(backward_bs, 148, 1, backward_RAIM);
        //通信状态 19
        temp = CommunicateStatus;
        bitset<32> backward_CommunicateStatus(temp);
        backward_bits(backward_bs, 149, 19, backward_CommunicateStatus);
        //std::cout<<backward_id<<"-逆向转换-"<<backward_bs<<std::endl;
    }
    string backward_str;//最终转换成的字符串
    for (size_t idx = 0; idx < 168/6; idx++) {
      bitset<6>a;
      int c = ubits(backward_bs, 6*idx, 6);
      //qDebug()<<"cccccc"<<c;
      char str;
      if(c < 40)
      {
           str = c + '0';
      }
      if(c > 39)
      {
           str = c + '0' + 8;
      }
      backward_str += str;
    }

    str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*5B";
    //生成校验码
    QByteArray CRC;
    CRC.append(str.data());
    if(!CheckXor(CRC)) {
        cout<<"校验位检查有问题";
        cout<<"校验位不对数据:"<<str.data();
        //continue;
    }
    CRC.clear();
    //cout<<"crc"<<crc<<crc.size();
    if(crc.size() > 1)
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*" + crc.toStdString();
    }
    else
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*0"+ crc.toStdString();
    }

    qDebug()<<"最终转换成3类的字符串"<<str.data();
}

//生成第4类AIS字符串函数
void aisBaseInfoSetting::create4Str()
{
     mSendfrequency = ui->Sendfrequency4->text().toInt();
    //cout<<"消息为4的助航信息";
    int id=ui->id4->text().toInt();
    int repeatindicator=ui->repeatindicator4->text().toInt();
    int mmsi=ui->mmsi4->text().toInt();
    int year = ui->year->text().toInt();
    int mouth = ui->mouth->text().toInt();
    int day = ui->day->text().toInt();
    int hour = ui->hour->text().toInt();
    int minute = ui->minute->text().toInt();
    int second = ui->second->text().toInt();
    int positionAccuracy=ui->positionAccuracy4->text().toInt();
    double lon= ui->lon4->text().toDouble();
    double lat= ui->lat4->text().toDouble();
    int fix_type = ui->fix_type4->text().toInt();
    int stnadby = ui->control4->text().toInt();
    int control = ui->control4->text().toInt();
    int raim =ui->raim4->text().toInt();
    int CommunicateStatus =ui->CommunicateStatus4->text().toInt();
    string channel = ui->channel4->text().toStdString();

    bitset<168> backward_bs;
    {
        //消息ID 6
        u_long temp = id;
        bitset<8> backward_id(temp);
        backward_bits(backward_bs, 0, 6, backward_id);
        //转发指示符 2
        temp = repeatindicator;
        bitset<8> backward_indicator(temp);
        backward_bits(backward_bs, 6, 2, backward_indicator);
        //MMSI编号 30
        temp = mmsi;
        bitset<32> backward_mmsi(temp);
        backward_bits(backward_bs, 8, 30, backward_mmsi);
        //年 14
        temp = year;
        bitset<32> backward_year(temp);
        backward_bits(backward_bs, 38, 14, backward_year);
        //月 4
        temp = mouth;
        bitset<8> backward_mouth(temp);
        backward_bits(backward_bs, 52, 4, backward_mouth);
        //日 5
        temp = day;
        bitset<8> backward_day(temp);
        backward_bits(backward_bs, 56, 5, backward_day);
        //时 5
        temp = hour;
        bitset<8> backward_hour(temp);
        backward_bits(backward_bs, 61, 5, backward_hour);
        //分 6
        temp = minute;
        bitset<8> backward_minute(temp);
        backward_bits(backward_bs, 66, 6, backward_minute);
        //秒 6
        temp = second;
        bitset<8> backward_second(temp);
        backward_bits(backward_bs, 72, 6, backward_second);
        //位置准确度 1
        temp = positionAccuracy;
        bitset<8> backward_positionAccuracy(temp);
        backward_bits(backward_bs, 78, 1, backward_positionAccuracy);
        //经度 28
        float x_temp = lon * 600000.;
        bitset<32> lon(x_temp);
        backward_bits(backward_bs, 79, 28, lon);
        //纬度 27
        float y_temp = lat * 600000.;
        bitset<32> lat(y_temp);
        backward_bits(backward_bs, 107, 27, lat);
        //电子定位装置的类型 4
        temp = fix_type;
        bitset<8> backward_fix_type(temp);
        backward_bits(backward_bs, 134, 4, backward_fix_type);
        //远距离广播消息 1
        temp = stnadby;
        bitset<8> backward_stnadby(temp);
        backward_bits(backward_bs, 138, 1, backward_stnadby);
        //备用 9
        temp = control;
        bitset<32> backward_control(temp);
        backward_bits(backward_bs, 139, 9, backward_control);
        //RAIM 标志 1
        temp = raim;
        bitset<8> backward_raim(temp);
        backward_bits(backward_bs, 148, 1, backward_raim);
        //通信状态 19
        temp = CommunicateStatus;
        bitset<32> backward_CommunicateStatus(temp);
        backward_bits(backward_bs, 149, 19, backward_CommunicateStatus);
        //std::cout<<backward_id<<"-逆向转换-"<<backward_bs<<std::endl;
    }
    string backward_str;//最终转换成的字符串
    for (size_t idx = 0; idx < 168/6; idx++) {
      bitset<6>a;
      int c = ubits(backward_bs, 6*idx, 6);
      //qDebug()<<"cccccc"<<c;
      char str;
      if(c < 40)
      {
           str = c + '0';
      }
      if(c > 39)
      {
           str = c + '0' + 8;
      }
      backward_str += str;
    }

    str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*5B";
    //生成校验码
    QByteArray CRC;
    CRC.append(str.data());
    if(!CheckXor(CRC)) {
        cout<<"校验位检查有问题";
        cout<<"校验位不对数据:"<<str.data();
        //continue;
    }
    CRC.clear();
    //cout<<"crc"<<crc<<crc.size();
    if(crc.size() > 1)
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*" + crc.toStdString();
    }
    else
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*0"+ crc.toStdString();
    }

    qDebug()<<"最终转换成4类的字符串"<<str.data();
}

//生成第5类AIS字符串函数
void aisBaseInfoSetting::create5Str()
{
    mSendfrequency = ui->Sendfrequency5->text().toInt();
    //cout<<"消息为3的助航信息";
    int id= ui->id5->text().toInt();
    int repeatindicator=ui->repeatindicator5->text().toInt();
    int mmsi=ui->mmsi5->text().toInt();
    int VersionIndicator = ui->VersionIndicator5->text().toInt();
    int imo = ui->imo5->text().toInt();
    QString callsign = ui->callsign5->text();
    QString shipname = ui->shipname5->text();
    int cargotype = ui->cargotype5->text().toInt();
    int shiplength_a = ui->shiplength_a->text().toInt();
    int shiplength_b = ui->shiplength_b->text().toInt();
    int shipwidth_c = ui->shipwidth_c5->text().toInt();
    int shipwidth_d = ui->shipwidth_d5->text().toInt();
    int fixtype = ui->fixtype5->text().toInt();
    int eta = ui->eta5->text().toInt();
    int draught = ui->draught5->text().toInt();
    QString dest =ui->dest5->text();
    int DTE = ui->DTE5->text().toInt();
    int standby = ui->standby5->text().toInt();
    string channel = ui->channel5->text().toStdString();

    bitset<424> backward_bs;
    {
        //消息ID 6
        u_long temp = id;
        bitset<8> backward_id(temp);
        backward_bits(backward_bs, 0, 6, backward_id);
        //转发指示符 2
        temp = repeatindicator;
        bitset<8> backward_indicator(temp);
        backward_bits(backward_bs, 6, 2, backward_indicator);
        //MMSI编号 30
        temp = mmsi;
        bitset<32> backward_mmsi(temp);
        backward_bits(backward_bs, 8, 30, backward_mmsi);
        //AIS版本指示符 2
        temp = VersionIndicator;
        bitset<8> backward_VersionIndicator(temp);
        backward_bits(backward_bs, 38, 2, backward_VersionIndicator);
        //IMO编号 30
        temp = imo;
        bitset<32> backward_imo(temp);
        backward_bits(backward_bs, 40, 30, backward_imo);
        //呼号 42
        QString bits_to_char_tbl = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^- !\"#$%&`()*+,-./0123456789:;<=>?";
        QString callsign_str = callsign;
        for(int i = 0; i < callsign_str.size(); i++)
        {
            int index = bits_to_char_tbl.indexOf(callsign_str.at(i));
            //qDebug()<<"字符串位置"<<i<<index;
            bitset<8> backward_index(index);
            backward_bits(backward_bs, 70 + 6*i,  6, backward_index);
        }
        //名称 120
        QString shipname_str = shipname.toLatin1().constData();
        for(int i = 0; i < shipname_str.size(); i++)
        {
            int index = bits_to_char_tbl.indexOf(shipname_str.at(i));
            qDebug()<<"字符串位置"<<i<<index;
            bitset<8> backward_index(index);
            backward_bits(backward_bs, 112 + 6*i,  6, backward_index);
        }
        //船舶和货物类型 8
        temp = cargotype;
        bitset<32> backward_cargotype(temp);
        backward_bits(backward_bs, 232, 8, backward_cargotype);
        //尺寸/位置 30
        temp = shiplength_a;
        bitset<16> tobow(temp);
        backward_bits(backward_bs, 240, 9, tobow);

        temp = shiplength_b;
        bitset<16> tostern(temp);
        backward_bits(backward_bs, 249, 9, tostern);

        temp = shipwidth_c;
        bitset<8> toport(temp);
        backward_bits(backward_bs, 258, 6, toport);

        temp = shipwidth_d;
        bitset<8> tostarboard(temp);
        backward_bits(backward_bs, 264, 6, tostarboard);
        //电子定位装置 4
        temp = fixtype;
        bitset<8> backward_fixtype(temp);
        backward_bits(backward_bs, 270, 4, backward_fixtype);
        //ETA 20
        temp = eta;
        bitset<32> backward_eta(temp);
        backward_bits(backward_bs, 274, 20, backward_eta);
        //目前最大静态吃水 8
        temp = draught;
        bitset<8> backward_draught(temp);
        backward_bits(backward_bs, 294, 8, backward_draught);
        //目的地 120
        QString dest_str = dest.toLatin1().constData();
        for(int i = 0; i < dest_str.size(); i++)
        {
            int index = bits_to_char_tbl.indexOf(dest_str.at(i));
            //qDebug()<<"字符串位置"<<i<<index;
            bitset<8> backward_index(index);
            backward_bits(backward_bs, 302 + 6*i,  6, backward_index);
        }
        //DTE 1
        temp = DTE;
        bitset<8> backward_DTE(temp);
        backward_bits(backward_bs, 422, 1, backward_DTE);
        //备用 1
        temp = standby;
        bitset<8> backward_standby(temp);
        backward_bits(backward_bs, 423, 1, backward_standby);
        //std::cout<<backward_id<<"-逆向转换-"<<backward_bs<<std::endl;
    }
    string backward_str;//最终转换成的字符串
    for (size_t idx = 0; idx < 424/6; idx++) {
      bitset<6>a;
      int c = ubits(backward_bs, 6*idx, 6);
      //qDebug()<<"cccccc"<<c;
      char str;
      if(c < 40)
      {
           str = c + '0';
      }
      if(c > 39)
      {
           str = c + '0' + 8;
      }
      backward_str += str;
    }

    str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*5B";
    //生成校验码
    QByteArray CRC;
    CRC.append(str.data());
    if(!CheckXor(CRC)) {
        cout<<"校验位检查有问题";
        cout<<"校验位不对数据:"<<str.data();
        //continue;
    }
    CRC.clear();
    //cout<<"crc"<<crc<<crc.size();
    if(crc.size() > 1)
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*" + crc.toStdString();
    }
    else
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*0"+ crc.toStdString();
    }

    qDebug()<<"最终转换成5类的字符串"<<str.data();
}

//生成第21类字符串函数
void aisBaseInfoSetting::create21Str()
{
    //cout<<"消息为21的助航信息";
    int id=ui->id_lineEdit->text().toInt();
    int repeatindicator=ui->zsf_lineEdit->text().toInt();
    int mmsi=ui->yh_lineEdit->text().toInt();

    QString name=ui->mc_lineEdit->text();
    int positionAccuracy=ui->jzd_lineEdit->text().toInt();
    double lon=ui->lon_lineEdit->text().toDouble();
    double lat=ui->lat_lineEdit->text().toDouble();
    int toBow=ui->tb_lineEdit->text().toInt();
    int toStern=ui->ts_lineEdit->text().toInt();
    int toPort=ui->tp_lineEdit->text().toInt();
    int toStarboard=ui->td_lineEdit->text().toInt();
    int fix_type=ui->fix_lineEdit->text().toInt();
    //long long UTC=obj.value("UTC").toVariant().toLongLong();
    int timestamp = ui->time_lineEdit->text().toInt();
    string channel = ui->channel_lineEdit->text().toStdString();
    mSendfrequency = ui->send_lineEdit->text().toInt();
    //std::cout<<"timestamp"<<timestamp;
    com::zhichenhaixin::proto::AidtoNavigationReport aidtoNav;

    aidtoNav.set_id(id);
    aidtoNav.set_repeatindicator(repeatindicator);
    aidtoNav.set_mmsi(mmsi);

    int aton_type = ui->lx_lineEdit->text().toInt();

    if((aton_type>=0) && (aton_type<=31)) {
        aidtoNav.set_atontype(static_cast<com::zhichenhaixin::proto::ATON_TYPE>(aton_type));
    } else {
        aidtoNav.set_atontype(static_cast<com::zhichenhaixin::proto::ATON_TYPE>(0));
    }
    aidtoNav.set_name(name.toLatin1().constData());
    aidtoNav.set_positionaccuracy(positionAccuracy);
    aidtoNav.set_lon(lon);
    aidtoNav.set_lat(lat);
    aidtoNav.set_tobow(toBow);
    aidtoNav.set_tostern(toStern);
    aidtoNav.set_toport(toPort);
    aidtoNav.set_tostarboard(toStarboard);
    switch (fix_type)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        aidtoNav.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(fix_type));
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        aidtoNav.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(8));
        break;
    default:
        aidtoNav.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
    }
    //aidtoNav.set_utc(UTC);
    bitset<360> backward_bs;
    {
        //消息ID 6
        u_long temp = aidtoNav.id();
        bitset<8> backward_id(temp);
        backward_bits(backward_bs, 0, 6, backward_id);
        //转发指示符 2
        temp = aidtoNav.repeatindicator();
        bitset<8> backward_indicator(temp);
        backward_bits(backward_bs, 6, 2, backward_indicator);
        //MMSI编号 30
        temp = aidtoNav.mmsi();
        bitset<32> backward_mmsi(temp);
        backward_bits(backward_bs, 8, 30, backward_mmsi);
        //助航设备类型 5
        temp = aidtoNav.atontype();
        bitset<8> backward_atontype(temp);
        backward_bits(backward_bs, 38, 5, backward_atontype);
        //助航设备名称 120
        QString bits_to_char_tbl = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^- !\"#$%&`()*+,-./0123456789:;<=>?";
        QString name_str = aidtoNav.name().data();
        //qDebug()<<name_str<<name_str.size()<<sizeof(name_str);
        for(int i = 0; i < name_str.size(); i++)
        {
            int index = bits_to_char_tbl.indexOf(name_str.at(i));
            //qDebug()<<"字符串位置"<<i<<index;
            bitset<8> backward_index(index);
            backward_bits(backward_bs, 43 + 6*i,  6, backward_index);
        }
        //位置精准度 1
        temp = aidtoNav.positionaccuracy();
        bitset<8> positionaccuracy(temp);
        backward_bits(backward_bs, 163, 1, positionaccuracy);
        //经度 28
        float x_temp = aidtoNav.lon() * 600000.;
        bitset<32> lon(x_temp);
        backward_bits(backward_bs, 164, 28, lon);
        //纬度 27
        float y_temp = aidtoNav.lat() * 600000.;
        bitset<32> lat(y_temp);
        backward_bits(backward_bs, 192, 27, lat);
        //尺寸/位置 30
        temp = aidtoNav.tobow();
        bitset<8> tobow(temp);
        backward_bits(backward_bs, 219, 9, tobow);

        temp = aidtoNav.tostern();
        bitset<8> tostern(temp);
        backward_bits(backward_bs, 228, 9, tostern);

        temp = aidtoNav.toport();
        bitset<8> toport(temp);
        backward_bits(backward_bs, 237, 6, toport);

        temp = aidtoNav.tostarboard();
        bitset<8> tostarboard(temp);
        backward_bits(backward_bs, 243, 6, tostarboard);
        //电子定位装置 4
        temp = aidtoNav.fixtype();
        bitset<8> fixtype(temp);
        backward_bits(backward_bs, 249, 4, fixtype);
        //时戳 6
        temp = timestamp;
        bitset<8> mtimestamp(timestamp);
        backward_bits(backward_bs, 253, 6, mtimestamp);
        //偏置位置指示符 1
        temp = 0;
        bitset<8> off_postamp(temp);
        backward_bits(backward_bs, 259, 1, off_postamp);
        //AtoN状态  8
        temp = 0;
        bitset<8> aton_statustamp(temp);
        backward_bits(backward_bs, 260, 8, aton_statustamp);
        //4个1  8
        backward_bits(backward_bs, 268, 1, off_postamp);
        backward_bits(backward_bs, 269, 1, off_postamp);
        backward_bits(backward_bs, 270, 1, off_postamp);
        backward_bits(backward_bs, 271, 1, off_postamp);
        //std::cout<<backward_id<<"-逆向转换-"<<backward_bs<<std::endl;
    }
    string backward_str;//最终转换成的字符串
    for (size_t idx = 0; idx < 272/6; idx++) {
      bitset<6>a;
      int c = ubits(backward_bs, 6*idx, 6);
      //qDebug()<<"cccccc"<<c;
      char str;
      if(c < 40)
      {
           str = c + '0';
      }
      if(c > 39)
      {
           str = c + '0' + 8;
      }
      backward_str += str;
    }


    str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*5B";

    //生成校验码
    QByteArray CRC;
    CRC.append(str.data());
    if(!CheckXor(CRC)) {
        cout<<"校验位检查有问题";
        cout<<"校验位不对数据:"<<str.data();
        //continue;
    }
    CRC.clear();
    cout<<"crc"<<crc<<crc.size();
    if(crc.size() > 1)
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*" + crc.toStdString();
    }
    else
    {
        str = "!ABVDM,1,1,2,"+channel+"," + backward_str + ",0*0"+ crc.toStdString();
    }
    qDebug()<<"最终转换成21类的字符串"<<str.data();
}

//打开串口
void aisBaseInfoSetting::openPort()
{
    if(serial)
    {
        if(serial->isOpen())//如果串口已经打开了 先给他关闭了
        {
            serial->clear();
            serial->close();
        }
        //设置串口名
        serial->setPortName(ui->PortBox->currentText());
        //打开串口
        serial->open(QIODevice::ReadWrite);
        //设置波特率
        serial->setBaudRate(ui->BaudBox->currentText().toInt());
        //设置数据位数
        switch(ui->BitNumBox->currentIndex())
        {
            case 1: serial->setDataBits(QSerialPort::Data5); break;
            case 2: serial->setDataBits(QSerialPort::Data6); break;
            case 3: serial->setDataBits(QSerialPort::Data7); break;
            case 4: serial->setDataBits(QSerialPort::Data8); break;
            default: break;
        }
            //设置奇偶校验
            switch(ui->ParityBox->currentIndex())
        {
            case 0: serial->setParity(QSerialPort::NoParity); break;
            default: break;
        }
        //设置停止位
        switch(ui->StopBox->currentIndex())
        {
            case 1: serial->setStopBits(QSerialPort::OneStop); break;
            case 2: serial->setStopBits(QSerialPort::OneAndHalfStop); break;
            case 3: serial->setStopBits(QSerialPort::TwoStop); break;
            default: break;
        }
        //设置流控制
        serial->setFlowControl(QSerialPort::NoFlowControl);
    }

}

//校验码
bool aisBaseInfoSetting::CheckXor(QByteArray data)
{
    //cout<<"data"<<data;
    QByteArray Array = data.mid(1, data.size());
    Array = Array.trimmed();
    //cout<<"Array"<<Array;
    QByteArray checkArray = Array.mid(0,Array.size() - 3);
    //cout<<"checkArray"<<checkArray;
    uchar checkx = checkxor(checkArray);
    //cout<<"checkx"<<checkx;

    QByteArray h;
    h.setNum(checkx,16);
    //cout<<"h.toUpper()"<<h.toUpper();
    QByteArray x = Array.mid(Array.size()-2,2);
    //cout<<"x"<<x;
    cout<<"h:"<<h.toUpper()<<"x:"<<x;
    crc = h.toUpper();
    return (h.toUpper() == x)?true:false;
}

uchar aisBaseInfoSetting::checkxor(QByteArray data)
{
    uchar checkSum = 0;
    for(int i=0;i<data.size();i++) {
        QChar byte = data.at(i);
        uchar cel = byte.cell();
        checkSum ^= cel;
    }
    return checkSum;
}

//选择构造消息类型
void aisBaseInfoSetting::on_type_comboBox_currentIndexChanged(int index)
{
    cout<<"全部页面"<<ui->tabWidget->count()<<"当前页面"<<index;
    switch (index) {
    case 0:
        ui->tabWidget->setCurrentIndex(1);
        break;
    case 1:
        ui->tabWidget->setCurrentIndex(2);
        break;
    case 2:
        ui->tabWidget->setCurrentIndex(3);
        break;
    case 3:
        ui->tabWidget->setCurrentIndex(0);
        break;
    }
}

//初始化下拉菜单
void aisBaseInfoSetting::initComboBox(int index)
{
    switch (index) {
    case 0:
        ui->tabWidget->setCurrentIndex(1);
        break;
    case 1:
        ui->tabWidget->setCurrentIndex(2);
        break;
    case 2:
        ui->tabWidget->setCurrentIndex(3);
        break;
    case 3:
        ui->tabWidget->setCurrentIndex(0);
        break;
    }
}

//页面和下拉栏一致变化
void aisBaseInfoSetting::on_tabWidget_currentChanged(int index)
{
    //cout<<"全部页面"<<ui->tabWidget->count()<<"当前页面"<<index;
    switch (index) {
    case 0:
        ui->type_comboBox->setCurrentIndex(3);
        break;
    case 1:
        ui->type_comboBox->setCurrentIndex(0);
        break;
    case 2:
        ui->type_comboBox->setCurrentIndex(1);
        break;
    case 3:
        ui->type_comboBox->setCurrentIndex(2);
        break;
    }
}

//保存数据到ini
void aisBaseInfoSetting::saveAllTtpeInfo(int index)
{
    switch (index) {
    case 0:
        {
            //21
            int id=ui->id_lineEdit->text().toInt();
            int repeatindicator=ui->zsf_lineEdit->text().toInt();
            int mmsi=ui->yh_lineEdit->text().toInt();
            QString name=ui->mc_lineEdit->text();
            int positionAccuracy=ui->jzd_lineEdit->text().toInt();
            double lon=ui->lon_lineEdit->text().toDouble();
            double lat=ui->lat_lineEdit->text().toDouble();
            int toBow=ui->tb_lineEdit->text().toInt();
            int toStern=ui->ts_lineEdit->text().toInt();
            int toPort=ui->tp_lineEdit->text().toInt();
            int toStarboard=ui->td_lineEdit->text().toInt();
            int fix_type=ui->fix_lineEdit->text().toInt();
            //long long UTC=obj.value("UTC").toVariant().toLongLong();
            int timestamp = ui->time_lineEdit->text().toInt();
            int aton_type = ui->lx_lineEdit->text().toInt();
            QString channel = ui->channel_lineEdit->text();
            int Sendfrequency = ui->send_lineEdit->text().toInt();
            //消息ID 6
            Utils::Profiles::instance()->setValue("21_AIS","id", id);
            //转发指示符 2
            Utils::Profiles::instance()->setValue("21_AIS","repeatindicator", repeatindicator);
            //MMSI编号 30
            Utils::Profiles::instance()->setValue("21_AIS","mmsi",  mmsi);
            //助航设备类型 5
            Utils::Profiles::instance()->setValue("21_AIS","aton_type", aton_type);
            //助航设备名称 120
            Utils::Profiles::instance()->setValue("21_AIS","name", name );
            //位置精准度 1
            Utils::Profiles::instance()->setValue("21_AIS","positionAccuracy", positionAccuracy );
            //经度 28
            Utils::Profiles::instance()->setValue("21_AIS","lon", lon );
            //纬度 27
            Utils::Profiles::instance()->setValue("21_AIS","lat", lat );
            //尺寸/位置 30
            Utils::Profiles::instance()->setValue("21_AIS","toBow",toBow  );
            Utils::Profiles::instance()->setValue("21_AIS","toStern", toStern );
            Utils::Profiles::instance()->setValue("21_AIS","toPort", toPort );
            Utils::Profiles::instance()->setValue("21_AIS","toStarboard", toStarboard );
            //电子定位装置 4
            Utils::Profiles::instance()->setValue("21_AIS","fix_type", fix_type );
            //时戳 4
            Utils::Profiles::instance()->setValue("21_AIS","timestamp", timestamp );
            //channel
            Utils::Profiles::instance()->setValue("21_AIS","channel", channel );
            //Sendfrequency
            Utils::Profiles::instance()->setValue("21_AIS","Sendfrequency", Sendfrequency );
            break;
        }
    case 1:
        {
        //3
            mSendfrequency = ui->Sendfrequency3->text().toInt();
            //cout<<"消息为3的助航信息";
            int id= ui->id3->text().toInt();
            int repeatindicator= ui->repeatindicator3->text().toInt();
            int mmsi= ui->mmsi3->text().toInt();
            int navstatus = ui->navstatus3->text().toInt();//状态
            int ROTAIS = ui->ROTAIS3->text().toInt();//转向率
            float SOG = ui->SOG3->text().toFloat();//速度
            int positionAccuracy= ui->positionAccuracy3->text().toInt();//位置精准度
            double lon= ui->lon3->text().toDouble();
            double lat= ui->lat3->text().toDouble();
            float cog = ui->COG3->text().toFloat();
            float heading = ui->heading3->text().toFloat();
            int timestamp = ui->timestamp3->text().toInt();
            int specialIndicator = ui->specialIndicator3->text().toInt();
            int standby = ui->standby3->text().toInt();
            int RAIM = ui->RAIM3->text().toInt();
            int CommunicateStatus = ui->CommunicateStatus3->text().toInt();
            QString channel = ui->channel3->text();
            //消息ID 6
            Utils::Profiles::instance()->setValue("3_AIS","id", id);
            //转发指示符 2
            Utils::Profiles::instance()->setValue("3_AIS","repeatindicator", repeatindicator);
            //MMSI编号 30
            Utils::Profiles::instance()->setValue("3_AIS","mmsi",  mmsi);
            Utils::Profiles::instance()->setValue("3_AIS","navstatus",  navstatus);
            Utils::Profiles::instance()->setValue("3_AIS","ROTAIS",  ROTAIS);
            Utils::Profiles::instance()->setValue("3_AIS","SOG",  SOG);
            Utils::Profiles::instance()->setValue("3_AIS","positionAccuracy",  positionAccuracy);
            Utils::Profiles::instance()->setValue("3_AIS","lon",  lon);
            Utils::Profiles::instance()->setValue("3_AIS","lat",  lat);
            Utils::Profiles::instance()->setValue("3_AIS","cog",  cog);
            Utils::Profiles::instance()->setValue("3_AIS","heading",  heading);
            Utils::Profiles::instance()->setValue("3_AIS","timestamp",  timestamp);
            Utils::Profiles::instance()->setValue("3_AIS","specialIndicator",  specialIndicator);
            Utils::Profiles::instance()->setValue("3_AIS","standby",  standby);
            Utils::Profiles::instance()->setValue("3_AIS","RAIM",  RAIM);
            Utils::Profiles::instance()->setValue("3_AIS","CommunicateStatus",  CommunicateStatus);
            Utils::Profiles::instance()->setValue("3_AIS","channel",  channel);
            Utils::Profiles::instance()->setValue("3_AIS","mSendfrequency",  mSendfrequency);

            break;
        }
    case 2:
        {
           //4
           mSendfrequency = ui->Sendfrequency4->text().toInt();
           //cout<<"消息为4的助航信息";
           int id=ui->id4->text().toInt();
           int repeatindicator=ui->repeatindicator4->text().toInt();
           int mmsi=ui->mmsi4->text().toInt();
           int year = ui->year->text().toInt();
           int mouth = ui->mouth->text().toInt();
           int day = ui->day->text().toInt();
           int hour = ui->hour->text().toInt();
           int minute = ui->minute->text().toInt();
           int second = ui->second->text().toInt();
           int positionAccuracy=ui->positionAccuracy4->text().toInt();
           double lon= ui->lon4->text().toDouble();
           double lat= ui->lat4->text().toDouble();
           int fix_type = ui->fix_type4->text().toInt();
           int stnadby = ui->control4->text().toInt();
           int control = ui->control4->text().toInt();
           int raim =ui->raim4->text().toInt();
           int CommunicateStatus =ui->CommunicateStatus4->text().toInt();
           QString channel = ui->channel4->text();
           //消息ID 6
           Utils::Profiles::instance()->setValue("4_AIS","id", id);
           //转发指示符 2
           Utils::Profiles::instance()->setValue("4_AIS","repeatindicator", repeatindicator);
           //MMSI编号 30
           Utils::Profiles::instance()->setValue("4_AIS","mmsi",  mmsi);
           Utils::Profiles::instance()->setValue("4_AIS","year",  year);
           Utils::Profiles::instance()->setValue("4_AIS","mouth",  mouth);
           Utils::Profiles::instance()->setValue("4_AIS","day",  day);
           Utils::Profiles::instance()->setValue("4_AIS","hour",  hour);
           Utils::Profiles::instance()->setValue("4_AIS","minute",  minute);
           Utils::Profiles::instance()->setValue("4_AIS","second",  second);
           Utils::Profiles::instance()->setValue("4_AIS","positionAccuracy",  positionAccuracy);
           Utils::Profiles::instance()->setValue("4_AIS","lon",  lon);
           Utils::Profiles::instance()->setValue("4_AIS","lat",  lat);
           Utils::Profiles::instance()->setValue("4_AIS","fix_type",  fix_type);
           Utils::Profiles::instance()->setValue("4_AIS","stnadby",  stnadby);
           Utils::Profiles::instance()->setValue("4_AIS","control",  control);
           Utils::Profiles::instance()->setValue("4_AIS","raim",  raim);
           Utils::Profiles::instance()->setValue("4_AIS","CommunicateStatus",  CommunicateStatus);
           Utils::Profiles::instance()->setValue("4_AIS","channel",  channel);
           Utils::Profiles::instance()->setValue("4_AIS","mSendfrequency",  mSendfrequency);
           break;
         }
    case 3:
         {
            //5
            mSendfrequency = ui->Sendfrequency5->text().toInt();
            //cout<<"消息为3的助航信息";
            int id= ui->id5->text().toInt();
            int repeatindicator=ui->repeatindicator5->text().toInt();
            int mmsi=ui->mmsi5->text().toInt();
            int VersionIndicator = ui->VersionIndicator5->text().toInt();
            int imo = ui->imo5->text().toInt();
            QString callsign = ui->callsign5->text();
            QString shipname = ui->shipname5->text();
            int cargotype = ui->cargotype5->text().toInt();
            int shiplength_a = ui->shiplength_a->text().toInt();
            int shiplength_b = ui->shiplength_b->text().toInt();
            int shipwidth_c = ui->shipwidth_c5->text().toInt();
            int shipwidth_d = ui->shipwidth_d5->text().toInt();
            int fixtype = ui->fixtype5->text().toInt();
            int eta = ui->eta5->text().toInt();
            int draught = ui->draught5->text().toInt();
            QString dest =ui->dest5->text();
            int DTE = ui->DTE5->text().toInt();
            int standby = ui->standby5->text().toInt();
            QString channel = ui->channel5->text();
            //消息ID 6
            Utils::Profiles::instance()->setValue("5_AIS","id", id);
            //转发指示符 2
            Utils::Profiles::instance()->setValue("5_AIS","repeatindicator", repeatindicator);
            //MMSI编号 30
            Utils::Profiles::instance()->setValue("5_AIS","mmsi",  mmsi);
            Utils::Profiles::instance()->setValue("5_AIS","VersionIndicator",  VersionIndicator);
            Utils::Profiles::instance()->setValue("5_AIS","imo",  imo);
            Utils::Profiles::instance()->setValue("5_AIS","callsign",  callsign);
            Utils::Profiles::instance()->setValue("5_AIS","shipname",  shipname);
            Utils::Profiles::instance()->setValue("5_AIS","cargotype",  cargotype);
            Utils::Profiles::instance()->setValue("5_AIS","shiplength_a",  shiplength_a);
            Utils::Profiles::instance()->setValue("5_AIS","shiplength_b",  shiplength_b);
            Utils::Profiles::instance()->setValue("5_AIS","shipwidth_c",  shipwidth_c);
            Utils::Profiles::instance()->setValue("5_AIS","shipwidth_d",  shipwidth_d);
            Utils::Profiles::instance()->setValue("5_AIS","fixtype",  fixtype);
            Utils::Profiles::instance()->setValue("5_AIS","eta",  eta);
            Utils::Profiles::instance()->setValue("5_AIS","draught",  draught);
            Utils::Profiles::instance()->setValue("5_AIS","dest",  dest);
            Utils::Profiles::instance()->setValue("5_AIS","DTE",  DTE);
            Utils::Profiles::instance()->setValue("5_AIS","standby",  standby);
            Utils::Profiles::instance()->setValue("5_AIS","channel",  channel);
            Utils::Profiles::instance()->setValue("5_AIS","mSendfrequency",  mSendfrequency);
            break;
         }
    }
}

//初始界面数据
void aisBaseInfoSetting::initAllTypeInfo()
{
    //21
    //消息ID 6
    QString id = Utils::Profiles::instance()->value("21_AIS","id").toString();
    ui->id_lineEdit->setText(id);
    //转发指示符 2
    QString repeatindicator = Utils::Profiles::instance()->value("21_AIS","repeatindicator").toString();
    ui->zsf_lineEdit->setText(repeatindicator);
    //mmsi
    QString mmsi = Utils::Profiles::instance()->value("21_AIS","mmsi").toString();
    ui->yh_lineEdit->setText(mmsi);
    //助航设备类型 5
    QString aton_type = Utils::Profiles::instance()->value("21_AIS","aton_type").toString();
    ui->lx_lineEdit->setText(aton_type);
    //助航设备名称 120
    QString name = Utils::Profiles::instance()->value("21_AIS","name").toString();
    ui->mc_lineEdit->setText(name);
    //位置精准度 1
    QString positionAccuracy = Utils::Profiles::instance()->value("21_AIS","positionAccuracy").toString();
    ui->jzd_lineEdit->setText(positionAccuracy);
    //经度 28
    QString lon = Utils::Profiles::instance()->value("21_AIS","lon").toString();
    ui->lon_lineEdit->setText(lon);
    //纬度 27
    QString lat = Utils::Profiles::instance()->value("21_AIS","lat").toString();
    ui->lat_lineEdit->setText(lat);
    //尺寸/位置 30
    QString toBow = Utils::Profiles::instance()->value("21_AIS","toBow").toString();
    QString toStern = Utils::Profiles::instance()->value("21_AIS","toStern").toString();
    QString toPort = Utils::Profiles::instance()->value("21_AIS","toPort").toString();
    QString toStarboard = Utils::Profiles::instance()->value("21_AIS","toStarboard").toString();
    ui->tb_lineEdit->setText(toBow);
    ui->ts_lineEdit->setText(toStern);
    ui->tp_lineEdit->setText(toPort);
    ui->td_lineEdit->setText(toStarboard);
    //电子定位装置 4
    QString fix_type = Utils::Profiles::instance()->value("21_AIS","fix_type").toString();
    ui->fix_lineEdit->setText(fix_type);
    //时戳 4
    QString timestamp = Utils::Profiles::instance()->value("21_AIS","timestamp").toString();
    ui->time_lineEdit->setText(timestamp);
    //channel
    QString channel = Utils::Profiles::instance()->value("21_AIS","channel").toString();
    ui->channel_lineEdit->setText(channel);
    QString port = Utils::Profiles::instance()->value("21_AIS","port").toString();
    ui->PortBox->setCurrentText(port);
    QString Baud = Utils::Profiles::instance()->value("21_AIS","Baud").toString();
    ui->BaudBox->setCurrentText(Baud);
    QString BitNum = Utils::Profiles::instance()->value("21_AIS","BitNum").toString();
    ui->BitNumBox->setCurrentText(BitNum);
    QString Parity = Utils::Profiles::instance()->value("21_AIS","Parity").toString();
    ui->ParityBox->setCurrentText(Parity);
    QString Stop = Utils::Profiles::instance()->value("21_AIS","Stop").toString();
    ui->StopBox->setCurrentText(Stop);
    bool Send = Utils::Profiles::instance()->value("21_AIS","Send").toBool();
    ui->SendCheck->setChecked(Send);
    QString Sendfrequency = Utils::Profiles::instance()->value("21_AIS","Sendfrequency").toString();
    ui->send_lineEdit->setText(Sendfrequency);
    //3
    ui->id3->setText( Utils::Profiles::instance()->value("3_AIS","id").toString());
    ui->repeatindicator3->setText(Utils::Profiles::instance()->value("3_AIS","repeatindicator").toString());
    ui->mmsi3->setText(Utils::Profiles::instance()->value("3_AIS","mmsi").toString());
    ui->navstatus3->setText( Utils::Profiles::instance()->value("3_AIS","navstatus").toString());//状态
    ui->ROTAIS3->setText(Utils::Profiles::instance()->value("3_AIS","ROTAIS").toString());//转向率
    ui->SOG3->setText(Utils::Profiles::instance()->value("3_AIS","SOG").toString());//速度
    ui->positionAccuracy3->setText(Utils::Profiles::instance()->value("3_AIS","positionAccuracy").toString());//位置精准度
    ui->lon3->setText(Utils::Profiles::instance()->value("3_AIS","lon").toString());
    ui->lat3->setText(Utils::Profiles::instance()->value("3_AIS","lat").toString());
    ui->COG3->setText(Utils::Profiles::instance()->value("3_AIS","cog").toString());
    ui->heading3->setText(Utils::Profiles::instance()->value("3_AIS","heading").toString());
    ui->timestamp3->setText(Utils::Profiles::instance()->value("3_AIS","timestamp").toString());
    ui->specialIndicator3->setText(Utils::Profiles::instance()->value("3_AIS","specialIndicator").toString());
    ui->standby3->setText(Utils::Profiles::instance()->value("3_AIS","standby").toString());
    ui->RAIM3->setText(Utils::Profiles::instance()->value("3_AIS","RAIM").toString());
    ui->CommunicateStatus3->setText(Utils::Profiles::instance()->value("3_AIS","CommunicateStatus").toString());
    ui->channel3->setText(Utils::Profiles::instance()->value("3_AIS","channel").toString());
    ui->Sendfrequency3->setText(Utils::Profiles::instance()->value("3_AIS","mSendfrequency").toString());
    //4
    ui->id4->setText( Utils::Profiles::instance()->value("4_AIS","id").toString());
    ui->repeatindicator4->setText(Utils::Profiles::instance()->value("4_AIS","repeatindicator").toString());
    ui->mmsi4->setText(Utils::Profiles::instance()->value("4_AIS","mmsi").toString());
    ui->year->setText( Utils::Profiles::instance()->value("4_AIS","year").toString());//状态
    ui->mouth->setText(Utils::Profiles::instance()->value("4_AIS","mouth").toString());//转向率
    ui->day->setText(Utils::Profiles::instance()->value("4_AIS","day").toString());//速度
    ui->hour->setText(Utils::Profiles::instance()->value("4_AIS","hour").toString());//位置精准度
    ui->minute->setText(Utils::Profiles::instance()->value("4_AIS","minute").toString());
    ui->second->setText(Utils::Profiles::instance()->value("4_AIS","second").toString());
    ui->positionAccuracy4->setText(Utils::Profiles::instance()->value("4_AIS","positionAccuracy").toString());
    ui->lon4->setText(Utils::Profiles::instance()->value("4_AIS","lon").toString());
    ui->lat4->setText(Utils::Profiles::instance()->value("4_AIS","lat").toString());
    ui->fix_type4->setText(Utils::Profiles::instance()->value("4_AIS","fix_type").toString());
    ui->control4->setText(Utils::Profiles::instance()->value("4_AIS","control").toString());
    ui->raim4->setText(Utils::Profiles::instance()->value("4_AIS","raim").toString());
    ui->CommunicateStatus4->setText(Utils::Profiles::instance()->value("4_AIS","CommunicateStatus").toString());
    ui->channel4->setText(Utils::Profiles::instance()->value("4_AIS","channel").toString());
    ui->Sendfrequency4->setText(Utils::Profiles::instance()->value("4_AIS","mSendfrequency").toString());
    //5
    ui->id5->setText( Utils::Profiles::instance()->value("5_AIS","id").toString());
    ui->repeatindicator5->setText(Utils::Profiles::instance()->value("5_AIS","repeatindicator").toString());
    ui->mmsi5->setText(Utils::Profiles::instance()->value("5_AIS","mmsi").toString());
    ui->VersionIndicator5->setText( Utils::Profiles::instance()->value("5_AIS","VersionIndicator").toString());//状态
    ui->imo5->setText(Utils::Profiles::instance()->value("5_AIS","imo").toString());//转向率
    ui->callsign5->setText(Utils::Profiles::instance()->value("5_AIS","callsign").toString());//速度
    ui->shipname5->setText(Utils::Profiles::instance()->value("5_AIS","shipname").toString());//位置精准度
    ui->cargotype5->setText(Utils::Profiles::instance()->value("5_AIS","cargotype").toString());
    ui->shiplength_a->setText(Utils::Profiles::instance()->value("5_AIS","shiplength_a").toString());
    ui->shiplength_b->setText(Utils::Profiles::instance()->value("5_AIS","shiplength_b").toString());
    ui->shipwidth_c5->setText(Utils::Profiles::instance()->value("5_AIS","shipwidth_c").toString());
    ui->shipwidth_d5->setText(Utils::Profiles::instance()->value("5_AIS","shipwidth_d").toString());
    ui->fixtype5->setText(Utils::Profiles::instance()->value("5_AIS","fixtype").toString());
    ui->eta5->setText(Utils::Profiles::instance()->value("5_AIS","eta").toString());
    ui->draught5->setText(Utils::Profiles::instance()->value("5_AIS","draught").toString());
    ui->dest5->setText(Utils::Profiles::instance()->value("5_AIS","dest").toString());
    ui->DTE5->setText(Utils::Profiles::instance()->value("5_AIS","DTE").toString());
    ui->standby5->setText(Utils::Profiles::instance()->value("5_AIS","standby").toString());
    ui->channel5->setText(Utils::Profiles::instance()->value("5_AIS","channel").toString());
    ui->Sendfrequency5->setText(Utils::Profiles::instance()->value("5_AIS","mSendfrequency").toString());
}
