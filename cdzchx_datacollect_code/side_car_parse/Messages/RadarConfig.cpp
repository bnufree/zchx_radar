///**************************************************************************
//* @File: RadarConfig.cpp
//* @Description:  雷达配置类
//* @Copyright: Copyright (c) 2017
//* @Company: 深圳置辰海信科技有限公司
//* @WebSite: http://www.szcenterstar.com/
//* @author 李鹭
//* @Revision History
//*
//* <pre>
//* ----------------------------------------------------------------------
//*   Ver     Date       Who             Comments
//*  ----- ----------  --------  ---------------------------------------
//*   1.0  2017/03/13    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/
#include <QLoggingCategory>
#include <QtCore/QString>
#include <QtXml/QDomElement>


#include "FilePath.h"
#include "zchxRadarUtils.h"

#include "RadarConfig.h"
#include "VideoConfig.h"
#include "TSPIConfig.h"
#include <QtCore>
#include <QTcpSocket>

using namespace ZCHX::Messages;

QStringList RadarConfig::gRadarNameList;

void RadarConfig::Initializer()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList paths;

    if ( env.contains( "ZCHX_RADAR_CONFIG" ) )
        paths << env.value( "ZCHX_RADAR_CONFIG" );

    if ( env.contains( "ZCHX_RADAR" ) )
        paths << env.value( "ZCHX_RADAR" ) + "/etc/configuration.xml";

    for ( QString path : paths )
    {
        auto filePath = ZchxRadarUtils::FilePath( path.toStdString() );

        if ( filePath.exists() )
        {
            SetConfigurationFilePath( path.toStdString() );
            break;
        }
    }
}

QStringList RadarConfig::getGRadarNameList()
{
    return gRadarNameList;
}

void RadarConfig::setGRadarNameList(const QStringList &value)
{
    gRadarNameList = value;
}

void RadarConfig::setLongitude( double longitude )
{
    longitude_ = longitude;
}

void RadarConfig::setLatitude( double latitude )
{
    latitude_ = latitude;
}

quint16 RadarConfig::getCmdPort() const
{
    return cmdPort_;
}

void RadarConfig::setCmdPort( const quint16 &cmdPort )
{
    cmdPort_ = cmdPort;
}

bool RadarConfig::saveRadarConfiguration()
{
    QDir dir = qApp->applicationDirPath();
    dir.cdUp();
    QString filepath;

    if ( dir.cd( "etc" ) && dir.exists( "configuration.xml" ) )
    {
        filepath = dir.filePath( "configuration.xml" );
    }

    QFile file( filepath );

    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        qCWarning( radarmsg ) << "failed to open file " << file.errorString();
        return false;
    }

    QDomDocument doc;
    QString docError;

    if ( ! doc.setContent( &file, &docError ) )
    {
        qCWarning( radarmsg ) << "failed to parse configuration file " << docError;
        return false;
    }

    QDomNodeList listradar = doc.elementsByTagName( "radar" );

    for ( int i = 0; i < listradar.size(); ++i )
    {
        QDomNode radarnode = listradar.at( i );
        QDomElement radarnamenode = radarnode.firstChildElement( "name" );
        qCDebug( radarmsg ) << "radarname: " << radarnamenode.childNodes().size() << radarnamenode.text();

        if ( radarnamenode.text() != radarName_ )
        {
            continue;
        }

        //遍历节点

        QDomNodeList list = radarnode.childNodes(); //获得元素e的所有子节点的列表

        for ( int a = 0; a < list.count(); a++ ) //遍历该列表
        {
            QDomNode node = list.at( a );

            if ( node.isElement() )
            {
                if ( node.nodeName() == "latitude" )
                {
                    QDomNode oldnode = node.firstChild();
                    node.firstChild().setNodeValue( QString::number( latitude_, 'g', 9 ) );
                    QDomNode newnode = node.firstChild();
                    node.replaceChild( newnode, oldnode );
                }

                if ( node.nodeName() == "longitude" )
                {
                    QDomNode oldnode = node.firstChild();
                    node.firstChild().setNodeValue( QString::number( longitude_, 'g', 9 ) );
                    QDomNode newnode = node.firstChild();
                    node.replaceChild( newnode, oldnode );
                }
            }
        }

        radarnode.firstChildElement( "commandhost" ).setAttribute( "host", cmdAddress_.toString() );
        radarnode.firstChildElement( "commandhost" ).setAttribute( "port", cmdPort_ );
        radarnode.firstChildElement( "video" ).setAttribute( "port", videoConfig_->GetPort() );
        radarnode.firstChildElement( "video" ).setAttribute( "host", cmdAddress_.toString() );
        radarnode.firstChildElement( "tspi" ).setAttribute( "port", tspiConfig_->GetPort() );
        radarnode.firstChildElement( "tspi" ).setAttribute( "host", cmdAddress_.toString() );
    }

    QFile filexml( filepath );

    if ( !filexml.open( QFile::WriteOnly | QFile::Truncate ) )
    {
        qWarning( "error::ParserXML->writeOperateXml->file.open\n" );
        return false;
    }

    QTextStream ts( &filexml );
    ts.reset();
    ts.setCodec( "utf-8" );
    doc.save( ts, 4, QDomNode::EncodingFromTextStream );
    filexml.close();

    emit radarInfoChanged();
}

QHostAddress RadarConfig::getCmdAddress() const
{
    return cmdAddress_;
}

void RadarConfig::changeAddress( const QString str )
{
    cmdAddress_.setAddress( str );
}

QString RadarConfig::getRadarName() const
{
    return radarName_;
}

void RadarConfig::setRadarName( const QString &radarName )
{
    radarName_ = radarName;
}

QStringList RadarConfig::getAllCommand()
{
    return mapCmd_.keys();
}

QByteArray RadarConfig::getRadarCommand( const QString &cmd )
{
    QString name = mapCmd_.value( cmd );
    QString path = dir_.filePath( name );
    QFile file( path );
    file.open( QFile::ReadOnly );
    return file.readAll();
}

void RadarConfig::sendRadarCommand( const QString &cmd )
{
    QTcpSocket cmdSocket;
    cmdSocket.connectToHost( cmdAddress_, cmdPort_ );

    if ( cmdSocket.waitForConnected( 300 ) )
    {
        int wbytes = cmdSocket.write( getRadarCommand( cmd ) );

        if ( cmdSocket.waitForBytesWritten( 100 ) )
            qInfo( radarmsg ) << "write succeed: " << cmdAddress_ << cmdPort_ << wbytes;
        else
            qCWarning( radarmsg ) << "write failed: " << cmdAddress_ << cmdPort_ << wbytes;
    }
}

bool RadarConfig::SetConfigurationFilePath( const std::string &path )
{
    QString filepath = QString::fromStdString( path );
    QFileInfo info( filepath );
    qCInfo( radarmsg ) << filepath;

    if ( ! info.exists() )
    {
        qCWarning( radarmsg ) << "no configuration file at " << filepath ;
        return false;
    }

    dir_ = info.dir();
    dir_.cd( "radarctrldata" );

    QFile file( filepath );

    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        qCWarning( radarmsg ) << "failed to open file " << file.errorString();
        return false;
    }

    QDomDocument doc;
    QString docError;

    if ( ! doc.setContent( &file, &docError ) )
    {
        qCWarning( radarmsg ) << "failed to parse configuration file " << docError;
        return false;
    }

    QDomNodeList listradar = doc.elementsByTagName( "radar" );
    QStringList radarNameList;
    bool loadOK = false;

    for ( int i = 0; i < listradar.size(); ++i )
    {
        QDomNode radarnode = listradar.at( i );
        QDomElement radarnamenode = radarnode.firstChildElement( "name" );
        QString radarName = radarnamenode.text();
        radarNameList.append(radarName);
        qCDebug( radarmsg ) << "radarname: " << radarnamenode.childNodes().size() << radarName;

        if ( radarName == radarName_ )
        {
            loadOK = Load( radarnode.toElement() );
            if ( !loadOK )
            {
                qCWarning( radarmsg ) << "invalid configuration file " << QString::fromStdString( path );
            }
        }
    }

    if(radarNameList != gRadarNameList)
    {
        RadarConfig::setGRadarNameList(radarNameList);
    }

    return loadOK;
}

RadarConfig::RadarConfig( QObject *parent )
    : QObject( parent )
    , name_( "Default" )
    , gateCountMax_( 4000 )
    , shaftEncodingMax_( 65535 )
    , rotationRate_( 6.0 )
    , rangeMin_( 1.0 )
    , rangeMax_( 300.0 )
    , rangeFactor_( CalculateRangeFactor() )
    , beamWidth_( 0.001544 )
    , latitude_( 37.0 + 49.0 / 60.0 + 7.83477 / 3600.0 )
    , longitude_( -( 116.0 + 31.0 / 60.0 + 53.51066 / 3600.0 ) )
    , height_( 0.0 )
    , videoConfig_( new VideoConfig )
    , tspiConfig_( new TSPIConfig )
{
}

RadarConfig::~RadarConfig()
{
}


const QString & RadarConfig::GetName()
{
    return name_;
}

uint32_t RadarConfig::GetGateCountMax()
{
    return gateCountMax_;
}

uint32_t RadarConfig::GetShaftEncodingMax()
{
    return shaftEncodingMax_;
}

double RadarConfig::GetRotationRate()
{
    return rotationRate_;
}

double RadarConfig::GetRangeMin_deprecated()
{
    return rangeMin_;
}

double RadarConfig::GetRangeMax()
{
    return rangeMax_;
}

double RadarConfig::GetRangeFactor_deprecated()
{
    return rangeFactor_;
}

double RadarConfig::GetBeamWidth()
{
    return beamWidth_;
}

double RadarConfig::CalculateRangeFactor()
{
    return ( rangeMax_ - rangeMin_ ) / ( gateCountMax_ - 1 );
}

double RadarConfig::GetSiteLongitude()
{
    return longitude_;
}

double RadarConfig::GetSiteLatitude()
{
    return latitude_;
}

double RadarConfig::GetSiteHeight()
{
    return height_;
}

const VideoConfig *RadarConfig::getVideoConfig()
{
    return videoConfig_;
}

const TSPIConfig *RadarConfig::getTSPIConfig()
{
    return tspiConfig_;
}

void RadarConfig::Load( const std::string &name, uint32_t gateCountMax, uint32_t shaftEncodingMax, double rotationRate,
                   double rangeMin, double rangeMax, double beamWidth )
{
    name_ = QString::fromStdString( name );
    gateCountMax_ = gateCountMax;
    shaftEncodingMax_ = shaftEncodingMax;
    rotationRate_ = rotationRate;
    rangeMin_ = rangeMin;
    rangeMax_ = rangeMax;
    beamWidth_ = beamWidth;
    rangeFactor_ = CalculateRangeFactor();
}

uint64_t RadarConfig::getID()
{
    return id_;
}

bool RadarConfig::GetEntry( const QDomElement &config, const QString &name, QString &value, QString &units )
{
    qCDebug( radarmsg ) << "RadarConfig::GetEntry" ;
    QDomElement element = config.firstChildElement( name );

    if ( element.isNull() )
    {
        qCWarning( radarmsg ) << "Missing field " << name ;
        return false;
    }

    units = "";

    if ( element.hasAttribute( "units" ) )
    {
        units = element.attribute( "units" );
    }

    value = element.text();
    qCDebug( radarmsg ) << "name: " << name << " value: " << value ;
    return true;
}

bool RadarConfig::Load( const QDomElement &config )
{
    qCDebug( radarmsg ) << "RadarConfig::Load" ;

    if ( ! config.hasChildNodes() )
    {
        qCWarning( radarmsg ) << "no config nodes found" ;
        return false;
    }

    QString value, units;

    if ( ! GetEntry( config, "id", value, units ) ) return false;

    id_ = value.toLongLong();

    if ( ! GetEntry( config, "name", value, units ) ) return false;

    name_ = value;

    if ( ! GetEntry( config, "gateCountMax", value, units ) ) return false;

    gateCountMax_ = value.toUInt();

    if ( gateCountMax_ < 2 )
    {
        qCWarning( radarmsg ) << "invalid gateCountMax value - " << gateCountMax_ ;
        return false;
    }

    if ( ! GetEntry( config, "shaftEncodingMax", value, units ) ) return false;

    shaftEncodingMax_ = value.toUInt();

    if ( shaftEncodingMax_ < 2 )
    {
        qCWarning( radarmsg ) << "invalid shaftEncodingMax value - " << shaftEncodingMax_ ;
        return false;
    }

    if ( ! GetEntry( config, "rangeMin", value, units ) ) return false;

    rangeMin_ = value.toDouble();

    if ( ! GetEntry( config, "rangeMax", value, units ) ) return false;

    rangeMax_ = value.toDouble();

    if ( rangeMin_ >= rangeMax_ || rangeMin_ < 0.0 || rangeMax_ <= 0.0 )
    {
        qCWarning( radarmsg ) << "invalid range specification - " << rangeMin_ << ","
                              << rangeMax_ ;
        return false;
    }

    if ( ! GetEntry( config, "rotationRate", value, units ) ) return false;

    rotationRate_ = value.toDouble();

    if ( rotationRate_ <= 0.0 )
    {
        qCWarning( radarmsg ) << "invalid rotationRate value - " << rotationRate_ ;
        return false;
    }

    if ( ! GetEntry( config, "beamWidth", value, units ) ) return false;

    beamWidth_ = value.toDouble();

    if ( beamWidth_ <= 0.0 )
    {
        qCWarning( radarmsg ) << "invalid beamWidth value - " << beamWidth_ ;
        return false;
    }

    rangeFactor_ = CalculateRangeFactor();

    if ( GetEntry( config, "latitude", value, units ) )
    {
        latitude_ = value.toDouble();

        if ( units == "radians" ) latitude_ = ZchxRadarUtils::radiansToDegrees( latitude_ );
    }

    if ( GetEntry( config, "longitude", value, units ) )
    {
        longitude_ = value.toDouble();

        if ( units == "radians" ) longitude_ = ZchxRadarUtils::radiansToDegrees( longitude_ );
    }

    if ( GetEntry( config, "height", value, units ) )
    {
        height_ = value.toDouble();

        if ( units == "feet" ) height_ = ZchxRadarUtils::feetToMeters( height_ );
    }

    QDomElement video_element = config.firstChildElement( "video" );

    if ( video_element.isNull() )
    {
        qCWarning( radarmsg ) << "Missing video field "  ;
        return false;
    }

    videoConfig_->Load( video_element );
    QDomElement tspi_element = config.firstChildElement( "tspi" );

    if ( tspi_element.isNull() )
    {
        qCWarning( radarmsg ) << "Missing tspi field "  ;
        return false;
    }

    tspiConfig_->Load( tspi_element );

    QDomElement cmdhostnode = config.firstChildElement( "commandhost" );
    cmdAddress_.setAddress( cmdhostnode.attribute( "host" ) );
    cmdPort_ = cmdhostnode.attribute( "port" ).toUShort();

    QDomNodeList cmdlist = config.elementsByTagName( "command" );

    for ( int i = 0; i < cmdlist.size(); ++i )
    {
        QDomElement cmdnode = cmdlist.at( i ).toElement();
        QString cmdname = cmdnode.attribute( "name" );
        QString cmdfile = cmdnode.text();
        mapCmd_.insert( cmdname, cmdfile );
    }

    return true;
}
