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

        if ( radarnamenode.text() != mName )
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
                    node.firstChild().setNodeValue( QString::number( mLat, 'g', 9 ) );
                    QDomNode newnode = node.firstChild();
                    node.replaceChild( newnode, oldnode );
                }

                if ( node.nodeName() == "longitude" )
                {
                    QDomNode oldnode = node.firstChild();
                    node.firstChild().setNodeValue( QString::number( mLon, 'g', 9 ) );
                    QDomNode newnode = node.firstChild();
                    node.replaceChild( newnode, oldnode );
                }
            }
        }

        radarnode.firstChildElement( "commandhost" ).setAttribute( "host", mCmdIP );
        radarnode.firstChildElement( "commandhost" ).setAttribute( "port", mCmdPort);
        radarnode.firstChildElement( "video" ).setAttribute( "port", mVideoConfig->GetPort() );
        radarnode.firstChildElement( "video" ).setAttribute( "host", QString::fromStdString(mVideoConfig->GetHost()) );
        radarnode.firstChildElement( "tspi" ).setAttribute( "port", mTSPIConfig->GetPort() );
        radarnode.firstChildElement( "tspi" ).setAttribute( "host", QString::fromStdString(mTSPIConfig->GetHost()) );
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


QStringList RadarConfig::getAllCommand()
{
    return mMapCmd.keys();
}

QByteArray RadarConfig::getRadarCommand( const QString &cmd )
{
    QString name = mMapCmd.value( cmd );
    QString path = mCfgDir.filePath( name );
    QFile file( path );
    file.open( QFile::ReadOnly );
    return file.readAll();
}

void RadarConfig::sendRadarCommand( const QString &cmd )
{
    QTcpSocket cmdSocket;
    cmdSocket.connectToHost( mCmdIP, mCmdPort );

    if ( cmdSocket.waitForConnected( 300 ) )
    {
        int wbytes = cmdSocket.write( getRadarCommand( cmd ) );

        if ( cmdSocket.waitForBytesWritten( 100 ) )
            qInfo( radarmsg ) << "write succeed: " << mCmdIP << mCmdPort << wbytes;
        else
            qCWarning( radarmsg ) << "write failed: " << mCmdIP << mCmdPort << wbytes;
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

    mCfgDir = info.dir();
    mCfgDir.cd( "radarctrldata" );

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

        if ( radarName == mName )
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
    , mName( "Default" )
    , mGateCountMax( 4000 )
    , mShaftEncodingMax( 65535 )
    , mRotationRate( 6.0 )
    , mRangeMin( 1.0 )
    , mRangeMax( 300.0 )
    , mRangeFactor( CalculateRangeFactor() )
    , mBeamWidth( 0.001544 )
    , mLat( 37.0 + 49.0 / 60.0 + 7.83477 / 3600.0 )
    , mLon( -( 116.0 + 31.0 / 60.0 + 53.51066 / 3600.0 ) )
    , mHeight( 0.0 )
    , mVideoConfig( new VideoConfig )
    , mTSPIConfig( new TSPIConfig )
    , mHead(180.0)

{
}

RadarConfig::~RadarConfig()
{
}

double RadarConfig::CalculateRangeFactor()
{
    mRangeFactor = (mRangeMax - mRangeMin) / (mGateCountMax - 1);
    return mRangeFactor;
}

QString RadarConfig::getVideoIP()
{
    return QString(getVideoConfig()->GetHost());
}

int RadarConfig::getVideoPort()
{
    return getVideoConfig()->GetPort();
}

void RadarConfig::Load( const std::string &name, uint32_t gateCountMax, uint32_t shaftEncodingMax, double rotationRate,
                   double rangeMin, double rangeMax, double beamWidth )
{
    mName = QString::fromStdString( name );
    mGateCountMax = gateCountMax;
    mShaftEncodingMax = shaftEncodingMax;
    mRotationRate = rotationRate;
    mRangeMin = rangeMin;
    mRangeMax = rangeMax;
    mBeamWidth = beamWidth;
    mRangeFactor = CalculateRangeFactor();
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

    mID = value.toLongLong();

    if ( ! GetEntry( config, "name", value, units ) ) return false;

    mName = value;

    if ( ! GetEntry( config, "gateCountMax", value, units ) ) return false;

    mGateCountMax = value.toUInt();

    if ( mGateCountMax < 2 )
    {
        qCWarning( radarmsg ) << "invalid gateCountMax value - " << mGateCountMax ;
        return false;
    }

    if ( ! GetEntry( config, "shaftEncodingMax", value, units ) ) return false;

    mShaftEncodingMax = value.toUInt();

    if ( mShaftEncodingMax < 2 )
    {
        qCWarning( radarmsg ) << "invalid shaftEncodingMax value - " << mShaftEncodingMax ;
        return false;
    }

    if ( ! GetEntry( config, "rangeMin", value, units ) ) return false;

    mRangeMin = value.toDouble();

    if ( ! GetEntry( config, "rangeMax", value, units ) ) return false;

    mRangeMax = value.toDouble();

    if ( mRangeMin >= mRangeMax || mRangeMin < 0.0 || mRangeMax <= 0.0 )
    {
        qCWarning( radarmsg ) << "invalid range specification - " << mRangeMin << ","
                              << mRangeMax ;
        return false;
    }

    if ( ! GetEntry( config, "rotationRate", value, units ) ) return false;

    mRotationRate = value.toDouble();

    if ( mRotationRate <= 0.0 )
    {
        qCWarning( radarmsg ) << "invalid rotationRate value - " << mRotationRate ;
        return false;
    }

    if ( ! GetEntry( config, "beamWidth", value, units ) ) return false;

    mBeamWidth = value.toDouble();

    if ( mBeamWidth <= 0.0 )
    {
        qCWarning( radarmsg ) << "invalid beamWidth value - " << mBeamWidth ;
        return false;
    }

    mRangeFactor = CalculateRangeFactor();

    if ( GetEntry( config, "latitude", value, units ) )
    {
        mLat = value.toDouble();

        if ( units == "radians" ) mLat = ZchxRadarUtils::radiansToDegrees( mLat );
    }

    if ( GetEntry( config, "longitude", value, units ) )
    {
        mLon = value.toDouble();

        if ( units == "radians" ) mLon = ZchxRadarUtils::radiansToDegrees( mLon );
    }

    if ( GetEntry( config, "height", value, units ) )
    {
        mHeight = value.toDouble();

        if ( units == "feet" ) mHeight = ZchxRadarUtils::feetToMeters( mHeight );
    }

    QDomElement video_element = config.firstChildElement( "video" );

    if ( video_element.isNull() )
    {
        qCWarning( radarmsg ) << "Missing video field "  ;
        return false;
    }

    mVideoConfig->Load( video_element );
    QDomElement tspi_element = config.firstChildElement( "tspi" );

    if ( tspi_element.isNull() )
    {
        qCWarning( radarmsg ) << "Missing tspi field "  ;
        return false;
    }

    mTSPIConfig->Load( tspi_element );

    QDomElement cmdhostnode = config.firstChildElement( "commandhost" );
    mCmdIP =  cmdhostnode.attribute( "host" );
    mCmdPort = cmdhostnode.attribute( "port" ).toUShort();

    QDomNodeList cmdlist = config.elementsByTagName( "command" );

    for ( int i = 0; i < cmdlist.size(); ++i )
    {
        QDomElement cmdnode = cmdlist.at( i ).toElement();
        QString cmdname = cmdnode.attribute( "name" );
        QString cmdfile = cmdnode.text();
        mMapCmd.insert( cmdname, cmdfile );
    }

    return true;
}
