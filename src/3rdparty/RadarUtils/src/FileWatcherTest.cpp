#include <fstream>

#include "Threading/Threading.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "FileWatcher.h"

using namespace Utils;

struct TestFileWatcher : public FileWatcher
{
    TestFileWatcher(double period)
    : FileWatcher(period), loaded_(false) {}

    bool loadFile(const std::string& path) { loaded_ = true; return true; }

    bool loaded_;
};

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("FileWatcher") {}
    void test();
};

void
Test::test()
{
    std::string path("fileWatcherTest.cfg");
    ::unlink(path.c_str());

    {
    TestFileWatcher tfw(1.0);
    assertFalse(tfw.setFilePath(path));
    }

    std::ofstream of(path.c_str());
    of << "blah\n";
    of.close();

    TestFileWatcher tfw(0.1);
    assertTrue(tfw.setFilePath(path));
    assertTrue(tfw.loaded_);

    tfw.loaded_ = false;
    tfw.reload();
    assertTrue(tfw.loaded_);
    tfw.loaded_ = false;

    Threading::Thread::Sleep(1.0);
    of.open(path.c_str(), std::ios_base::out | std::ios_base::app);
    of << "blah\n";
    of.close();

    // Spin until the reload is finished.
    //
    int counter = 5;
    while (tfw.isStale() && --counter)
    Threading::Thread::Sleep(0.5);

    // These should have reverted back to their configured values
    //
    assertTrue(tfw.loaded_);

    ::unlink(path.c_str());
}

int
main(int argc, const char* argv[])
{
    return  Test().mainRun();
}
