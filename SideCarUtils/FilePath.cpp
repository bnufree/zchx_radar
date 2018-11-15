//**************************************************************************
//* @File: FilePath.cpp
//* @Description: 文件路径类
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
#include <sys/stat.h>		// for ::stat()
#include <cstdlib>		// for ::getenv()
#include <iostream>
#include <unistd.h>		// for ::unlink()

//#include "Utils/Exception.h"
#include "Exception.h"
#include "FilePath.h"

using namespace ZchxRadarUtils;

FilePath::FilePath(const std::string& dir, const std::string& file)
    : filePath_(dir)
{
    if (dir.size() == 0 || dir.rfind('/') != dir.size() - 1) {
	filePath_ += '/';
    }
    filePath_ += file;
}

std::string
FilePath::pathPart() const
{
    auto pos = filePath_.rfind('/');
    return pos == std::string::npos ? std::string("") : filePath_.substr(0, pos + 1);
}

std::string
FilePath::filePart() const
{
    auto pos = filePath_.rfind('/');
    return pos == std::string::npos ? filePath_ : filePath_.substr(pos + 1, std::string::npos);
}

std::string
FilePath::extension() const
{
    auto fp = filePart();
    auto pos = fp.rfind('.');
    return pos == std::string::npos ? std::string("") : fp.substr(pos + 1, std::string::npos);
}

bool
FilePath::exists() const
{
    struct stat status;
    return (::stat(filePath_.c_str(), &status) == 0 && ((status.st_mode & S_IFMT) == S_IFREG ||
                                                        (status.st_mode & S_IFMT) == S_IFCHR ||
                                                        (status.st_mode & S_IFMT) == S_IFDIR));
}

void
FilePath::relocate(const std::string& path)
{
    std::string newPath(path);
    if (newPath.size() && newPath[newPath.size() - 1] != '/') {
	newPath += '/';
    }
    filePath_ = newPath + filePart();
    expandEnvVars();
}

void
FilePath::setExtension(const std::string& ext)
{
    // Search for an extension, but don't treat any '.' found before a '/' character.
    //
    auto dotPos = filePath_.rfind('.');
    auto slashPos = filePath_.rfind('/');
    if (slashPos != std::string::npos && dotPos != std::string::npos && dotPos < slashPos) {
	dotPos = std::string::npos;
    }
    
    if (ext.size() == 0) {

	// Removing any existing extension.
	//
	if (dotPos == std::string::npos) return;
	filePath_.erase(dotPos);
    }
    else {
	std::string newExt(ext);
	
	// Adding/replacing extension
	//
	if (newExt[0] != '.') newExt.insert(0, ".");
	if (dotPos == std::string::npos) {
	    filePath_ += newExt;
	}
	else {
	    filePath_.replace(dotPos, std::string::npos, newExt);
	}
    }
}

std::ostream&
FilePath::print(std::ostream& os) const
{
    return os << filePath_;
}

void
FilePath::expandEnvVars()
{
    auto pos = 0;
    do {
	pos = filePath_.find_first_of("$~", pos, 2);
	if (pos == std::string::npos) break;

	auto name = pos;
	std::string var;
	const char* value = nullptr;

	// Treat '~' as equivalent of $HOME.
	//
	if (filePath_[pos++] == '~') {
	    var = "HOME";
	}
	else {

	    // Find location of first character that does not belong to a shell variable name.
	    //
	    while (pos < filePath_.size()) {
		auto c = filePath_[pos];
		if (c == '_' || c == '{' || c == '}' || isalnum(c)) {
		    ++pos;
		    if (c != '}') continue;
		}
		break;
	    }

	    // Extract the name of the environment variable to fetch. Properly account for optional '{' '}'
	    // characters.
	    //
	    if (filePath_[name + 1] == '{') {
		var = filePath_.substr(name + 2, pos - name - 3);
	    }
	    else {
		var = filePath_.substr(name + 1, pos - name - 1);
	    }
	}

	// Fetch the value of the variable and if OK, replace the variable name with its value.
	//
	value = ::getenv(var.c_str());
	if (! value) {
	    std::cerr << "FilePath::expandEnvVars: environment variable '" << var << "' does not exist\n";
	    value = "";
	}

	filePath_.replace(name, pos - name, value);

	// Start the next search near the beginning of the previous match so we can expand variables in previous
	// expansions. Note that we do not handle embedded epansions like ${FOO_${BAR}}. C'est la vie.
	//
	pos = name + 1;

    } while (1);
}

time_t
FilePath::getModificationTime() const
{
    struct stat st;
    auto rc = ::stat(filePath_.c_str(), &st);
    if (rc == -1) {
    ZchxRadarUtils::Exception ex("FilePath::getModificationTime: ");
	ex << "unable to locate file " << filePath_;
	throw ex;
    }

    return st.st_mtime;
}

void
ConfigFilePath::locate()
{
    if (! exists()) {
	std::cerr << "ConfigFilePath: file '" << filePath() << "' does not exist -- looking in $AP_CONFIG_DIR\n";
	relocate("$AP_CONFIG_DIR/");
	if (! exists()) {
	    std::cerr << "ConfigFilePath: file '" << filePath() << "' does not exist\n";
	}
    }
}

TemporaryFilePath::TemporaryFilePath()
    : filePath_(""), fd_(-1)
{
    char buffer[] = "/tmp/TemporaryFilePath.XXXXXXXXXX";
    fd_ = ::mkstemp(buffer);
    filePath_ = FilePath(buffer);
}

TemporaryFilePath::TemporaryFilePath(const std::string& path, bool clear)
    : filePath_(path), fd_(-1)
{
    if (clear) ::unlink(filePath_.c_str());
}

TemporaryFilePath::~TemporaryFilePath()
{
    ::unlink(filePath_.c_str());
    if (fd_ != -1) ::close(fd_);
}
