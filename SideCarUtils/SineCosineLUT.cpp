//**************************************************************************
//* @File: SineCosineLUT.cpp
//* @Description: 正弦余弦查询表实现类
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
//*   1.0  2017/03/08    李鹭       初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************


#include <cmath>

#include <QLoggingCategory>

#include "SineCosineLUT.h"


using namespace ZchxRadarUtils;

SineCosineLUT*
SineCosineLUT::Make(size_t size)
{
    qCDebug(radarutils) << "SineCosineLUT::Make" ;

    if (size % 4 != 0) {
        qCWarning(radarutils) << "Requested LUT size not a multiple of 4 - " << size ;
        return 0;
    }

    return new SineCosineLUT(size);
}

SineCosineLUT::SineCosineLUT(size_t size) throw(InvalidSize)
    : values_(size / 4), size_(size)
{
    if (size % 4 != 0) {
        InvalidSize ex;
        ex << "Size not a multiple of 4 - " << size;
        throw ex;
    }

    // Fill in one quadrant.
    //
    double increment = M_PI * 2.0 / size;
    size = values_.size();
    for (auto index = 0; index < size; ++index) {
        auto angle = index * increment;
        values_[index] = SineCosine(::sin(angle), ::cos(angle));
    }
}

void
SineCosineLUT::lookup(size_t index, double& sine, double& cosine) const
{
    while (index >= size_) {
        index -= size_;
    }

    auto quadrant = index / values_.size();
    index -= quadrant * values_.size();
    const SineCosine& value(values_[index]);

    switch (quadrant) {
    case 0:
        sine = value.sine;
        cosine = value.cosine;
        return;

    case 1:
        sine = value.cosine;
        cosine = - value.sine;
        return;

    case 2:
        sine = - value.sine;
        cosine = - value.cosine;
        return;

    case 3:
        sine = - value.cosine;
        cosine = value.sine;
        return;
    }
}
