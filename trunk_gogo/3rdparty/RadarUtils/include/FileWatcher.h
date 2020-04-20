//**************************************************************************
//* @File: FileWatcher.h
//* @Description: 文件观察者接口
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
#ifndef UTILS_FILEWATCHER_H	// -*- C++ -*-
#define UTILS_FILEWATCHER_H

#include "UtilsGlobal.h"

namespace ZchxRadarUtils {


/** Abstract base class that monitors a file for changes to its modification timestamp. Derived clases must
    define a loadFile() method to do the actual loading.
*/
class ZCHX_API FileWatcher
{
public:


    /** Constructor.

        \param period number of seconds to wait between checks for file changes
    */
    FileWatcher(double period = 10.0);

    /** Destructor. Shuts down and disposes of any active monitor object.
     */
    virtual ~FileWatcher();

    /** Set the path to a file to watch. Invokes the virtual function loadFile() to signal derived classes to
        work with the file.

        \param path file path

        \return true if successful
    */
    bool setFilePath(const std::string& path);

    /** Obtain the modification time of the active file. Only valid after setFilePath().

        \return modification time of the file or time_t(-1) if file no longer
        exists.
    */
    time_t getModificationTime() const;

    /** Determine if the file has been modified since the last load.

        \return true if so
    */
    bool isStale() const { return getModificationTime() > lastModification_; }

    /** Check to see if the watched file has changed since the last load. If file is not stale, nothing is done,
    but routine still returns true.

    \return true if successful or file is not stale
    */
    bool check() { return isStale() ? reload() : true; }

    /** Reload a file and update the load timestamp. Invokes loadFile to do the actual work.

        \return true if successful
    */
    bool reload();

protected:

    /** Notification that the file has new contents. Derived classes must define.

        \param path configured file path

        \return true if successful
    */
    virtual bool loadFile(const std::string& path) = 0;

    virtual void start() { startMonitor(); }

    virtual void stop() { stopMonitor(); }

private:

    /** Start a separate thread to watch the configured file.

        \param period amount to time in seconds to sleep between checks for
        file changes
    */
    void startMonitor();

    /** Stop any running monitoor.
     */
    void stopMonitor();

    double period_;
    std::string path_;
    time_t lastModification_;
    struct Monitor;
    Monitor* monitor_;
};

} // end namespace ZchxRadarUtils

/** \file
 */

#endif
