///**************************************************************************
//* @File: Header.h
//* @Description:  消息基类接口 所有消息继承此消息基类
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

#ifndef ZCHX_RADAR_MESSAGES_HEADER_H // -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_HEADER_H

#include <string>

#include <QString>
#include <QSharedPointer>
#include <boost/shared_ptr.hpp>
#include <google/protobuf/message.h>
#include "GUID.h"
#include "MessagesGlobal.h"

class QDomElement;

namespace ZCHX {
namespace Messages {

class RadarConfig;
class MetaTypeInfo;

/** Definition of the header found at the beginning of all messages, regardless of content type. All messages
    contain a timestmp indicating when they were created, a timestamp when they were written out to a file or
    the network, and an unique sequence number.
*/
class Header
{
public:
    using Ref = boost::shared_ptr<Header>;

    static const MetaTypeInfo* GetMessageMetaTypeInfo(const QSharedPointer<QByteArray>& raw);

    /** Constructor for messages read in from an external device.

        \param metaTypeInfo type specification for the message
    */
    explicit Header(const MetaTypeInfo& metaTypeInfo);

    /** Constructor for new messages

        \param producer name of the entity that created the message

        \param metaTypeInfo type specification for the message

    */
    Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo);

    /** Constructor for new messages

        \param producer name of the entity that created the message

        \param metaTypeInfo type specification for the message

        \param the globally-unique ID of the message
    */
    Header(const std::string& producer,  const MetaTypeInfo& metaTypeInfo, const Ref& basis);

    /** Constructor for new messages

        \param producer name of the entity that created the message

        \param metaTypeInfo type specification for the message
    */
    Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const Ref& basis,
           MetaTypeInfo::SequenceType sequenceNumber);

    /** Destructor. Made virtual so that derived classes will properly clean up our instance values
    (specifically guid_) destructed thru a Header pointer.
    */
    virtual ~Header();

    /** Obtain the type specification for the message.

        \return
    */
    const MetaTypeInfo& getMetaTypeInfo() const { return metaTypeInfo_; }

    /** Get the globally-unique ID of the message. A GUID contains a unique sequence number given to it by the
        metaTypeInfo_ attribute, accessed by the GUID.getMessageSequenceNumber().

        \return unique ID
    */
    const GUID& getGloballyUniqueID() const { return guid_; }

    /** Get the message sequence number for the message. This is the sequence number from the GUID. Note that
        alone this is not unique among all SideCar messages; it is only unique within a message type.

        \return sequence number
    */
    MetaTypeInfo::SequenceType getMessageSequenceNumber() const { return guid_.getMessageSequenceNumber(); }

    /** Set the sequence number for this message. Not to be used lightly. Particularly useful when an algorithm
        generates multiple output messages that should be synchronized by their sequence number.

    NOTE: a better approach than this would be to have unique message sequence generators, which is
        available right now since there is a unique sequence generator associated with each unique producer
        name.

        \param msn new sequence number to use
    */
    void setMessageSequenceNumber(MetaTypeInfo::SequenceType msn) { guid_.setMessageSequenceNumber(msn); }

    /** Obtain when the message was created.

        \return time stamp reference
    */
    const TimeStamp& getCreatedTimeStamp() const { return createdTimeStamp_; }

    /** Revise the creation message timestamp. Used by some zchx utilities that wish to set/adjust the
        message frequency, which is determined by this time stamp.

        \param timeStamp new time stamp value
    */
    void setCreatedTimeStamp(const TimeStamp& timeStamp);


    /** Obtain the C++ structure size for this object. Derived classes must override if they extend Header with
        additional members.

        \return size of a Header object.
    */
    virtual size_t getSize() const { return sizeof(Header); }

    /** Obtain new instance data from a CDR input stream

        \param cdr stream to read from

        \return stream read from
    */
    virtual void load(const QSharedPointer<QByteArray>& raw);


    /** Utility functor that inserts a textual representation of a Header's header data into a std::ostream
    object. Example of it use: \code LOGDEBUG << msg.headerPrinter() << std::endl; \endcode Relies on
    Header::printHeader() to do the actual conversion of header information to text, which derived classes
    may override.
    */
    struct HeaderPrinter
    {
    const Header& ref_;
    HeaderPrinter(const Header& ref) : ref_(ref) {}
    friend std::ostream& operator<<(std::ostream& os, const HeaderPrinter& us)
            { return us.ref_.printHeader(os); }
    };

    /** Convenience method that simply returns a new HeaderPrinter functor for this header object.

        \return HeaderPrinter object
    */
    HeaderPrinter headerPrinter() const { return HeaderPrinter(*this); }

    /** Utility functor that inserts a textual representation of message data into a std::ostream object.
    Example of it use: \code LOGDEBUG << msg.dataPrinter() << std::endl; \endcode Relies on
    Header::printData() to do the actual conversion of data to text, which derived classes may override.
    */
    struct DataPrinter
    {
    const Header& ref_;
    DataPrinter(const Header& ref) : ref_(ref) {}
    friend std::ostream& operator<<(std::ostream& os, const DataPrinter& us)
            { return us.ref_.printData(os); }
    };

    /** Convenience method that simply returns a new DataPrinter functor for this header object.

        \return DataPrinter object
    */
    DataPrinter dataPrinter() const { return DataPrinter(*this); }

    /** Utility functor that inserts an XML representation of message data into a std::ostream object. Example
    of it use: \code LOGDEBUG << msg.xmlPrinter() << std::endl; \endcode Relies on Header::printData() to do
    the actual conversion of data to text, which derived classes may override.
    */
    struct XMLPrinter
    {
    const Header& ref_;
    XMLPrinter(const Header& ref) : ref_(ref) {}
    friend std::ostream& operator<<(std::ostream& os, const XMLPrinter& us)
            { return us.ref_.printXML(os); }
    };

    /** Convenience method that simply returns a new XMLPrinter functor for this header object.

        \return XMLPrinter object
    */
    XMLPrinter xmlPrinter() const { return XMLPrinter(*this); }

    /** Write out the entire message to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& print(std::ostream& os) const;

    /** Write out the message header to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printHeader(std::ostream& os) const;

    /** Write out the message data to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printData(std::ostream& os) const;

    /** Write out the message to a C++ text output stream in XML format. Note: invokes printDataXML() to write
        the data elements of the message.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printXML(std::ostream& os) const;

    /** Write out the message data to a C++ text output stream in XML format.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printDataXML(std::ostream& os) const;

    /** Determine if the message has a basis message attached to it.

        \return true if so
    */
    bool hasBasis() const { return basis_.get() != 0; }

    /** Obtain a reference to the message that this one was based on. NOTE: currently, this is only valid within
        a processing stream; in orther words, once a message is written to disk or the network, the message
        loses its basis value.

    \return message reference
    */
    Ref getBasis() const { return basis_; }

    /** Templated version of the above method. Returns a typed reference to the basis message. Returns an
        invalid reference if the type-cast is invalid.

        \return typed message reference
    */
    template <typename T>
    typename T::Ref getBasis() const { return boost::dynamic_pointer_cast<T>(basis_); }

    RadarConfig *getRadarConfig() const;
    void setRadarConfig(RadarConfig *radarConfig);

private:

    /** Constructor restricted to use for the GetMessageMetaTypeInfo() class method.

        \param guid
    */
    Header(const QSharedPointer<QByteArray>& raw);

    /** Prohibit copy construction.

        \param rhs copy to use
    */
    Header(const Header& rhs);

    const MetaTypeInfo& metaTypeInfo_; ///< Meta type info for this object
    GUID guid_;                ///< Globally-unique ID for this object
    TimeStamp createdTimeStamp_; ///< When created
    Ref basis_;			       ///< Msg that is the basis for this one

protected:
    RadarConfig *radarConfig_;
};

/** Convenience operator overload that inserts a textual representation of a message's header and data into a
    std::ostream object. Example of its use: \code LOGDEBUG << msg << std::endl; \endcode Relies on
    Header::print() to do the actual conversion of data to text, which derived classes may override. To just
    show header or data, use the output stream inserters printHeader or printData.

    \param os stream to write to

    \param ref Header reference to print

    \return stream written to
*/
inline std::ostream&
operator<<(std::ostream& os, const Header::Ref& ref)
{ return ref->print(os); }


} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
