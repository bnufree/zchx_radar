///**************************************************************************
//* @File: BinaryVideo.cpp
//* @Description:  二进制视频类
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
#include <QLoggingCategory>

#include "BinaryVideo.h"

using namespace ZCHX::Messages;

MetaTypeInfo BinaryVideo::metaTypeInfo_(MetaTypeInfo::Value::kBinaryVideo, "BinaryVideo",
                                        &BinaryVideo::CDRLoader);

const MetaTypeInfo&
BinaryVideo::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer, const RIUInfo& riuInfo, size_t count)
{
    Ref ref(new BinaryVideo(producer, riuInfo, count));
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer, const Video::Ref& video)
{
    Ref ref(new BinaryVideo(producer, video));
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer, const BinaryVideo::Ref& binaryVideo)
{
    Ref ref(new BinaryVideo(producer, binaryVideo));
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(const QSharedPointer<QByteArray>& raw)
{
    Ref ref(new BinaryVideo);
    ref->load(raw);
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer,  const RIUInfo& riuInfo, const DatumType* first,
                  const DatumType* end)
{
    Ref ref(new BinaryVideo(producer, riuInfo, end - first));
    Container& c(ref->getData());
    while (first != end) c.push_back(*first++);
    return ref;
}

Header::Ref
BinaryVideo::CDRLoader(const QSharedPointer<QByteArray>& raw)
{
    return Make(raw);
}


BinaryVideo::BinaryVideo()
    : Super(GetMetaTypeInfo())
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer,  const RIUInfo& riuInfo, size_t size)
    : Super(producer, GetMetaTypeInfo(), riuInfo, size)
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer, const Video::Ref& basis)
    : Super(producer, GetMetaTypeInfo(), basis, basis->size())
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer, const BinaryVideo::Ref& basis)
    : Super(producer, GetMetaTypeInfo(), basis, basis->size())
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer)
   : Super(producer, GetMetaTypeInfo())
{
   ;
}

BinaryVideo::~BinaryVideo()
{
    ;
}

Video::Ref
BinaryVideo::getVideoBasis() const
{
    qCDebug(radarmsg) << "getVideoBasis" ;

    Header::Ref basis(getBasis());
    Video::Ref video;
    while (basis) {
    qCDebug(radarmsg) << basis.get() ;
    if (basis->getMetaTypeInfo().isa(Messages::MetaTypeInfo::Value::kVideo)) {
        video = boost::dynamic_pointer_cast<Video>(basis);
        break;
    }
    basis = basis->getBasis();
    }

    return video;
}
