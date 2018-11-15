//**************************************************************************
//* @File: FileWatcher.cpp
//* @Description: 文件观察者类
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
//*   1.0  2017/03/16    李鹭       初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************
#include <sys/stat.h>       // for struct stat

#include <QLoggingCategory>

#include "Threading.h"

#include "FileWatcher.h"

using namespace ZchxRadarUtils;

struct FileWatcher::Monitor : public Threading::Thread
{
    Monitor( FileWatcher &config, double sleep )
        : Threading::Thread(), config_( config ), sleep_( sleep ),
          stopRunningCondition_( Threading::Condition::Make() ), stopRunning_( false )
    {}

    void run()
    {
        qCDebug( radarutils ) << "Monitor::run" ;
        Threading::Locker lock( stopRunningCondition_ );

        while ( ! stopRunning_ )
        {
            qCDebug( radarutils ) << "checking for stale file";

            if ( config_.isStale() )
            {
                qCDebug( radarutils ) << "file is stale - reloading";
                config_.reload();
            }

            stopRunningCondition_->timedWaitForSignal( sleep_ );
        }
    }

    void stop()
    {
        qCDebug( radarutils ) << "Monitor::stop" ;
        Threading::Locker lock( stopRunningCondition_ );
        stopRunning_ = true;
        stopRunningCondition_->signal();
    }

    void die()
    {
        qCDebug( radarutils ) << "Monitor::die" ;
        stop();
        join();
    }

    FileWatcher &config_;
    double sleep_;
    Threading::Condition::Ref stopRunningCondition_;
    bool stopRunning_;
};


FileWatcher::FileWatcher( double period )
    : period_( period ), path_( "" ), lastModification_( 0 ), monitor_( 0 )
{
    if ( period_ < 0.1 )
    {
        period_ = 0.1;
    }
}

FileWatcher::~FileWatcher()
{
    stopMonitor();
}

time_t
FileWatcher::getModificationTime() const
{
    qCDebug( radarutils ) << "getModificationTime" ;
    struct stat st;
    int rc = ::stat( path_.c_str(), &st );

    if ( rc == -1 ) st.st_mtime = time_t( -1 );

    qCDebug( radarutils ) << QString::fromStdString( path_ ) << ' ' << st.st_mtime ;
    return st.st_mtime;
}

void
FileWatcher::startMonitor()
{
    qCDebug( radarutils ) << "startMonitor" ;
    monitor_ = new Monitor( *this, period_ );
    monitor_->start();
}

void
FileWatcher::stopMonitor()
{
    qCDebug( radarutils ) << "stopMonitor" ;

    if ( monitor_ )
    {
        monitor_->die();
        delete monitor_;
        monitor_ = 0;
    }
}

bool
FileWatcher::setFilePath( const std::string &path )
{
    qCDebug( radarutils ) << "setFilePath: %s" << QString::fromStdString( path ) ;
    stop();
    path_ = path;

    if ( path_.size() )
    {
        if ( ! reload() )
        {
            qCWarning( radarutils ) << "failed to load config file " << QString::fromStdString( path ) ;
            return false;
        }

        start();
    }

    return true;
}

bool
FileWatcher::reload()
{
    qCDebug( radarutils ) << "reload" ;
    time_t lm = getModificationTime();
    qCDebug( radarutils ) << QString::fromStdString( path_ ) << ' ' << lm  ;

    if ( lm == time_t( -1 ) )
    {
        qCWarning( radarutils ) << "failed to locate configuration file " << QString::fromStdString( path_ ) ;
        return false;
    }

    if ( ! loadFile( path_ ) )
    {
        qCWarning( radarutils ) << "failed to reload configuration file " << QString::fromStdString( path_ ) ;
        return false;
    }

    lastModification_ = lm;
    return true;
}
