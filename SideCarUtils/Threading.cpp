//**************************************************************************
//* @File: Threading.cpp
//* @Description: 线程类
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
//*   1.0  2017/03/10    李鹭       初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************
// -*- C++ -*-

#include <cmath>        // for std::floor
#include <cstring>      // for stderror
#include <errno.h>      // for EBUSY, ETIMEDOUT
#include <iostream>
#include <limits>
#include <sstream>      // for std::ostringstream
#include <stdexcept>        // for std::out_of_range
#include <time.h>       // for nanosleep
#include <sys/time.h>       // for gettimeofday

#include "Threading.h"

using namespace Threading;

Mutex::Ref Guard::mutex_;
pthread_once_t Guard::onceControl_ = PTHREAD_ONCE_INIT;

void
Guard::Init()
{
    mutex_ = Mutex::Make();
}

extern "C" {
    void initStub()
    {
        Guard::Init();
    }
}

void
Guard::Lock( bool state ) throw( Mutex::FailedInit, Mutex::FailedLock, Mutex::FailedUnlock )
{
    pthread_once( &onceControl_, &initStub );

    if ( state )
    {
        mutex_->lock();
    }
    else
    {
        mutex_->unlock();
    }
}

ThreadingException::ThreadingException( const char *className, const char *routine, int rc ) throw()
    : ZchxRadarUtils::Exception()
{
    *this << className << "::" << routine << " - rc: " << rc << ' ' << strerror( rc );
}

/** Create an absolute time consisting of the current time (now) + some offset value, and use the result to fill

    in a given timespec structure. \param offset number of seconds added to the current time. \param abs if

    true, generate an absolute time \param ts timespec structure filled in with the future time
*/
static void
fillTimespec( double offset, bool abs, timespec &ts ) throw( std::out_of_range )
{
    static const long kNanosecondsPerSecond = 1000000000L;

    // Get current time in seconds/microseconds.
    //
    if ( abs )
    {
        timeval now;
        gettimeofday( &now, NULL );
        ts.tv_sec = now.tv_sec;
        ts.tv_nsec = now.tv_usec * 1000;
    }
    else
    {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
    }

    // Now add in offset to make a time in the future.
    //
    double tmp = std::floor( offset );

    if ( tmp > std::numeric_limits<long>::max() )
    {
        throw std::out_of_range( "fillTimespec" );
    }

    // Extract the number of whole seconds from given value.
    //
    ts.tv_sec += static_cast<long>( tmp );
    tmp = offset - tmp;
    // If the offset value still has something in it (> 0.0 && < 1.0) then use as a multiplier to obtain a
    // fraction of nanoseconds. Add the fraction to the timespec field, but detect and massage any overflow.
    //
    ts.tv_nsec += static_cast<long>( tmp * kNanosecondsPerSecond );

    if ( ts.tv_nsec >= kNanosecondsPerSecond )
    {
        ts.tv_sec += ts.tv_nsec / kNanosecondsPerSecond;
        ts.tv_nsec %= kNanosecondsPerSecond;
    }
}

Mutex::Mutex( int kind ) throw( Mutex::FailedInit )
{
    pthread_mutexattr_t attr;
    int rc = pthread_mutexattr_init( &attr );

    if ( rc ) throw FailedInit( rc );

    rc = pthread_mutexattr_settype( &attr, kind );

    if ( rc ) throw FailedInit( rc );

    rc = pthread_mutex_init( &mutex_, &attr );

    if ( rc ) throw FailedInit( rc );

    rc = pthread_mutexattr_destroy( &attr );

    if ( rc ) throw FailedInit( rc );
}

Mutex::~Mutex() throw()
{
    pthread_mutex_destroy( &mutex_ );
}

void
Mutex::lock() throw( Mutex::FailedLock )
{
    int rc = pthread_mutex_lock( &mutex_ );

    if ( rc )
    {
        throw FailedLock( rc );
    }
}

void
Mutex::unlock() throw( Mutex::FailedUnlock )
{
    int rc = pthread_mutex_unlock( &mutex_ );

    if ( rc )
    {
        throw FailedUnlock( rc );
    }
}

bool
Mutex::tryToLock() throw( Mutex::FailedLock )
{
    int rc = pthread_mutex_trylock( &mutex_ );

    switch ( rc )
    {
    case 0:
    case EBUSY:
    case EDEADLK:
        break;

    default:
        throw FailedLock( rc );
    }

    // Return true if no error (obtained lock), and false otherwise
    //
    return rc == 0;
}

Condition::Condition( const Mutex::Ref &mutex ) throw( Condition::FailedInit )
    : mutex_( mutex )
{
    int rc = pthread_cond_init( &condition_, 0 );

    if ( rc ) throw FailedInit( rc );
}

Condition::~Condition() throw()
{
    pthread_cond_destroy( &condition_ );
}

void
Condition::signal() throw()
{
    pthread_cond_signal( &condition_ );
}

void
Condition::broadcast() throw()
{
    pthread_cond_broadcast( &condition_ );
}

void
Condition::waitForSignal() throw( Condition::FailedWaitForSignal )
{
#ifdef darwin
    timedWaitForSignal( 0.5 );
#else
    int rc = pthread_cond_wait( &condition_, mutex_->getMutexID() );

    if ( rc ) throw FailedWaitForSignal( rc );

#endif
}

bool
Condition::timedWaitForSignal( double duration ) throw( Condition::FailedWaitForSignal, std::out_of_range )
{
    timespec ts;
    fillTimespec( duration, true, ts );
#ifdef darwin
    pthread_testcancel();
#endif
    int rc = pthread_cond_timedwait( &condition_, mutex_->getMutexID(), &ts );
#ifdef darwin
    pthread_testcancel();
#endif

    if ( rc != 0 && rc != ETIMEDOUT ) throw FailedWaitForSignal( rc );

    // Return true if timed out, false otherwise (eg. signal wakeup)
    //
    return rc == ETIMEDOUT;
}

Locker::Locker( const Mutex::Ref &mutex ) throw( Mutex::FailedLock )
    : mutex_( mutex )
{
    mutex_->lock();
}

Locker::Locker( const Condition::Ref &condition ) throw( Mutex::FailedLock )
    : mutex_( condition->mutex() )
{
    mutex_->lock();
}

Locker::~Locker() throw( Mutex::FailedUnlock )
{
    mutex_->unlock();
}

void
Thread::Sleep( double duration ) throw( std::out_of_range )
{
    timespec ts;
    fillTimespec( duration, false, ts );
#ifdef darwin
    pthread_testcancel();
#endif
    ::nanosleep( &ts, 0 );
#ifdef darwin
    pthread_testcancel();
#endif
}

void
Thread::announceStarted()
{
    Locker lock( condition_ );
    started_ = true;
    running_ = true;
    condition_->signal();
}

void
Thread::announceFinished()
{
    Locker lock( condition_ );
    running_ = false;
    condition_->signal();
}

/** Helper class that calls the announceStarted and announceFinished methods for a thread. Used in conjunction
    with the pthread_cleanup_push and pthread_cleanup_pop routines.
*/
struct Announcer
{
    /** Definition of function to call to announce.
     */
    using AnnounceProc = void ( Thread::* )();

    /** Constructor. Calls the announceStarted routine for the thread, but remembers the announceFinished method
        for use when we are destroyed.
    */
    Announcer( Thread *obj, AnnounceProc startedProc, AnnounceProc finishedProc )
        : obj_( obj ), finishedProc_( finishedProc )
    {
        ( obj_->*startedProc )();
    }

    /** Destructor. Call Thread::announceFinished for our thread.
     */
    ~Announcer()
    {
        ( obj_->*finishedProc_ )();
    }

private:
    Thread *obj_;
    AnnounceProc finishedProc_;
};

/** C routine given to pthread_cleanup_push to properly destroy the Annoncer object created for a thread.
 */
extern "C" {
    void cleanupStub( void *obj )
    {
        delete static_cast<Announcer *>( obj );
    }
}

void
Thread::runWrapper()
{
    // Make sure the thread variables are properly set if the thread is canceled, or an exception is raised.
    //
//    pthread_cleanup_push( &cleanupStub, new Announcer( this, &Thread::announceStarted, &Thread::announceFinished ) );

//    try
//    {
//        run();
//    }
//    catch ( Utils::Exception &ex )
//    {
//        std::cerr << tid_ << " Thread:runWrapper: caught Utils::Exception: " << ex.err() << std::endl;
//    }
//    catch ( std::exception &ex )
//    {
//        std::cerr << tid_ << " Thread::runWrapper std::exception: " << ex.what() << std::endl;
//    }
//    catch ( ... )
//    {
//        std::cerr << tid_ << " Thread::runWrapper: caught unknown exception" << std::endl;
//    }

//    pthread_cleanup_pop( true );

}

/** Helper class used to invoke the Thread::runWrapper() method. We do things this way so we don't have to open
    holes in the Thread class permissions.
*/
struct Starter
{
    using RunWrapper = void ( Thread::* )();

    Starter( Thread *obj, RunWrapper runWrapper ) : obj_( obj ), runWrapper_( runWrapper ) {}

    void start()
    {
        ( obj_->*runWrapper_ )();
    }

    Thread *obj_;
    RunWrapper runWrapper_;
};

/** C routine called by pthread_create() to start a new thread. Simply calls the registered routine
    Thread::runWrapper.
*/
extern "C" {
    void* starterStub( void *arg )
    {
        static_cast<Starter *>( arg )->start();
        return 0;
    }
}

void
Thread::start() throw( Thread::FailedCreate, Mutex::FailedLock, Condition::FailedInit )
{
    // Create a helper object to invoke our runWrapper method in a new thread.
    //
    Starter starter( this, &Thread::runWrapper );
    int rc = pthread_create( &tid_, 0, starterStub, &starter );

    if ( rc ) throw FailedCreate( rc );

    // DO NOT exit until the thread has announced that is has started. Otherwise, we destroy the Starter object
    // above before the starterStub routine can reference it in the other thread.
    //
    Locker lock( condition_ );

    while ( ! started_ )
    {
        condition_->waitForSignal();
    }
}

void
Thread::waitToFinish() throw( Mutex::FailedLock )
{
    Locker lock( condition_ );

    while ( running_ )
    {
        condition_->waitForSignal();
    }
}

Thread::~Thread()
{
    if ( running_ )
    {
        pthread_cancel( tid_ );
        Locker lock( condition_ );

        while ( running_ )
        {
            condition_->waitForSignal();
        }

        pthread_detach( tid_ );
    }
}

bool
Thread::isRunning() throw( Mutex::FailedLock )
{
#ifdef darwin
    pthread_testcancel();
#endif
    Locker lock( condition_ );
    return running_;
}

bool
Thread::detach() throw()
{
    int rc = pthread_detach( tid_ );
    return rc == 0;
}

bool
Thread::join() throw( Thread::FailedJoin )
{
    if ( ! started_ ) return false;

    int rc = pthread_join( tid_, 0 );

    if ( rc && rc != ESRCH && rc != EINVAL ) throw FailedJoin( rc );

    return rc == 0;
}

void
Thread::cancel() throw( Thread::FailedCancel )
{
    if ( ! started_ ) return;

    int rc = pthread_cancel( tid_ );

    if ( rc ) throw FailedCancel( rc );
}

bool
Thread::operator ==( const Thread &rhs ) const throw()
{
    return pthread_equal( tid_, rhs.tid_ ) != 0;
}
