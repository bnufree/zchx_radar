///**************************************************************************
//* @File: Header.cpp
//* @Description:  消息基类
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
//*   1.0  2017/03/30    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/
#include <QLoggingCategory>

#include "Header.h"
#include "MetaTypeInfo.h"

using namespace ZCHX;
using namespace ZCHX::Messages;

Q_LOGGING_CATEGORY( radarmsg, "zchx.radar.messages" )

const MetaTypeInfo *
Header::GetMessageMetaTypeInfo( const QSharedPointer<QByteArray> &raw )
{
    // Create a temporary Header object that we only use to obtain the message type key found in the GUID. When
    // we have that, use it to obtain the actual MetaTypeInfo object.
    Header tmp( raw );
    return MetaTypeInfo::Find( tmp.guid_.getMessageTypeKey() );
}


Header::Header( const QSharedPointer<QByteArray> &raw )
    : metaTypeInfo_( *MetaTypeInfo::Find( MetaTypeInfo::Value::kInvalid ) )
    , guid_()
    , createdTimeStamp_()
    , basis_()
{
    qCDebug( radarmsg ) << "Header(0)" ;
    load( raw );
}

RadarConfig *Header::getRadarConfig() const
{
    return radarConfig_;
}

void Header::setRadarConfig(RadarConfig *radarConfig)
{
    radarConfig_ = radarConfig;
}

Header::Header( const std::string &producer, const MetaTypeInfo &metaTypeInfo )
    : metaTypeInfo_( metaTypeInfo )
    , guid_( producer, metaTypeInfo )
    , createdTimeStamp_( QDateTime::currentDateTime() ),
      basis_()
{
}

Header::Header( const std::string &producer, const MetaTypeInfo &metaTypeInfo, const Ref &basis )
    : metaTypeInfo_( metaTypeInfo )
    , guid_( producer, metaTypeInfo )
    , createdTimeStamp_( QDateTime::currentDateTime() )
    , basis_( basis )
{
}

Header::Header( const std::string &producer, const MetaTypeInfo &metaTypeInfo, const Ref &basis,
                MetaTypeInfo::SequenceType sequenceNumber )
    : metaTypeInfo_( metaTypeInfo )
    , guid_( producer, metaTypeInfo, sequenceNumber )
    , createdTimeStamp_( QDateTime::currentDateTime() )
    , basis_( basis )
{
}

Header::Header(const MetaTypeInfo &metaTypeInfo )
    : metaTypeInfo_( metaTypeInfo )
    , guid_()
    , createdTimeStamp_()
    , basis_()
    , radarConfig_(0)
{
}

Header::~Header()
{
    ;
}

void
Header::setCreatedTimeStamp( const TimeStamp &timeStamp )
{
    createdTimeStamp_ = timeStamp;
}


void Header::load( const QSharedPointer<QByteArray> &raw )
{
    //    const Descriptor* descriptor = msg->GetDescriptor();
    //    const Reflection* reflection = msg->GetReflection();
    //    const FieldDescriptor* utc_field = descriptor->FindFieldByName("UTC");
    //    if (utc_field != NULL)
    //    {
    //       uint64 UTC = reflection->Getuint64(*msg, utc_field);
    //       static boost::posix_time::ptime const time_epoch(boost::gregorian::date(1970, 1, 1));//一个基准点。
    //       createdTimeStamp_ = time_epoch + boost::posix_time::milliseconds(UTC);
    //    }else
    //    {
    //        createdTimeStamp_ = boost::posix_time::microsec_clock::local_time();
    //    }
}

std::ostream &
Header::printHeader( std::ostream &os ) const
{
    return os << "GUID: " << guid_.getRepresentation()
           << " CTime: " << createdTimeStamp_.msec();
}

std::ostream &
Header::printData( std::ostream &os ) const
{
    return os << "*** printData undefined ***";
}

std::ostream &
Header::printDataXML( std::ostream &os ) const
{
    return os << "*** printDataXML undefined ***";
}

std::ostream &
Header::print( std::ostream &os ) const
{
    printHeader( os ) << '\n';
    printData( os ) << '\n';
    return os;
}

std::ostream &
Header::printXML( std::ostream &os ) const
{
    os << "<msg id=\"" << guid_.getMessageSequenceNumber()
       << "\" ctime=\"" << createdTimeStamp_.msec()
       << "\">\n";
    printDataXML( os );
    return os << "</msg>";
}

