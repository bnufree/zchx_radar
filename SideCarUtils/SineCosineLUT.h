//**************************************************************************
//* @File: SineCosineLUT.h
//* @Description: 正弦余弦查询表接口
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

#ifndef UTILS_SINECOSINELUT_H // -*- C++ -*-
#define UTILS_SINECOSINELUT_H

#include <utility>		// for std::pair
#include <vector>


#include "Exception.h"
#include "UtilsGlobal.h"

namespace ZchxRadarUtils {


/** Simple table lookup class for sine/cosine values. Note that this does not support interpolation, only
    indexed lookup, where the indices range from [0,size) and linearly map to [0,2*PI) radians. One source of
    indices that behave like this is a radar shaft encoder that emits a unique integer value depending on where
    the antenna is pointing, with a resolution of N values.

    The lookup table contains sine and cosine values calculated for N / 4 indices. The lookup() method uses
    various trig identities to calculate the remaining values for the rest of the indices. Because of this space
    reduction, the size of the lookup table must be a multiple of 4.
*/
class ZCHX_API SineCosineLUT
{
public:

    /** Exception thrown if the requested table size is not a multiple of 4.
     */
    struct InvalidSize : public ZchxRadarUtils::Exception {};

    /** Factory method that creates a new lookup table. Checks the \p size argument to see that it is a multiple
        of 4.

        \param size number of integral values in a circle (2 * PI)

        \return new SineCosineLUT object or NULL if invalid size
    */
    static SineCosineLUT* Make(size_t size);

    /** Constructor. Creates and populates the lookup table. Throws an exception if the given size is not a
        multiple of 4.

        \param size number of integral values in a circle (2 * PI)
    */
    SineCosineLUT(size_t size) throw(InvalidSize);

    /** Obtain the sine and cosine values for a given lookup index

        \param index integral value in [0,size) to fetch

        \param sine reference to storage for sine value

        \param cosine reference to storage for cosine value
    */
    void lookup(size_t index, double& sine, double& cosine) const;

    size_t size() const { return size_; }

private:

    /** Internal structure that holds the calculate sine/cosine values.
     */
    struct SineCosine
    {
        SineCosine() {}
        SineCosine(double s, double c) : sine(s), cosine(c) {}
        double sine;
        double cosine;
    };

    std::vector<SineCosine> values_; ///< Lookup table for 1 quadrant of indices
    size_t size_;		     ///< Number of indices in a circle
};

} // end namespace ZchxRadarUtils

#endif
