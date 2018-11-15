//**************************************************************************
//* @File: Threading.h
//* @Description: 线程接口
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
#ifndef THREADING_H		// -*- C++ -*-
#define THREADING_H

#ifdef _WIN32
#include "pthread.h"
#else
#include <pthread.h>
#endif

#include <stdexcept>		// for std::out_of_range
#include <string>

#include "boost/shared_ptr.hpp"
#include "Exception.h"

namespace Threading {

/** Top-level exception class for all thread-related exceptions.
 */
struct ThreadingException : public ZchxRadarUtils::Exception
{
    /** Constructor. Fills in exception with names of the class and class method that threw the exception.
        
        \param className name of the class

        \param routine name of the method

        \param rc return value from a pthreads library call that reported an error
    */

    
    ThreadingException(const char* className, const char* routine, int rc) throw();
};

/**
   Mutual-exclusion lock. Provides a means for a thread to restrict thread execution in a block of code to one
   thread at a time. A thread requests ownership of a mutex by calling Mutex::lock(). If the mutex is available,
   it grants ownership to the requesting thread. Any other threads also seeking ownership will block on the
   Mutex::lock() call until some time in the future when the mutex is again available. When a thread is done
   with the mutex, it must revoke its ownership by calling Mutex::unlock(). Failure to do so will most likely
   cause a program the fail. To prevent this, one should use a Locker instance to manage the locking of a mutex
   or condition, since it provides an exception-safe way of making sure that the mutex is unlocked when the
   scope the Locker instance is in goes away.
*/
class ZCHX_API Mutex
{
public:
    using Ref = boost::shared_ptr<Mutex>;

    /** Top-level exception class for all Mutex-related exceptions
     */
    struct Exception : public ThreadingException {
	Exception(const char* routine, int rc) throw() : ThreadingException("Mutex", routine, rc) {}
    };

    /** Exception thrown if there is a problem with pthread_mutex_init
     */
    struct FailedInit : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedInit> {
	FailedInit(int rc) throw() : Exception("Mutex", rc) {} };

    /** Exception thrown if there is a problem with pthread_mutex_lock
     */
    struct FailedLock : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedLock> {
	FailedLock(int rc) throw() : Exception("lock", rc) {} };

    /** Exception thrown if there is a problem with pthread_mutex_unlock
     */
    struct FailedUnlock : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedLock> {
	FailedUnlock(int rc) throw() : Exception("unlock", rc) {} };

    
    /** Factory method for creating new Mutex objects.
        
        \param kind which variant to create

        \return reference to new Mutex object
    */
    static Ref Make(int kind = PTHREAD_MUTEX_ERRORCHECK) throw(FailedInit)
	{
	    Ref ref(new Mutex(kind));
	    return ref;
	}

    /** Destructor. Releases system-dependent mutex resource.
     */
    virtual ~Mutex() throw();

    /** Acquire exclusive ownership of the mutex. Other threads attempting to lock the same mutex will block and
        wait until the mutex is unlocked.
    */
    void lock() throw(FailedLock);

    /** Similar to the Mutex::lock method, but will not block if the mutex is currently locked.

        \return true if locking was successful, false if mutex is already locked by another thread (if already
        locked by same thread as caller, an exception is thrown)
    */
    bool tryToLock() throw(FailedLock);

    /** Release ownership of the mutex.
     */
    void unlock() throw(FailedUnlock);

    /** Accessor to system-dependent mutex resource.
     */
    pthread_mutex_t* getMutexID() throw() { return &mutex_; }

    bool operator==(const Mutex& rhs) const { return this == &rhs; }

    bool operator!=(const Mutex& rhs) const { return this != &rhs; }
    
private:

    /** Constructor. Initializes system-dependent mutex resource.

        \param kind type of mutex to create. See pthread_mutexattr_settype man page for more information about
        this value.
    */
    Mutex(int kind = PTHREAD_MUTEX_ERRORCHECK) throw(FailedInit);

    /** Copying is prohibited.
     */
    Mutex(const Mutex&) throw();

    /** Assigning is prohibited.
     */
    Mutex& operator =(const Mutex&) throw();

    pthread_mutex_t mutex_;
};

/** Utility class whose instances all share the same mutex, thus providing a simple way to protect a section of
    code for one thread at a time.

    @code
    // multi-threaded code
    {
    Guard sentry;
    // thread-safe code
    }
    // multi-threaded code
    @endcode
*/
class ZCHX_API Guard
{
public:
    static void Init();

    Guard() throw(Mutex::FailedInit, Mutex::FailedLock) { Lock(true); }
    ~Guard() throw(Mutex::FailedInit, Mutex::FailedUnlock) { Lock(false); }

private:

    Guard(const Guard&);

    Guard& operator=(const Guard&);

    void operator delete(void*);

    static void Lock(bool state) throw(Mutex::FailedInit, Mutex::FailedLock, Mutex::FailedUnlock);
    static Mutex::Ref mutex_;
    static pthread_once_t onceControl_;
};

/** Condition checking for threads. Condition checks act as a kind of flip-flop for a mutex. They allow threads
    to periodically check a running condition in a thread-safe manner. Each thread wishing to check or
    manipulate the condition must first acquire ownership of the instance (which is derived from Mutex)
*/
class ZCHX_API Condition
{
public:
    using Ref = boost::shared_ptr<Condition>;

    /** Top-level exception class for all Condition-related exceptions.
     */
    struct Exception : public ThreadingException {
	Exception(const char* routine, int rc) throw() : ThreadingException("Condition", routine, rc) {}
    };

    /** Exception thrown if there is a problem with pthread_cond_init.
     */
    struct FailedInit : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedInit> {
	FailedInit(int rc) throw() : Exception("Condition", rc) {} };

    /** Exception thrown if there is a problem with pthread_cond_wait.
     */
    struct FailedWaitForSignal : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedWaitForSignal> {
	FailedWaitForSignal(int rc) throw() : Exception("waitForSignal", rc) {} };

    /** Exception thrown if there is a problem with pthread_cond_timedwait.
     */
    struct FailedTimedWaitForSignal : public Exception,
                                      public ZchxRadarUtils::ExceptionInserter<FailedTimedWaitForSignal> {
	FailedTimedWaitForSignal(int rc) throw() : Exception("timedWaitForSignal", rc) {} };

    static Ref Make()
	{
	    Ref ref(new Condition(Mutex::Make()));
	    return ref;
	}

    static Ref Make(const Mutex::Ref& mutex)
	{
	    Ref ref(new Condition(mutex));
	    return ref;
	}

    /** Destructor. Releases system-dependent condition variable resource.
     */
    virtual ~Condition() throw();

    /** Announce a change in the condition we represent. One or more threads blocking on the condition via
        Condition::waitForSignal will awake with ownership of the locking mutex, and they can safely check the
        condition to see if it has been met.
    */
    void signal() throw();

    /** Announce to all blocking threads that a change in the condition has occured. When resumed, each thread
        will have ownership of the associated mutex, and they can verify the condition change in a thread-safe
        manner.
    */
    void broadcast() throw();

    /** The current thread will release ownership of the mutex lock, and wait until another thread signals a
        condition change. Whenever the thread wakes up because of a condition change signal, it will have
        ownership of the mutex lock so that it may thread-safely check the condition. If the condition is met.
        Usually if the condition is not met, the thread will call Thread::waitForSignal again; when the
        condition is satisfied, the thread will usually continue operation, but must release the mutex lock when
        it is appropriate to do so.
    */
    void waitForSignal() throw(FailedWaitForSignal);

    /** Similar to Condition::waitForSignal, but the call will only block for a certain length of time. If the
        timer expires, the call will return false, but the thread will still have ownership of the mutex.

        \param duration amount of time to wait before giving up \return true if the call did not time out; false
        otherwise
    */
    bool timedWaitForSignal(double duration) throw(FailedWaitForSignal, std::out_of_range);

    Mutex::Ref mutex() const { return mutex_; }

private:

    /** Constructor. Initializes system-dependent condition variable resource.
     */
    Condition(const Mutex::Ref& mutex) throw(FailedInit);

    /** Copying is prohibited
     */
    Condition(const Condition&) throw();

    /** Assigning is prohibited
     */
    Condition& operator =(const Condition&) throw();

    Mutex::Ref mutex_;
    pthread_cond_t condition_;
};

/** Resoure manager for a mutex. A Locker instance is allocated on the stack. When program flow exits the scope
    in which the Locker was declared, the Locker's destructor is called which automatically unlocks the mutex.
*/
class ZCHX_API Locker
{
public:

    /** Constructor.

        \param mutex the mutex to use for the lock
    */
    Locker(const Mutex::Ref& mutex) throw(Mutex::FailedLock);

    /** Constructor.

        \param mutex the mutex to use for the lock
    */
    Locker(const Condition::Ref& condition) throw(Mutex::FailedLock);

    /** Destructor. Release any hold we may have on the mutex.
     */
    ~Locker() throw(Mutex::FailedUnlock);

private:

    /** Heap objects are prohibited.
     */
    static void* operator new(size_t);

    /** Copying is prohibited.
     */
    Locker(const Locker& rhs) throw();

    /** Assigning is prohibited.
     */
    Locker& operator =(const Locker&) throw();

    Mutex::Ref mutex_;
};

/** Abstract base class representation of a thread. When a new Thread instance is created, it is running in the
    thread that created it. To actually start a new thread, call the instance method Thread::start. New threads
    invoke the Thread::run method which must be defined by derived classes.
*/
class ZCHX_API Thread
{
public:

    /** Top-level exception class for all Thread-related exceptions.
     */
    struct Exception : public ThreadingException {
	Exception(const char* routine, int rc) throw() : ThreadingException("Thread", routine, rc) {} };

    /** Exception thrown if there is a problem with pthread_create.
     */
    struct FailedCreate : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedCreate> {
	FailedCreate(int rc) throw() : Exception("start", rc) {} };

    /** Exception thrown if there is a problem with pthread_create.
     */
    struct FailedJoin : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedJoin> {
	FailedJoin(int rc) throw() : Exception("join", rc) {} };

    /** Exception thrown if there is a problem with pthread_create.
     */
    struct FailedCancel : public Exception, public ZchxRadarUtils::ExceptionInserter<FailedCancel> {
	FailedCancel(int rc) throw() : Exception("cancel", rc) {} };

    /** Utility to pause a thread for some amount of time. Other threads can run while this one is sleeping.
        
        \param duration amount of time to pause, in seconds/microseconds
    */
    static void Sleep(double duration) throw(std::out_of_range);

    static pthread_t Self() { return pthread_self(); }

    /** Constructor.
     */
    Thread() throw(Condition::FailedInit, Mutex::FailedInit)
    : tid_(pthread_self()), condition_(Condition::Make()), running_(false), started_(false) {}

    /** Destructor. Release system-dependent resources.
     */
    virtual ~Thread();

    /** Fetch the thread ID.

        \return thread ID
    */
    pthread_t getThreadId() const throw() { return tid_; }

    /** \return true if the thread is still running. Not necessarily thread-safe, but for the resolution
        required vs. the cost of locking this attribute, its OK.
    */
    bool isRunning() throw(Mutex::FailedLock);

    /** Pause the current thread until the thread represented by this instance has finished running.
     */
    void waitToFinish() throw(Mutex::FailedLock);

    /** Detach the running thread so that a join() call is unnecessary to clean up thread-specific memory when
        the thread run() method finishes.

        \return true if successful
    */
    bool detach() throw();

    /** Another thread wishes to consume the one we represent. This method will block the calling thread until
        our thread exits.

        \return true if pthread_join was successful; false if our thread has already terminated and has joined
        with another thread.
    */
    bool join() throw(Thread::FailedJoin);

    /** Request that a our thread be canceled. A thread cancellation is deferred until the thread reaches a
        cancellation point. Note that usually a call to join() should follow a cancel request, but is not
        mandatory.
    */
    void cancel() throw(Thread::FailedCancel);

    /** See if two Thread objects represent the same thread.

        \param rhs Thread object to compare with

        \return true if same
    */
    bool operator ==(const Thread& rhs) const throw();

    /** See if two Thread objects do not represent the same thread.

        \param rhs Thread object to compare with
        
        \return true if not the same
    */
    bool operator !=(const Thread& rhs) const throw() { return ! (*this == rhs); }

    /** \return true if this instance represents the thread currently executing
     */
    bool isActive() const throw() { return pthread_self() == tid_; }
    
    /** Start a new thread. The new thread will invoke Thread::run, which must be defined by derived classes.
        Creates a new Starter object to do the actually starting. Call will not return until the new thread is
        up and running.
    */
    void start() throw(FailedCreate, Mutex::FailedLock, Condition::FailedInit);

protected:

    /** Thread-specific code to run when Thread::start is called. Derived classes must define.
     */
    virtual void run() = 0;

private:
    void runWrapper();
    void announceStarted();
    void announceFinished();

    /** Copying is prohibited.
     */
    Thread(const Thread&) throw();

    /** Assigning is prohibited.
     */
    Thread& operator =(const Thread&) throw();

    pthread_t tid_;		///< System-specific thread resource.
    Condition::Ref condition_;	///< Condition/mutex for running_ flag. 
    bool running_;		///< True if thread is still running.
    bool started_;		///< True if instance represents unique thread.
};

} // end namespace Threading

#endif
