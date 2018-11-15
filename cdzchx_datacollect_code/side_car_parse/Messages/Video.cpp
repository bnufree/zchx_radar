///**************************************************************************
//* @File: Video.cpp
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
#include <algorithm>

#include <QLoggingCategory>

#include "Video.h"

using namespace ZCHX::Messages;

MetaTypeInfo Video::metaTypeInfo_(MetaTypeInfo::Value::kVideo, "Video",&Video::CDRLoader);


const MetaTypeInfo&
Video::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

Video::Ref
Video::Make(const std::string& producer, const RIUInfo& riuInfo, size_t count)
{
    Ref ref(new Video(producer, riuInfo, count));
    return ref;
}

Video::Ref
Video::Make(const std::string& producer,  const RIUInfo& riuInfo, const DatumType* first, const DatumType* end)
{
    Ref ref(new Video(producer, riuInfo, end - first));
    Container& c(ref->getData());

    // !!! TODO: measure best way to initialize
    // c.resize(end - first, 0);
    // std::copy(first, end, c.begin());
    while (first != end) c.push_back(*first++);
    return ref;
}

Video::Ref
Video::Make(const std::string& producer,  const RIUInfo& riuInfo, const Container& data)
{
    Ref ref(new Video(producer, riuInfo, data));
    return ref;
}

Video::Ref
Video::Make(const std::string& producer, const PRIMessage::Ref& basis)
{
    Ref ref(new Video(producer, basis));
    return ref;
}

Video::Ref
Video::Make(const QSharedPointer<QByteArray>& raw)
{
    Ref ref(new Video);
    ref->load(raw);
    return ref;
}

Header::Ref
Video::CDRLoader(const QSharedPointer<QByteArray>& raw)
{
    return Make(raw);
}


Video::Video(const std::string& producer,  const RIUInfo& riuInfo, size_t size)
    : Super(producer, GetMetaTypeInfo(), riuInfo, size)
{
    ;
}

Video::Video(const std::string& producer,  const RIUInfo& riuInfo, const Container& data)
    : Super(producer, GetMetaTypeInfo(), riuInfo, data)
{
    ;
}

Video::Video(const std::string& producer, const PRIMessage::Ref& basis)
    : Super(producer, GetMetaTypeInfo(), basis, basis->size())
{
    ;
}

Video::Video(const std::string& producer)
    : Super(producer, GetMetaTypeInfo())
{
    ;
}

Video::Video()
    : Super(GetMetaTypeInfo())
{
    ;
}

Video::~Video()
{
    ;
}
