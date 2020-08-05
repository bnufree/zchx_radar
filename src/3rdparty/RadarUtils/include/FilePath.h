//**************************************************************************
//* @File: FilePath.h
//* @Description: 文件路径接口
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
#ifndef FILEPATH_H		// -*- C++ -*-
#define FILEPATH_H

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>		// for time_t
#endif
#include <unistd.h>
#include <cstdio>
#include <iosfwd>
#include <string>

#include <os-win32.h>


#include "Exception.h"
#include "Format.h"

namespace ZchxRadarUtils {

/** Simple wrapper around the std string class that can decompose a file path into three parts: directory path,
    file name, and extension. It will also expand embedded shell variables such as `$FOO' or `${HOME}'.
*/
class ZCHX_API FilePath : public Printable
{
public:

    /** Constructor using C++ string for path

    \param path name and location of file
    */
    FilePath(const std::string& path) : filePath_(path) { expandEnvVars(); }

    /** Constructor using C string for path

    \param path name and location of file
    */
    FilePath(const char* path) : filePath_(path) { expandEnvVars(); }

    /** Create a file path from a directory path and a file name.

        \param dir directory path

        \param file file name

    */
    FilePath(const std::string& dir, const std::string& file);

    /** Destructor. Here to silence GCC warnings.
     */
    virtual ~FilePath() = default;

    /** Returns the directory path component of the file path. If there is no directory component, returns an
    empty string. All non-empty directory components end in a '/' character.
    */
    std::string pathPart() const;

    /** Returns the file name component of the file path. If there is no file component (the file path ends in a
    '/'), returns an empty string.
    */
    std::string filePart() const;

    /** Returns the extension found at the end of the file name. An extension is defined as any characters
    following the last `.' found in the file path. If there are more than one `.' in the path, only the last
    one is treated as the extension.
    */
    std::string extension() const;

    /** Returns true if the instance points to a valid file. This only checks that the file exists; it does not
    indicate whether the user can open the file.

    \return true if the file exists, false otherwise
    */
    bool exists() const;

    /** Change the directory path component to the given value.

    \param path new directory path to assign to the file path.
    */
    void relocate(const std::string& path);

    /** Change the extension of the file path to the given value. The extention text may start with a `.'
    character; if one is not present, the method will add one before using.

    \param ext extension value to use
    */
    void setExtension(const std::string& ext);

    /** Fetch the entire file path.

    \return file path
    */
    const std::string& filePath() const { return filePath_; }

    /** Fetch the entire file path as a C string

    \return NULL-terminated C string
    */
    const char* c_str() const { return filePath_.c_str(); }

    /** Type conversion operator for C++ string.

    \return C++ string
    */
    operator const std::string&() const { return filePath_; }

    /** Type conversion operator into C string.

    \return NULL-terminated C string
    */
    operator const char*() const { return filePath_.c_str(); }

    /** Implementation of Printable interface. Write out full file path to given stream as a string.

    \param os stream to write to

    \return stream written to
    */
    std::ostream& print(std::ostream& os) const;

    bool empty() const { return filePath_.empty(); }

    /** Get the number of characters in the full path.

    \return full path length
    */
    int size() const { return filePath_.size(); }

    /** Get the time of the last modification to this file. File must exist or else an exception is thrown.

    \return modification time
    */
    time_t getModificationTime() const;

private:

    /** Perform inline replacement of `~' and `$' shell variable forms found embedded in the file path.
     */
    void expandEnvVars();

    std::string filePath_;
};				// class FilePath

/** Specialization of FilePath that used the environment variable $AP_CONFIG_DIR to locate configuration files.
    ConfigFilePath first looks for a file in the location given by the FilePath::pathPart() method. If it is not
    there, try using the value of $AP_CONFIG_DIR and looking there.

    NOTE: the 'AP' of AP_CONFIG_DIR stands for "airport". This class was used by the Runway Status Lights
    project (RWSL). See http://rwsl.ll.mit.edu/index.html
*/
class ZCHX_API ConfigFilePath : public FilePath
{
public:

    /** Constructor using C++ string for path.

    \param path name and location of file
    */
    ConfigFilePath(std::string path) : FilePath(path) { locate(); };

    /** Constructor using C string for path.

    \param path name and location of file
    */
    ConfigFilePath(const char* path) : FilePath(path) { locate(); }

private:

    /** Look for the file on the disk. If not found, check again under $AP_CONFIG_DIR.
     */
    void locate();
};


/** Wrapper for a file path that attempts to remove existing files at the given path during construction and
    destruction.
*/
class ZCHX_API TemporaryFilePath
{
public:

    TemporaryFilePath();

    TemporaryFilePath(const std::string& path, bool clear = true);

    ~TemporaryFilePath();

    int getFileDescriptor() const { return fd_; }

    const FilePath& getFilePath() const { return filePath_; }

    const std::string& filePath() const throw() { return filePath_.filePath(); }

    operator const std::string&() const throw() { return filePath_.filePath(); }

    operator const char*() const throw() { return filePath_.filePath().c_str(); }

private:
    FilePath filePath_;
    int fd_;
};

} // namespace ZchxRadarUtils

/** \file
 */

#endif
