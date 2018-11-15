//**************************************************************************
//* @File: Format.cpp
//* @Description: 格式化类
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
#include <iostream>
#include <iomanip>

#include "Format.h"


using namespace ZchxRadarUtils;

namespace {

/** Collection of useful formats, made available at the start of the program.
 */
struct Formatters
{
    ZchxRadarUtils::Format hexPtr;	///< Formatter for pointer addresses
    ZchxRadarUtils::Format heading;	///< Formatter for headings
    ZchxRadarUtils::Format time;		///< Formatter for timestamps

    /** Class method that returns the single instance.

        \return reference to singleton
    */
    static Formatters& Singleton();

private:

    /** Constuctor. Initialize all Format objects.
     */
    Formatters();
};

Formatters::Formatters()
    : hexPtr(sizeof(void*) << 1, 0), heading(3, 0), time(8, 1)
{
    hexPtr.fill('0').hex().uppercase();
    heading.fill('0');
    time.showpoint().fixed();
}

Formatters&
Formatters::Singleton()
{
    static Formatters singleton_;
    return singleton_;
}
 
} // NULL namespace

void
ZchxRadarUtils::Format::put(std::ios& s) const
{
    // Install our settings in the stream object
    //
    s.precision(precision_);
    s.width(width_);
    s.fill(fill_);
    s.flags(flags_);
}

std::ostream&
hexPtr::print(std::ostream& os) const
{
    // I believe std::ostream's formatting for a void* is to show it in hex, with lowercase alpha characters.
    // This blows chunks when we try to override the settings to hex uppercase alpha. Convert to another type to
    // stop the insanity. This is very fragile and will probably break on another architecture. C'est la vie...
    //
    return os << "0x" << Formatters::Singleton().hexPtr(ptr_);
}

std::ostream&
formatTime::print(std::ostream& os) const
{
    return os << Formatters::Singleton().time(when_);
}

std::ostream&
center::print(std::ostream& os) const
{
    // Save current stream format, but don't apply our format. We just use the field width of the format.
    //
    PushFormat saved(os);

    // If too wide, just write out the entire data element.
    //
    if (static_cast<int>(data_.size()) >= width_) return os << data_;

    // How many spaces do we add to the left?
    //
    auto skip = (width_ - data_.size()) / 2;
    if (skip) os << std::setw(skip) << ' ';

    // Write out text
    //
    os << std::setw(0) << data_;

    // How many spaces do we add to the write?
    //
    skip = width_ - data_.size() - skip;
    if (skip) os << std::setw(skip) << ' ';
    return os;
}

std::ostream&
dashLine::print(std::ostream& os) const
{
    // Save current stream format, setup stream to fill with the dash character for our field width, and then
    // write out one dash character. When we exit, the stream is put back in its previous state.
    //
    PushFormat saved(os, format_);
    return os << format_.fill();
}

std::ostream&
dumpHex::print(std::ostream& os) const
{
    auto ptr = static_cast<const unsigned char*>(data_);
    for (auto count = size_; count; --count, ++ptr) {
	os << digit(*ptr >> 4) << digit(*ptr) << ' ';
    }
    return os << std::endl;
}
