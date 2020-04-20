///**************************************************************************
//* @File: GGUID.cpp
//* @Description:  唯一标识符接口 Globally-unique ID for this object
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
//*   1.0  2017/04/01    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#include <sstream>
#include <unistd.h>

#include <QLoggingCategory>

#include "GGUID.h"

using namespace ZCHX::Messages;

GGUID::GGUID()
    : producer_( "" ), messageTypeKey_( MetaTypeInfo::Value::kInvalid ), messageSequenceNumber_( 0 ),
      representation_( "" )
{
//    qCDebug( radarmsg ) << "messageSequenceNumber: " << messageSequenceNumber_ ;
}

GGUID::GGUID( const std::string &producer, const MetaTypeInfo &metaTypeInfo )
    : producer_( producer ), messageTypeKey_( metaTypeInfo.getKey() ),
      messageSequenceNumber_( metaTypeInfo.getNextSequenceNumber() ), representation_( "" )
{
//    qCDebug( radarmsg ) << "messageSequenceNumber: " << messageSequenceNumber_ ;
}

GGUID::GGUID( const std::string &producer, const MetaTypeInfo &metaTypeInfo,
            MetaTypeInfo::SequenceType sequenceNumber )
    : producer_( producer ), messageTypeKey_( metaTypeInfo.getKey() ), messageSequenceNumber_( sequenceNumber ),
      representation_( "" )
{
//    qCDebug( radarmsg ) << "messageSequenceNumber: " << messageSequenceNumber_ ;
}

void GGUID::load( const std::string &producer, const MetaTypeInfo &typeInfo,
                 MetaTypeInfo::SequenceType sequenceNumber )
{
    producer_ = producer ;
    messageTypeKey_ = typeInfo.getKey();
    messageSequenceNumber_ = sequenceNumber;
    representation_ = "";
}

const std::string &
GGUID::getSequenceKey() const
{
    if ( ! sequenceKey_.size() )
    {
        std::ostringstream os;
        os << producer_ << '/' << messageTypeKey_;
        sequenceKey_ = os.str();
    }

    return sequenceKey_;
}

const std::string &
GGUID::getRepresentation() const
{
    if ( ! representation_.size() )
    {
        std::ostringstream os;
        os << producer_ << '/' << messageTypeKey_;
        sequenceKey_ = os.str();
        os << '/' << messageSequenceNumber_;
        representation_ = os.str();
    }

    return representation_;
}

std::ostream &
GGUID::print( std::ostream &os ) const
{
    return os << getRepresentation();
}
