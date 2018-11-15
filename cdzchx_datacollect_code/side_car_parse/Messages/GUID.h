///**************************************************************************
//* @File: GUID.h
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

#ifndef ZCHX_RADAR_MESSAGES_GUID_H // -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_GUID_H

#include "MetaTypeInfo.h"

namespace ZCHX {
namespace Messages {


/** Definition of a globally-unique message ID. When a message is first created, it is assigned a GUID value
    that is unique for all hosts and applications running in the zchx system.
*/
class GUID 
{
public:

    GUID();

    GUID(const std::string& producer, const MetaTypeInfo& typeInfo);

    GUID(const std::string& producer, const MetaTypeInfo& typeInfo, MetaTypeInfo::SequenceType sequenceNumber);

    ~GUID() {}

    const std::string& getProducerName() const { return producer_; }

    MetaTypeInfo::Value getMessageTypeKey() const { return messageTypeKey_; }

    MetaTypeInfo::SequenceType getMessageSequenceNumber() const { return messageSequenceNumber_; }

    void setMessageSequenceNumber(MetaTypeInfo::SequenceType seq_num) { messageSequenceNumber_ = seq_num; }

    const std::string& getSequenceKey() const;

    const std::string& getRepresentation() const;

    void load(const std::string& producer, const MetaTypeInfo& typeInfo,
           MetaTypeInfo::SequenceType sequenceNumber=0);

    /** Write out the header values to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const;

private:

    GUID(const GUID& rhs);

    GUID& operator=(const GUID& rhs);

    std::string producer_;
    MetaTypeInfo::Value messageTypeKey_;
    MetaTypeInfo::SequenceType messageSequenceNumber_;
    mutable std::string sequenceKey_;
    mutable std::string representation_;
};

} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
