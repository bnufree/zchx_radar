///**************************************************************************
//* @File: BinaryVideo.h
//* @Description:  二进制视频接口
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

#ifndef ZCHX_RADAR_MESSAGES_BINARYVIDEO_H // -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_BINARYVIDEO_H

#include "Video.h"

namespace ZCHX {
namespace Messages {



/** Collection of gate values for one PRI message from a radar. The gate values are boolean, representing a
    true/false or pass/fail condition as the result of an algorithm.
*/
class BinaryVideo : public TPRIMessage<Traits::Bool>
{
public:
    using Super = TPRIMessage<Traits::Bool>;
    using Ref = TPRIMessageRef<BinaryVideo>;

    /** Obtain the message type information for BinaryVideo objets

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    /** Factory method that generates a new, empty BinaryVideo message. Reserves (allocates) space for \a count
        values, but the message is initially empty.

        \param producer the name of the task that created the message

        \param vmeHeader the VME parameters that define the message

        \param count the number of values to reserve

        \return new Video message reference
    */
    static Ref Make(const std::string& producer, const RIUInfo& riuInfo, size_t count);


    /** Class factory method used to create a BinaryVideo object from a Video object.

	   \param producer name of the entity that is creating the new object

        \param video video object to use

        \return reference to new BinaryVideo object
    */
    static Ref Make(const std::string& producer, const Video::Ref& video);

    /** Class factory method used to create a BinaryVideo object from another BinaryVideo object.

	   \param producer name of the entity that is creating the new object

        \param video binary video object to use

        \return reference to new BinaryVideo object
    */
    static Ref Make(const std::string& producer, const BinaryVideo::Ref& video);

    /** 

        \param producer 
        
        \param guid 

        \param Header 

        \param count 

        \return 
    */
    static Ref Make(const std::string& producer, const RIUInfo& riuInfo, const DatumType* first,
                    const DatumType* end);

    /** Class factory used to create a new Video object from a byte stream.

        \param  input stream to load from

        \return reference to new Video object
    */
    static Ref Make(const QSharedPointer<QByteArray>& raw);

    ~BinaryVideo();

    /** Obtain the first Video message that forms the basis for this instance.

        \return Video message found, or NULL if none exist
    */
    Video::Ref getVideoBasis() const;

    /** Obtain the BinaryVideo message that was the basis for this instance.

        \return BinaryVideo message or NULL if none exist
    */
    Ref getBinaryBasis() const { return getBasis<BinaryVideo>(); }

private:

    BinaryVideo();

    BinaryVideo(const std::string& producer, const RIUInfo& riuInfo, size_t size);

    BinaryVideo(const std::string& producer, const Video::Ref& basis);

    BinaryVideo(const std::string& producer, const BinaryVideo::Ref& basis);

    BinaryVideo(const std::string& producer);

    /** Load procedure used to create a new BinaryVideo object using data from a raw data stream.

        \param  streamn containing the raw data

        \return reference to new BinaryVideo object
    */
    static Header::Ref CDRLoader(const QSharedPointer<QByteArray>& raw);

    static MetaTypeInfo metaTypeInfo_;
};

inline std::ostream&
operator<<(std::ostream& os, const BinaryVideo::Ref& ref)
{ return ref->print(os); }

} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
