///**************************************************************************
//* @File: PRIMessage.cpp
//* @Description:  脉冲重复间隔消息类(Pluse Repetition Interval)
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
//*   1.0  2017/03/13    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/
#include <algorithm>        // for std::transform
#include <cmath>
#include <functional>       // for std::bind* and std::mem_fun*

#include <iostream>
#include <limits>

#include <QLoggingCategory>

#include <boost/bind.hpp>

#include "zchxRadarUtils.h"

#include "PRIMessage.h"

using namespace ZCHX;
using namespace ZCHX::Messages;

PRIMessage::RIUInfo::RIUInfo( const RIUInfo &riuinfo )
    : timeStamp( riuinfo.timeStamp )
    , sequenceCounter( riuinfo.sequenceCounter )
    , shaftEncoding( riuinfo.shaftEncoding )
    , prfEncoding( riuinfo.prfEncoding )
    , deviceTime( riuinfo.deviceTime )
    , rangeMin( riuinfo.rangeMin )
    , rangeFactor( riuinfo.rangeFactor )
{
}

void PRIMessage::RIUInfo::load( const QSharedPointer<QByteArray> &raw )
{
    //    cdr >> msgDesc;
    //    cdr >> timeStamp;
    //    cdr >> sequenceCounter;
    //    cdr >> shaftEncoding;
    //    cdr >> prfEncoding;
    //    cdr >> irigTime;
    //    if ( version > 3 )
    //    {
    //        cdr >> rangeMin;
    //        cdr >> rangeFactor;
    //    }
    //    else
    //    {
    //        rangeMin = radarConfig_->GetRangeMin_deprecated();
    //        rangeFactor = radarConfig_->GetRangeFactor_deprecated();
    //    }
}

std::ostream &PRIMessage::RIUInfo::print( std::ostream &os ) const
{
    return os << " TimeStamp: " << timeStamp
           << " Seq#: " << sequenceCounter
           << " Shaft: " << shaftEncoding
           << " deviceTime: " << deviceTime
           << " rangeMin: " << rangeMin
           << " rangeFactor: " << rangeFactor
           ;
}

std::ostream &PRIMessage::RIUInfo::printXML( std::ostream &os ) const
{
    return os << "<riu time=\"" << timeStamp
           << "\" seq=\"" << sequenceCounter
           << "\" shaft=\"" << shaftEncoding
           << "\" prf=\"" << prfEncoding
           << "\" deviceTime=\"" << deviceTime
           << "\" rangeMin=\"" << rangeMin
           << "\" rangeFactor=\"" << rangeFactor
           << "\"/>\n";
}

PRIMessage::PRIMessage( const std::string &producer, const MetaTypeInfo &metaTypeInfo, const RIUInfo &riuInfo )
    : Super( producer, metaTypeInfo, Header::Ref() )
    , riuInfo_( riuInfo )
{
    ;
}

PRIMessage::PRIMessage( const std::string &producer, const MetaTypeInfo &metaTypeInfo, const PRIMessage::Ref &copy )
    : Super( producer, metaTypeInfo, copy ), riuInfo_( copy->getRIUInfo() )
{
    ;
}

PRIMessage::PRIMessage( const std::string &producer, const MetaTypeInfo &metaTypeInfo )
    : Super( producer, metaTypeInfo ), riuInfo_()
{
    ;
}

PRIMessage::PRIMessage( const MetaTypeInfo &metaTypeInfo )
    : Super( metaTypeInfo ), riuInfo_()
{
    ;
}

void
PRIMessage::loadArray( const QSharedPointer<QByteArray> &raw, uint32_t &count )
{
    Super::load( raw );
    com::zhichenhaixin::proto::VideoFrame videoFrame;

    if ( videoFrame.ParseFromArray( raw->data(), raw->size() ) )
    {
        RIUInfo &riuInfo( this->riuInfo_ );
        riuInfo.timeStamp = videoFrame.timeofday();
        riuInfo.sequenceCounter = videoFrame.msgindex();
        //        riuInfo.shaftEncoding = static_cast<uint32_t>(azimuth / (2 * M_PI) * radarConfig_->GetShaftEncodingMax());
        riuInfo.shaftEncoding = videoFrame.azimuth();
        riuInfo.prfEncoding = 0;
        riuInfo.deviceTime = 0.0;
        riuInfo.rangeMin = videoFrame.startrange();
        riuInfo.rangeFactor = videoFrame.rangefactor() / 1000.0; //米换算公里
        count = videoFrame.amplitude_size();
//        qCDebug( radarmsg ) << "azimuth" << riuInfo.shaftEncoding;
    }
    else
    {
        qCWarning( radarmsg ) << "failed to decode protobuf msg ,   error, detail:";
    }
}


double
PRIMessage::getAzimuthStart() const
{
    return radarConfig_->GetAzimuth( riuInfo_.shaftEncoding );
}

double
PRIMessage::getAzimuthEnd() const
{
    return ZchxRadarUtils::normalizeRadians( radarConfig_->GetAzimuth( riuInfo_.shaftEncoding ) + radarConfig_->GetBeamWidth() );
}

std::ostream &
PRIMessage::printHeader( std::ostream &os ) const
{
    return riuInfo_.print( os );
}


std::ostream &
PRIMessage::printDataXML( std::ostream &os ) const
{
    riuInfo_.printXML( os );
    return os;
}


template <typename T> void
GenericPrinterXML( std::ostream &os, const std::vector<T> &data )
{
    size_t count = data.size();
    const T *ptr( count ? &data[0] : 0 );

    while ( count-- )
    {
        os << *ptr++ << ' ';
    }
}

void
Traits::Bool::Reader( const QSharedPointer<QByteArray> &raw, size_t size, std::vector<Type> &data )
{
    if ( size )
    {
        data.resize( size );
        // cdr.read_char_array(&data[0], size);
    }
}

void
Traits::Bool::Printer( std::ostream &os, const std::vector<Type> &data )
{
    size_t count = data.size();
    const Type *ptr( count ? &data[0] : 0 );
    os << "Size: " << count << '\n';

    while ( count )
    {
        for ( int index = 0; index < 60 && count; ++index, --count ) os << int( *ptr++ ) << ' ';

        os << '\n';
    }
}




void
Traits::Bool::PrinterXML( std::ostream &os, const std::vector<Type> &data )
{
    size_t count = data.size();
    const Type *ptr( count ? &data[0] : 0 );

    while ( count-- ) os << int( *ptr++ ? 1 : 0 ) << ' ';
}

void
Traits::Int16::Reader( const QSharedPointer<QByteArray> &raw, size_t size, std::vector<Type> &data )
{
    if ( size )
    {
        //        data.resize(size);
        com::zhichenhaixin::proto::VideoFrame videoFrame;

        if ( videoFrame.ParseFromArray( raw->data(), raw->size() ) )
        {
            for ( int i = 0; i < videoFrame.amplitude_size(); i++ )
            {
                data.push_back( videoFrame.amplitude( i ) );
            }
        }
    }
}

void
Traits::Int16::Printer( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinter<Type, 40>( os, data );
}


void
Traits::Int16::PrinterXML( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinterXML<Type>( os, data );
}

void
Traits::ComplexInt16::Reader( const QSharedPointer<QByteArray> &raw, size_t size, std::vector<Type> &data )
{
    if ( size )
    {
        data.resize( size );
        // cdr.read_short_array(reinterpret_cast<int16_t*>(&data[0]), size * 2);
    }
}

void
Traits::ComplexInt16::Printer( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinter<Type, 40>( os, data );
}

void
Traits::ComplexInt16::PrinterXML( std::ostream &os, const std::vector<Type> &data )
{
    size_t count = data.size();
    const Type *ptr( count ? &data[0] : 0 );

    while ( count-- )
    {
        os << ptr->real() << ',' << ptr->imag() << ' ';
        ++ptr;
    }
}

void
Traits::Int32::Reader( const QSharedPointer<QByteArray> &raw, size_t size, std::vector<Type> &data )
{
    data.reserve( size );
    int32_t value;

    while ( size-- )
    {
        // cdr >> value;
        // data.push_back(value);
    }
}

void
Traits::Int32::Printer( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinter<Type, 20>( os, data );
}

void
Traits::Int32::PrinterXML( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinterXML( os, data );
}

void
Traits::Float::Reader( const QSharedPointer<QByteArray> &raw, size_t size, std::vector<Type> &data )
{
    if ( size )
    {
        data.resize( size );
        // cdr.read_float_array(&data[0], size);
    }
}


void
Traits::Float::Printer( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinter<Type, 20>( os, data );
}

void
Traits::Float::PrinterXML( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinterXML<Type>( os, data );
}

void
Traits::Double::Reader( const QSharedPointer<QByteArray> &raw, size_t size, std::vector<Type> &data )
{
    if ( size )
    {
        data.resize( size );
        // cdr.read_double_array(&data[0], size);
    }
}

void
Traits::Double::Printer( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinter<Type, 10>( os, data );
}


void
Traits::Double::PrinterXML( std::ostream &os, const std::vector<Type> &data )
{
    GenericPrinterXML<Type>( os, data );
}
