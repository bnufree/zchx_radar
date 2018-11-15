///**************************************************************************
//* @File: Extraction.cpp
//* @Description:  二进制抽取者类(点迹)
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
#include <cmath>

#include <QLoggingCategory>

#include "zchxRadarUtils.h"

#include "Extraction.h"


using namespace ZCHX::Messages;

MetaTypeInfo Extractions::metaTypeInfo_(MetaTypeInfo::Value::kExtractions, "Extractions",
                                        &Extractions::CDRLoader);

Extraction::Extraction(qint64 when, double range, double azimuth, double elevation)
    : when_(when), range_(range), azimuth_(azimuth), elevation_(elevation), x_(range * ::sin(azimuth)),
      y_(range * ::cos(azimuth))
{
    ;
}

Extraction::Extraction(const QSharedPointer<QByteArray>& raw)
{
    // when_;
    // range_;
    // azimuth_;
    // elevation_;
    // x_;
    // y_;
}

std::ostream&
Extraction::printXML(std::ostream& os) const
{
    return os << "<extraction when=\"" << when_ << "\" range=\"" << range_
          << "\" azimuth=\"" << ZchxRadarUtils::radiansToDegrees(azimuth_)
	      << "\" elevation=\"" << elevation_ << "\">\n";
}

const MetaTypeInfo&
Extractions::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}


Extractions::Ref
Extractions::Make(const std::string& producer, const Header::Ref& basis)
{
    Ref ref(new Extractions(producer, basis));
    return ref;
}

Extractions::Ref
Extractions::Make(const QSharedPointer<QByteArray>& raw)
{
    Ref ref(new Extractions);
    ref->load(raw);
    return ref;
}

Header::Ref
Extractions::CDRLoader(const QSharedPointer<QByteArray>& raw)
{
    return Make(raw);
}


void
Extractions::load(const QSharedPointer<QByteArray>& raw)
{
    // Header::load(cdr);
    // uint32_t count;
    // cdr >> count;
    // data_.reserve(count);
    // while (count--) {
    // Extraction extraction(cdr);
    // data_.push_back(extraction);
    // }

}


std::ostream&
Extractions::printData(std::ostream& os) const
{
    for (size_t index = 0; index < size(); ++index) {
//	os << index << ": " << data_[index] << '\n';
    os << index << ": "  << '\n'; //@fixme heron lee
    }
    return os;
}

std::ostream&
Extractions::printDataXML(std::ostream& os) const
{
    for (size_t index = 0; index < size(); ++index) {
    data_[index].printXML(os);
    }
    return os;
}
