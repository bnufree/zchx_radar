#ifndef TARGETIMAGE_H
#define TARGETIMAGE_H

#include "ImageDataTypes.h"
#include "TargetSize.h"
#include "boost/shared_array.hpp"
#include "boost/shared_ptr.hpp"
#include <iomanip>
#include <vector>
#include "common.h"

using AzimuthData = std::vector<AZIMUTH>;
using AzimuthDataPtr = boost::shared_ptr<AzimuthData>;

// TargetImage prototype
template <class TYPE>
class TargetImage;

// definitions for TargetImage's of masks and video (and pointers to each)
using VideoTargetImage = TargetImage<VIDEODATA>;
using VideoTargetImagePtr = boost::shared_ptr<VideoTargetImage>;
using BinaryTargetImage = TargetImage<BINARYDATA>;
using BinaryTargetImagePtr = boost::shared_ptr<BinaryTargetImage>;

template <class TYPE>
class TargetImage {
private:
    // m_data is an array m_rows x m_cols that stores the actual pixel data
    boost::shared_array<TYPE> m_data;

    // m_az is a pointer to a vector that stores the azimuth for each row.
    //  it may or may not be valid for the image (NULL if not valid)
    AzimuthDataPtr m_az;

    // m_minRange and m_maxRange define (inclusive) the range of the image (always valid)
    RANGEBIN m_minRange;
    RANGEBIN m_maxRange;

    // m_rows and m_cols define the number of pixels in the image (always valid)
    PRI_COUNT m_rows;
    RANGEBIN m_cols;

public:
    using TargetImagePtr = boost::shared_ptr<TargetImage<TYPE>>;

    // constructs an unitialized image
    TargetImage(RANGEBIN minRange, RANGEBIN maxRange, AzimuthDataPtr az) :
        m_az(az), m_minRange(minRange), m_maxRange(maxRange)
    {
        m_cols = maxRange - minRange + 1;
        m_rows = az->size();
        m_data = boost::shared_array<TYPE>(new TYPE[m_rows * m_cols]);
    }

    // constructs an initialized image (the owner is responsible for deleting the data)
    TargetImage(RANGEBIN minRange, RANGEBIN maxRange, AzimuthDataPtr az, TYPE* data) :
        m_data(data), m_az(az), m_minRange(minRange), m_maxRange(maxRange)
    {
        m_cols = maxRange - minRange + 1;
        m_rows = az->size();
    }

    // constructs an initialized image without az data
    TargetImage(RANGEBIN minRange, RANGEBIN maxRange, PRI_COUNT rows, TYPE* data) :
        m_data(data), m_minRange(minRange), m_maxRange(maxRange)
    {
        m_cols = maxRange - minRange + 1;
        m_rows = rows;
    }

    void SetAzimuthData(AzimuthDataPtr az) { m_az = az; }

    AzimuthDataPtr GetAzimuthData() { return m_az; }

    TYPE* GetDataRef() { return m_data.get(); }

    PRI_COUNT GetRows() { return m_rows; }

    RANGEBIN GetCols() { return m_cols; }

    // returns the size of the image.
    TargetSize GetSize()
    {
        TargetSize size;
        size.minRange = m_minRange;
        size.maxRange = m_maxRange;
        size.priCount = (PRI_COUNT)m_rows;
        size.maxRangeValid = true;
        size.minRangeValid = true;
        size.priValid = true;

        // include az data, only if it is valid
        if (m_az) {
            size.minAz = m_az->front();
            size.maxAz = m_az->back();
            size.minAzValid = true;
            size.maxAzValid = true;
        } else {
            size.minAzValid = false;
            size.maxAzValid = false;
        }

        return size;
    }

    // print the image in a printer-friendly format
    void Dump()
    {
        LOGDEBUG << m_rows << "x" << m_cols << " range=" << m_minRange << " az=" << m_az->front()<<std::endl;
        for (int r = 0; r < m_rows; r++) {
            LOGDEBUG <<std::setw(3)<<(*m_az)[r] << ":";
            for (int c = 0; c < m_cols; c++) { LOGDEBUG << (m_data[(r * m_cols) + c] ? "X" : " "); }
            LOGDEBUG<<std::endl;
        }
    }

    // print the image data in a comma separated value format
    void DumpCSV(std::ostream& os)
    {
        for (int r = 0; r < m_rows; r++) {
            for (int c = 0; c < m_cols; c++) { os << (int)m_data[(r * m_cols) + c] << ", "; }
            os << std::endl;
        }
    }

    // perform a difference gradient in the x and y directions on the image.
    //  ignore any pixels where the kernel would require a value where the
    //  mask is zero.
    void MaskGradient(BinaryTargetImagePtr maskImage, TargetImagePtr& xGradient, TargetImagePtr& yGradient)
    {
#define KERNEL_1_1 ((int)-1)
#define KERNEL_1_2 ((int)-2)
#define KERNEL_1_3 ((int)-1)
#define KERNEL_2_1 ((int)0)
#define KERNEL_2_2 ((int)0)
#define KERNEL_2_3 ((int)0)
#define KERNEL_3_1 ((int)1)
#define KERNEL_3_2 ((int)2)
#define KERNEL_3_3 ((int)1)

        // create the actual image storage arrays
        TYPE* x_grad = new TYPE[m_rows * m_cols]; // gradient along columns
        TYPE* y_grad = new TYPE[m_rows * m_cols]; // graident along rows
        BINARYDATA* mask = maskImage->GetDataRef();

        // point the shared array pointers to the new data
        xGradient = TargetImagePtr(new TargetImage(m_minRange, m_maxRange, m_az, x_grad));
        yGradient = TargetImagePtr(new TargetImage(m_minRange, m_maxRange, m_az, y_grad));

        // clear the first row and last row
        memset(x_grad, 0x00, m_cols * sizeof(VIDEODATA));
        memset(y_grad, 0x00, m_cols * sizeof(VIDEODATA));
        memset(x_grad + (m_rows - 1) * m_cols, 0x00, m_cols * sizeof(VIDEODATA));
        memset(y_grad + (m_rows - 1) * m_cols, 0x00, m_cols * sizeof(VIDEODATA));

        // do the convolution
        for (PRI_COUNT r = 1; r < (m_rows - 1); r++) {
            // clear the first and last column elements
            x_grad[r * m_cols] = 0;
            y_grad[r * m_cols] = 0;
            x_grad[(r + 1) * m_cols - 1] = 0;
            y_grad[(r + 1) * m_cols - 1] = 0;
            for (RANGEBIN c = 1; c < (m_cols - 1); c++) {
                int center = r * m_cols + c;
                int centerPrevRow = center - m_cols;
                int centerNextRow = center + m_cols;

                if (mask[centerPrevRow - 1] && mask[centerPrevRow + 0] && mask[centerPrevRow + 1] && mask[center - 1] &&
                    mask[center + 0] && mask[center + 1] && mask[centerNextRow - 1] && mask[centerNextRow + 0] &&
                    mask[centerNextRow + 1]) {
                    int x, y;

                    // note that integer arithmetic is used here to help alleviate overflows. also,
                    //  (for the original kernel coefficients) the order of operations is optimized
                    //  to prevent overflows. (in other words, -1 multiples are followed by +1 multiples,
                    //  etc.)

                    x = KERNEL_1_3 * (int)m_data[centerPrevRow - 1] + KERNEL_2_3 * (int)m_data[centerPrevRow + 0] +
                        KERNEL_3_3 * (int)m_data[centerPrevRow + 1]

                        + KERNEL_1_2 * (int)m_data[center - 1] + KERNEL_2_2 * (int)m_data[center + 0] +
                        KERNEL_3_2 * (int)m_data[center + 1]

                        + KERNEL_1_1 * (int)m_data[centerNextRow - 1] + KERNEL_2_1 * (int)m_data[centerNextRow + 0] +
                        KERNEL_3_1 * (int)m_data[centerNextRow + 1];

                    y = KERNEL_1_1 * (int)m_data[centerPrevRow - 1] + KERNEL_2_1 * (int)m_data[center - 1] +
                        KERNEL_3_1 * (int)m_data[centerNextRow - 1]

                        + KERNEL_1_2 * (int)m_data[centerPrevRow] + KERNEL_2_2 * (int)m_data[center] +
                        KERNEL_3_2 * (int)m_data[centerNextRow]

                        + KERNEL_1_3 * (int)m_data[centerPrevRow + 1] + KERNEL_2_3 * (int)m_data[center + 1] +
                        KERNEL_3_3 * (int)m_data[centerNextRow + 1];

                    // do not wrap if the result is too large, just saturate the short
                    // this is important, as wrapping would cause spurious targets to
                    // be detected.  NB: this will only work as intended if sizeof(int) > sizeof(VIDEODATA)
                    x_grad[center] =
                        (VIDEODATA)std::min<int>(std::max<int>(x, (int)std::numeric_limits<VIDEODATA>::min()),
                                                 (int)std::numeric_limits<VIDEODATA>::max());
                    y_grad[center] =
                        (VIDEODATA)std::min<int>(std::max<int>(y, (int)std::numeric_limits<VIDEODATA>::min()),
                                                 (int)std::numeric_limits<VIDEODATA>::max());
                } else {
                    x_grad[center] = 0;
                    y_grad[center] = 0;
                }
            }
        }
    }

    // identify regions where the slope transitions from positive to negative
    //  in the x-direction.  Further, if the slope goes {positive, zero, negative},
    //  identify the entire "zero" region as part of the peak as well.  this will
    //  give all the "peak" regions in the image.
    BinaryTargetImagePtr IdentifyPeaksX()
    {
        BINARYDATA* peaks = new BINARYDATA[m_rows * m_cols];

        // point the shared array pointers to the new data
        BinaryTargetImagePtr peaksImg(new BinaryTargetImage(m_minRange, m_maxRange, m_az, peaks));

        for (PRI_COUNT r = 0; r < m_rows; r++) {
            memset(peaks + r * m_cols, 0x00, m_cols);

            bool lastWasPlus = (m_data[r * m_cols] > 0);
            RANGEBIN startPlus = 0;

            for (RANGEBIN c = 0; c < m_cols; c++) {
                TYPE pix = m_data[r * m_cols + c];

                if (pix > 0) {
                    lastWasPlus = true;
                    startPlus = c;
                } else if (pix < 0) {
                    if (lastWasPlus) {
                        memset(peaks + r * m_cols + startPlus, 0x01, c - startPlus);
                        lastWasPlus = false;
                    }
                }
            }
        }

        return peaksImg;
    }

    // identify regions where the slope transitions from positive to negative
    //  in the y-direction.  Further, if the slope goes {positive, zero, negative},
    //  identify the entire "zero" region as part of the peak as well.  this will
    //  give all the "peak" regions in the image.
    BinaryTargetImagePtr IdentifyPeaksY()
    {
        BINARYDATA* peaks = new BINARYDATA[m_rows * m_cols];

        // point the shared array pointers to the new data
        BinaryTargetImagePtr peaksImg(new BinaryTargetImage(m_minRange, m_maxRange, m_az, peaks));

        for (RANGEBIN c = 0; c < m_cols; c++) {
            bool lastWasPlus = (m_data[c] > 0);
            PRI_COUNT startPlus = 0;

            for (PRI_COUNT r = 0; r < m_rows; r++) {
                TYPE pix = m_data[r * m_cols + c];
                peaks[r * m_cols + c] = 0;

                if (pix > 0) {
                    lastWasPlus = true;
                    startPlus = r;
                } else if (pix < 0) {
                    if (lastWasPlus) {
                        for (int ri = startPlus; ri < r; ri++) peaks[ri * m_cols + c] = 1;
                        lastWasPlus = false;
                    }
                }
            }
        }

        return peaksImg;
    }

    // perform a binary "and" of two images.  this probalby on makes sense to
    // use with BinaryTargetImages.  Used to determine where the peaks in the x
    // and the peaks in the y overlap.
    BinaryTargetImagePtr operator&&(const TargetImage& op)
    {
        if ((op.m_rows * op.m_cols) != (m_rows * m_cols)) {
            LOGDEBUG << "TargetImage::operator& : unable to and image arrays together";
            //abort();
        }

        BINARYDATA* result = new BINARYDATA[m_rows * m_cols];

        // point the shared array pointers to the new data
        BinaryTargetImagePtr resultImg(new BinaryTargetImage(m_minRange, m_maxRange, m_az, result));

        for (unsigned int i = 0; i < (unsigned int)(m_rows * m_cols); i++) result[i] = m_data[i] && op.m_data[i];

        return resultImg;
    }
};

/** \file
 */

#endif
