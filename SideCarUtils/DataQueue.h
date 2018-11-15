//**************************************************************************
//* @File: DataQueue.h
//* @Description: 数据队列线程接口
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

#ifndef DATAQUEUE_H		// -*- C++ -*-
#define DATAQUEUE_H

#include <cstddef>
#include <deque>
#include <limits>
#include "Threading.h"

namespace Threading {

/** Wrapper around a Locker object that will signal a condition variable if it is "armed" any time before the
    object is destroyed. Used by DataQueueBase to signal threads waiting on a queue full or empty condition.
*/
class ZCHX_API Signaler
{
public:
    
    /** Constructor.

        \param condition condition variable to signal when armed.
    */
    Signaler(Condition::Ref condition) : condition_(condition), locker_(condition_), armed_(false) {}
    
    /** Destructor. Signal condition variable if armed.
     */
    ~Signaler() { if (armed_) condition_->signal(); }

    /** Arm the signaler.
     */
    void arm() { armed_ = true; }

private:
    Condition::Ref condition_; ///< Condition variable to signal
    Locker locker_;		     ///< Locker for the c.v. mutex
    bool armed_;		     ///< True if armed
};

/** Abstract base class for all type-specific data queue classes. Understands locking and signaling protocol for
    the data queue.
*/
class ZCHX_API DataQueueBase
{
protected:
    DataQueueBase();
    virtual ~DataQueueBase() {}

    Mutex::Ref mutex() const { return mutex_; }
    Condition::Ref readCondition() const { return rCond_; }
    Condition::Ref writeCondition() const { return wCond_; }

    void waitUntilReadable(Signaler& signaler);
    void waitUntilWritable(Signaler& signaler);

    virtual bool queueEmpty() const = 0;
    virtual bool queueFull() const = 0;

private:
    Mutex::Ref mutex_;
    Condition::Ref rCond_;
    Condition::Ref wCond_;
};

/** Template class for type-specific data queues. A data queue is a FIFO container with a maximum size which
    supports thread-safe inserts and deletions. If the queue is empty, any attempt to obtain a value from the
    queue will block. Likewise, if the queue is full any attempt to add another value to the queue will block.
*/
template <typename T>
class ZCHX_API TDataQueue : public DataQueueBase
{
public:

    /** Default constructor. Queue size is essentially unlimited, so writes will never block.
     */
    TDataQueue() : DataQueueBase(), queue_(), maxSize_(std::numeric_limits<size_t>::max()) {}

    /** Constructor.

        \param maxSize maximum number of entries for the queue.
    */
    TDataQueue(size_t maxSize) : DataQueueBase(), queue_(), maxSize_(maxSize) {}

    /** Determine if the queue is empty (thread-safe)

	\return true if the queue is empty
    */
    bool empty() const { Locker l(mutex()); return queue_.empty(); }

    /** Determin if the queue is full (thread-safe)

        \return true if the queue is full
    */
    bool full() const { Locker l(mutex()); return queue_.size() == maxSize_; }

    /** Obtain the next object from the queue (thread-safe). Blocks if the queue is empty.

        \return next available object
    */
    T pop()
	{
	    Signaler signaler(writeCondition());
	    waitUntilReadable(signaler);
	    T tmp = queue_.back();
	    queue_.pop_back();
	    return tmp;
	}

    /** Add an object to the queue. Blocks if the queue is full.

        \param value object to add
    */
    void push(const T& value)
	{
	    Signaler signaler(readCondition());
	    waitUntilWritable(signaler);
	    queue_.push_front(value);
	}

private:
    bool queueEmpty() const { return queue_.empty(); }
    bool queueFull() const { return queue_.size() == maxSize_; }

    using Container = std::deque<T>;
    Container queue_;
    size_t maxSize_;
};

/** Queue of char buffers that is allocated in one big block. Pointers to parts of the allocated block are held
    in the free_ and inUse_ queues. One thread obtains a buffer via the allocateBuffer() method, fills it with
    some data, and then posts it to another thread for processing with the pushWork() method. Another thread (a
    worker) obtains buffers to process using the popWork() method. When it is done processing the buffer, it
    makes it available to the other thread by calling the releaseBuffer() method.
*/
class ZCHX_API WorkBufferQueue
{
public:
    struct WorkEntry
    {
	void* ptr;
	size_t size;
	WorkEntry(void* p, size_t s) : ptr(p), size(s) {}
    };

    using FreeQueue = TDataQueue<void*>;
    using WorkQueue = TDataQueue<WorkEntry>;

    WorkBufferQueue(size_t queueSize, size_t blockSize);
    ~WorkBufferQueue();

    void* allocateBuffer() { return free_.pop(); }
    void releaseBuffer(void* ptr) { free_.push(ptr); }

    void pushWork(void* ptr, size_t size) { work_.push(WorkEntry(ptr, size)); }

    WorkEntry popWork() { return work_.pop(); }

private:
    unsigned char* raw_;
    FreeQueue free_;
    WorkQueue work_;
};

} // namespace Threading

#endif
