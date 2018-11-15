//**************************************************************************
//* @File: ThreadingTests.cpp
//* @Description: 线程测试类
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

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <pthread.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "Threading.h"
#include "UnitTest/UnitTest.h"

struct Tests : public UnitTest::TestObj
{
    void testGuard();
    void testMutex();
    void testThreads();
    void testJoin();
    void testCancel();

    static void Install(UnitTest::ProcSuite<Tests>& suite)
	{
	    suite.add("guard", &Tests::testGuard);
	    suite.add("mutex", &Tests::testMutex);
	    suite.add("threads", &Tests::testThreads);
	    suite.add("join", &Tests::testJoin);
	    // suite.add("cancel", &Tests::testCancel);
	}
};

struct WriteThread : public Threading::Thread
{
    WriteThread(std::ostream& os, const char* tag)
	: Threading::Thread(), os_(os), tag_(tag) { ++alive_; }

    virtual void run();

    std::ostream& os_;
    const char* tag_;

    static int alive_;
    static int dead_;
    static Threading::Condition::Ref condition_;
};

int WriteThread::alive_ = 0;
int WriteThread::dead_ = 0;
Threading::Condition::Ref WriteThread::condition_;

void
WriteThread::run()
{
    // Randomize the time before we try for the master lock.
    //
    Sleep(::drand48());
    {
	Threading::Guard guard;
	std::cerr << tag_ << '\n';
	os_ << tag_;

	// Randomize the time we hold the lock.
	//
	Sleep(::drand48());
    }

    Threading::Locker lock(condition_);
    ++dead_;
    condition_->broadcast();
}

void
Tests::testGuard()
{
    WriteThread::condition_ = Threading::Condition::Make();

    std::ostringstream os("");
    WriteThread a(os, "A"); a.start();
    WriteThread b(os, "B"); b.start();
    WriteThread c(os, "C"); c.start();
    WriteThread d(os, "D"); d.start();
    WriteThread e(os, "E"); e.start();
    WriteThread f(os, "F"); f.start();
    WriteThread g(os, "G"); g.start();

    Threading::Locker lock(WriteThread::condition_);
    while (WriteThread::alive_ != WriteThread::dead_)
	WriteThread::condition_->waitForSignal();

    std::string output(os.str());
    assertEqual(7, int(output.size()));
}

void
Tests::testMutex()
{
    Threading::Mutex::Ref a(Threading::Mutex::Make());
    a->lock();
    a->unlock();
    assertEqual(true, a->tryToLock());
    assertEqual(false, a->tryToLock());
    a->unlock();
}

struct Thready : public Threading::Thread
{
    Thready();
    virtual void run();
    static Threading::Condition::Ref condition_;
    static int alive_;
    static int dead_;
};

Threading::Condition::Ref Thready::condition_ = Threading::Condition::Make();
int Thready::alive_ = 0;
int Thready::dead_ = 0;

Thready::Thready() : Threading::Thread()
{
    ++Thready::alive_;
}

void
Thready::run()
{
    Threading::Locker lock(Thready::condition_);
    ++Thready::dead_;
    Thready::condition_->signal();
}

void
Tests::testThreads()
{
    // Keep threads from doing anything until all of them are running
    //
    Threading::Locker lock(Thready::condition_);
    Thready a; a.start();
    assertEqual(true, a.isRunning());
    Thready b; b.start();
    assertEqual(true, b.isRunning());
    Thready c; c.start();
    Thready d; d.start();
    Thready e; e.start();
    assertEqual(true, a.isRunning());
    assertEqual(true, b.isRunning());
    assertEqual(true, c.isRunning());
    assertEqual(true, d.isRunning());
    assertEqual(true, e.isRunning());

    assertEqual(5, Thready::alive_);
    assertEqual(0, Thready::dead_);
    assertEqual(false, a == b);

    while (Thready::alive_ != Thready::dead_)
	Thready::condition_->waitForSignal();

    a.join();
    b.join();
    c.join();
    d.join();
    e.join();
}

struct NullThread : public Threading::Thread
{
    static int counter_;
    virtual void run();
};

void
NullThread::run()
{
    ++counter_;
}

int NullThread::counter_ = 0;

struct AbortThread : public Threading::Thread
{
    static int counter_;
    virtual void run();
};

void
AbortThread::run()
{
    throw "abort thread";
    ++counter_;
}

int AbortThread::counter_ = 0;

void
Tests::testJoin()
{
    NullThread a;
    assertEqual(false, a.isRunning());
    assertEqual(false, a.join());
    a.start();
    while (a.isRunning()) ;

    assertEqual(true, a.join());
    assertEqual(false, a.isRunning());

    NullThread b;
    assertEqual(false, b.isRunning());
    assertEqual(false, b.join());
    b.start();
    while (b.isRunning()) ;

    assertEqual(true, b.join());
    assertEqual(2, NullThread::counter_);

    AbortThread c;
    assertEqual(false, c.isRunning());
    assertEqual(false, b.join());
    c.start();
    while (b.isRunning()) ;
    assertEqual(true, c.join());
    assertEqual(0, AbortThread::counter_);
}

struct SpinThread : public Threading::Thread
{
    SpinThread() : Threading::Thread() { start(); }
    virtual ~SpinThread();
    virtual void run();

    static int alive_;
    static Threading::Condition::Ref condition_;
};

int SpinThread::alive_ = 0;
Threading::Condition::Ref SpinThread::condition_ =
    Threading::Condition::Make();

SpinThread::~SpinThread()
{
    Threading::Locker lock(SpinThread::condition_);
    --alive_;
}

void
SpinThread::run()
{
    {
	Threading::Locker lock(SpinThread::condition_);
	++alive_;
    }

    while (isRunning()) {
	pthread_testcancel();
    }
}

void
Tests::testCancel()
{
    {
	SpinThread a;
	SpinThread b;
	SpinThread c;

	assertEqual(true, a.isRunning());
	assertEqual(true, b.isRunning());
	assertEqual(true, c.isRunning());

	// Make sure we have three running threads accounted for.
	//
	Threading::Locker lock(SpinThread::condition_);
	while (SpinThread::alive_ != 3)
	    SpinThread::condition_->waitForSignal();

	// Cancel the first thread, and clean it up by using its join method.
	//
	a.cancel();
	assertEqual(true, a.join());
	assertEqual(false, a.join());

	// Now exit scope, which should cause the other two threads to exit and detach via the ~Thread() method.
	// Since there is no (easy) way to detect that all of this has happened properly, check the trace output
	// to see that the thread destructors were properly called.
	//
    }

    // Wait until all threads are done.
    //
    Threading::Locker lock(SpinThread::condition_);
    while (SpinThread::alive_ != 0)
	SpinThread::condition_->waitForSignal();

    // There should be no thread trace output after this statement.
    //
}

int
main(int argc, const char* argv[])
{
    UnitTest::ProcSuite<Tests> ps("Threading");
    Tests::Install(ps);
    return ps.mainRun();
}
