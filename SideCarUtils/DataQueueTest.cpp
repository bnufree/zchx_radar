//**************************************************************************
//* @File: DataQueueTest.cpp
//* @Description: 数据队列测试类
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
#include <numeric>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "DataQueue.h"
#include "UnitTest/UnitTest.h"
#include "Utils/Utils.h"

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("DataQueue"), queue_(29) {}

    void test();

    using DataQueue = Threading::TDataQueue<std::string>;

    struct Writer : public Threading::Thread
    {
	Writer(DataQueue& queue) : Threading::Thread(), queue_(queue) {}
	virtual void run();
	DataQueue& queue_;
    };

    struct Reader : public Threading::Thread
    {
	Reader(DataQueue& queue)
	    : Threading::Thread(), queue_(queue), fetched_(0) {}
	virtual void run();
	size_t fetched() const { return fetched_; }
	DataQueue& queue_;
	size_t fetched_;
    };

    DataQueue queue_;
};

void
Test::Writer::run()
{
    for (int index = 0; index < 1000; ++index) {
	queue_.push(std::string(10, '0' + (index % 10)));
    }
}

void
Test::Reader::run()
{
    while (isRunning()) {
	std::string blah(queue_.pop());
	++fetched_;
	Sleep(::drand48() / 10.0);
    }
}

static inline int
summer(int total, Test::Reader* reader) 
{
    return total + reader->fetched();
}

void
Test::test()
{
    std::vector<Reader*> readers;
    while (readers.size() < 13) {
	Reader* reader = new Reader(queue_);
	reader->start();
	readers.push_back(reader);
    }

    Writer writer(queue_);
    writer.start();
    writer.waitToFinish();
    while (! queue_.empty())
	;

    int total = std::accumulate(readers.begin(), readers.end(), 0, &summer);
    assertEqual(1000, total);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
