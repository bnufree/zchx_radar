//**************************************************************************
//* @File: FilePathTest.cpp
//* @Description: 文件路径测试类
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
#include "UnitTest/UnitTest.h"
#include "FilePath.h"

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("FilePath") {}

    void test();
};

void
Test::test()
{
    Utils::FilePath fp1("/one/two/three.four");
    assertFalse(fp1.exists());
    assertEqual("four", fp1.extension());
    assertEqual("three.four", fp1.filePart());
    assertEqual("/one/two/", fp1.pathPart());

    Utils::FilePath fp2("one.");
    assertFalse(fp2.exists());
    assertEqual("", fp2.extension());
    assertEqual("one.", fp2.filePart());
    assertEqual("", fp2.pathPart());
    fp2.setExtension("blah");
    assertEqual("one.blah", fp2.filePart());
    fp2.relocate("/one/two");
    assertEqual("/one/two/one.blah", fp2);

    Utils::FilePath fp3("/one/two", "three.four");
    assertEqual(fp1, fp3);

    std::string keep("");
    {
	// Create a unique temporary file.
	//
	Utils::TemporaryFilePath fp4;
	keep = fp4.getFilePath().filePath();

	// The TemporaryFilePath class uses ::mkstemp to generate unique file names. It gives ::mkstemp a
	// template of '/tmp/tmp.XXXXXX' Verify that the path starts with '/tmp/tmp.'
	//
	assertEqual(size_t(0), keep.find("/tmp/TemporaryFilePath."));

	// Write out some data, close the file, before TemporaryFilePath goes out of scope and the file
	// disappears.
	//
	int fd = fp4.getFileDescriptor();
	assertEqual(5, ::write(fd, "1234", 5));
	::close(fd);
    }

    // Verify that the temporary file no longer exists.
    //
    assertEqual(-1, ::unlink(keep.c_str()));
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
