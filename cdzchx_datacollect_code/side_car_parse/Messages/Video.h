///**************************************************************************
//* @File: Video.h
//* @Description:  视频接口
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
#ifndef ZCHX_RADAR_MESSAGES_VIDEO_H // -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_VIDEO_H

#include "PRIMessage.h"

namespace ZCHX {
namespace Messages {


/** Collection of sample values for one PRI message from a radar. The sample values are fixed-point, 16-bit
    values (normally treated as integers).
*/
class Video : public TPRIMessage<Traits::Int16>
{
public:
    using Super = TPRIMessage<Traits::Int16>;
    using Ref = TPRIMessageRef<Video>;

    /** Obtain the message type information for Video objects.

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    /** Factory method that generates a new, empty Video message. Reserves (allocates) space for \a count
        values, but the message is initially empty.

        \param producer the name of the task that created the message

        \param vmeHeader the Header parameters that define the message

        \param count the number of values to reserve

        \return new Video message reference
    */
    static Ref Make(const std::string& producer, const RIUInfo& riuInfo, size_t count);

    /** Factory method that generates a new Video message filled with values taken from a pointer.

        \param producer the name of the task that created the message

        \param Header the parameters that define the message

	   \param first pointer to the first value to use

	   \param end pointer to the last+1 value to use

        \return new Video message reference
    */
    static Ref Make(const std::string& producer, const RIUInfo& riuInfo, const DatumType* first,
                    const DatumType* end);

    /** Factory method that generates a new Video message filled with values taken from a std::vector of values.

        \param producer the name of the task that created the message

        \param Header the parameters that define the message

        \param data container of values to copy

        \return 
    */
    static Ref Make(const std::string& producer, const RIUInfo& riuInfo, const Container& data);

    /** Class factory used to create a Video object from another Video object.

	    \param producer name of the entity that is creating the new object

        \param basis message that forms the basis for the new one

        \return reference to new Video object
    */
    static Ref Make(const std::string& producer, const PRIMessage::Ref& basis);
    
    /** Class factory used to create a new Video object from an byte stream.

        \param cdr input stream to load from

        \return reference to new Video object
    */
    static Ref Make(const QSharedPointer<QByteArray>& raw);

    ~Video();

    /** Obtain the Video message that was the basis for this instance.

        \return Video message or NULL if none exist
    */
    Video::Ref getVideoBasis() const { return getBasis<Video>(); }

private:

   
    Video(const std::string& producer, const RIUInfo& riuInfo, size_t size);


    Video(const std::string& producer, const RIUInfo& riuInfo, const Container& data);


    Video(const std::string& producer, const PRIMessage::Ref& basis);

    Video(const std::string& producer);

    /** Default constructor.
     */
    Video();

    /** Class factory used to create a new Video object from an byte stream.

        \param cdr input stream to load from

        \return reference to new Video object
    */
    static Header::Ref CDRLoader(const QSharedPointer<QByteArray>& raw);

    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
