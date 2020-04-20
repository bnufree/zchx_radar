///**************************************************************************
//* @File: Extraction.h
//* @Description:  人工抽取目标接口(点迹)
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

#ifndef ZCHX_RADAR_MESSAGES_EXTRACTION_H	// -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_EXTRACTION_H

#include <vector>

#include <boost/shared_ptr.hpp>


#include "Header.h"

namespace ZCHX {
namespace Messages {

/** Data type for a video artifact extraction. Contains range and azimumth information, and derived X, Y
    rectangular coordinates. NOTE: the conversion follows the nautical convention that 0 degrees is north, and
    not the mathematical one (ie x := r * sin(a), not r * cos(a)).
*/
class Extraction 
{
public:

    /** Constructor for new extraction entries.

        \param range range of the artifact

        \param azimuth azimuth of the artifact (radians)
    */
    Extraction(double when, double range, double azimuth, double elevation);

    /** Constructor for objects loaded from an ACE input stream.

        \param cdr stream to read from
    */
    Extraction(const QSharedPointer<QByteArray>& raw);
    ~Extraction()
    {
        //qDebug()<<"~Extraction()";
    }

    double getWhen() const { return when_; }

    /** Obtain the range of the extraction.

        \return value in kilometers
    */
    double getRange() const { return range_; }

    /** Obtain the azimuth of the extraction.

        \return value in radians
    */
    double getAzimuth() const { return azimuth_; }
    double getAzimuthDegree() const {return azimuth_ * 180 / 3.1415926;}

    /** Obtain the elevation of the extraction.

        \return value in meters
    */
    double getElevation() const { return elevation_; }

    /** Obtain whether this extraction is correlated

	\return boolean
    */
    bool getCorrelated() const { return correlated_; }

    /** Set whether this extraction is correlated

	\param corr correlated
    */
    void setCorrelated(bool corr) { correlated_ = corr; }

    /** Obtain the number of scans this extraction has been correlated for

	\return num correlations
    */
    int getNumCorrelations() const { return numCorrelations_; }

    /** Sets the number of scans this extraction has been correlated for

	\param corr -- # of scans
    */
    void setNumCorrelations(int corr) { numCorrelations_ = corr; }

    /** Obtain the X coordinate of the extraction.

        \return value in kilometers
    */
    double getX() const { return x_; }

    /** Obtain the Y coordinate of the extraction.

        \return value in kilometers
    */
    double getY() const { return y_; }

    //计算两个目标之间的距离
    double dis2Extraction(const Extraction& other) const;


    std::ostream& print(std::ostream& os) const
	{ return os << "When: " << when_ << " Range: " << range_ << " Azimuth: " << azimuth_ << " Elevation: "
		    << elevation_; }

    std::ostream& printXML(std::ostream& os) const;

private:

    double when_;
    double range_;		///< Range of extracted object
    double azimuth_;		///< Azimuth of extracted object
    double elevation_;		///< Elevation of extracted object
    double x_, y_;
    bool correlated_;
    int numCorrelations_;
};

/** Container of Extraction objects. The interface is similar to that for TPRIMessage objects, but it does not
    derive from PRIMessageBase since the elements may not have the same azimuth value.
*/
class Extractions : public Header
{
public:
    using Container = std::vector<Extraction>;
    using iterator = Container::iterator;
    using const_iterator = Container::const_iterator;

    static const MetaTypeInfo& GetMetaTypeInfo();
    ~Extractions()
    {
    }

    /** Reference-counting reference to message object. Derived from boost::shared_ptr to allow [] indexing.
     */
    class Ref : public boost::shared_ptr<Extractions>
    {
    public:
	using Base = boost::shared_ptr<Extractions>;

	/** Default constructor.
	 */
	Ref() : Base() {}

	/** Constructor that takes ownership of a Extractions object

	    \param self object to acquire
	*/
	Ref(Extractions* obj) : Base(obj) {}

	/** Conversion constructor for boost::shared_ptr objects.

	    \param r shared ptr to share
	*/
	Ref(Base const& copy) : Base(copy) {}

	/** Obtain a read-only reference to the item at a given position

	    \param index position to dereference

	    \return data reference
	*/
	const Extraction& operator[](size_t index) const
	    { return Base::get()->operator[](index); }

	/** Obtain a reference to the item at a given position

	    \param index position to dereference

	    \return data reference
	*/
	Extraction& operator[](size_t index) { return Base::get()->operator[](index); }

	/** Assignment operator.

	    \param rhs object to copy from

	    \return reference to self
	*/
	Ref& operator=(Ref const& rhs) { Base::operator=(rhs); return *this; }
    };

    static Ref Make(const std::string& producer, const Header::Ref& basis);

    static Ref Make(const QSharedPointer<QByteArray>& raw);

    /** Obtain a writeable reference to the underlying sample data container.

        \return Container reference
    */
    Container& getData() { return data_; }

    /** Allocate enough space in the sample container to hold a certain number of samples. Simply forwards to
        the container's reserve() method.

        \param size space to reserve
    */
    void reserve(size_t size) { data_.reserve(size); }

    /** Obtain the number of data elements in the message.

        \return count
    */
    size_t size() const { return data_.size(); }

    /** Determine if the message has any data at all.

        \return true if empty
    */
    bool empty() const { return data_.empty(); }

    /** Append a sample value to the end of the message

        \param datum value to append
    */
    void push_back(const Extraction& value) { data_.push_back(value); }

    /** Index operator to access samples using array notation.

        \param index sample to obtain

        \return reference to indexed sample
    */
    Extraction& operator[](size_t index) { return data_[index]; }

    /** Read-only index operator to access samples using array notation.

        \param index sample to obtain

        \return read-only reference to indexed sample
    */
    const Extraction& operator[](size_t index) const { return data_[index]; }

    /** Obtain read-only iterator to the first sample value in the message.

        \return read-only iterator
    */
    const_iterator begin() const { return data_.begin(); }

    /** Obtain read-only iterator to the last + 1 sample value in the message.

        \return read-only iterator
    */
    const_iterator end() const { return data_.end(); }

    /** Obtain writable iterator to the first sample value in the message.

        \return writable iterator
    */
    iterator begin() { return data_.begin(); }

    /** Obtain writable iterator to the last + 1 sample value in the message.

        \return writable iterator
    */
    iterator end() { return data_.end(); }

    size_t getSize() const { return data_.size(); }

    /** Read in binary data from a CDR stream.

        \param cdr stream to read from

        \return stream read from
    */
    void load(const QSharedPointer<QByteArray>& raw);

    std::ostream& printData(std::ostream& os) const;

    std::ostream& printDataXML(std::ostream& os) const;

    void append(const Extractions::Ref& other)
    {
        if(!other) return;
        for(int i=0;i<other->getSize(); i++)
        {
            this->data_.push_back(other[i]);
        }
    }

    void clear()
    {
        this->data_.clear();
    }

protected:

    Extractions()
    : Header(GetMetaTypeInfo()) {}

    Extractions(const std::string& producer)
	: Header(producer, GetMetaTypeInfo()) {}

    Extractions(const std::string& producer, const Header::Ref& basis)
    : Header(producer, GetMetaTypeInfo(), basis) {}

private:
    Container data_;

    static Header::Ref CDRLoader(const QSharedPointer<QByteArray>& raw);
    
    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
