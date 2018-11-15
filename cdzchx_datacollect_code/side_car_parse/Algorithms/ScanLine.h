#ifndef SCANLINE_H // -*- C++ -*-
#define SCANLINE_H

#include "ImageDataTypes.h"

// ScanLine is a pure virtual class that allows scan lines with vectors
//  and scan lines with arrays to be run to the ImageSegmentation class
template <class TYPE>
class ScanLine {
public:
    virtual ~ScanLine(){};

    virtual RANGEBIN size() const = 0;
    virtual TYPE operator[](RANGEBIN bin) const = 0;
};

// ScanLineArray provides a ScanLine interface to an array of data
template <class TYPE>
class ScanLineArray : public ScanLine<TYPE> {
public:
    ScanLineArray(TYPE* data, unsigned int size) : m_data(data), m_size(size) {}

    virtual RANGEBIN size() const { return m_size; }

    virtual TYPE operator[](RANGEBIN bin) const { return m_data[bin]; }

private:
    TYPE* m_data;
    unsigned int m_size;
};

// ScanLineVector provies a ScanLine interface to a vector of data
template <class TYPE>
class ScanLineVector : public ScanLine<TYPE> {
public:
    ScanLineVector(std::vector<TYPE>* data) : m_data(data) {}

    virtual ~ScanLineVector() {}

    virtual RANGEBIN size() const { return m_data->size(); }

    virtual TYPE operator[](RANGEBIN bin) const { return (*m_data)[bin]; }

    void push_back(TYPE data) { m_data->push_back(data); }

private:
    std::vector<TYPE>* m_data;
};

using BinaryScanLine = ScanLine<BINARYDATA>;
using BinaryScanLineVector = ScanLineVector<BINARYDATA>;
using BinaryScanLineArray = ScanLineArray<BINARYDATA>;
using VideoScanLineVector = std::vector<VIDEODATA>;

/** \file
 */

#endif
