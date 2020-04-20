#include <cmath>
#include <cstdlib>
#include <iostream>

#include "UnitVector.h"
#include "Vector.h"

using namespace ZCHX::Geometry;

Vector::Vector() :
    /*flags_(kHasMagnitudeSquared | kHasMagnitude | kHasDirection | kHasElevation),*/ x_(0.0), y_(0.0), z_(0.0)/*,
    magnitudeSquared_(0.0), magnitude_(0.0), direction_(0.0), elevation_(0.0)*/
{
    ;
}

Vector::Vector(double x, double y, double z) : /*flags_(0), */x_(x), y_(y), z_(z)
{
    ;
}

Vector::Vector(const Vector& from, const Vector& to) :
    /*flags_(0),*/ x_(to.getX() - from.getX()), y_(to.getY() - from.getY()), z_(to.getZ() - from.getZ())
{
    ;
}

Vector::Vector(const UnitVector& uv, double length) :
    /*flags_(kHasMagnitudeSquared | kHasMagnitude | kHasDirection | kHasElevation),*/ x_(uv.getX() * length),
    y_(uv.getY() * length), z_(uv.getZ() * length)/*, magnitudeSquared_(length * length), magnitude_(length),
    direction_(uv.getDirection()), elevation_(uv.getElevation())*/
{
    ;
}

double
Vector::getMagnitudeSquared() const
{
    return x_ * x_ + y_ * y_ + z_ * z_;
#if 0
    if (!(flags_ & kHasMagnitudeSquared)) {
        if (flags_ & kHasMagnitude) {
            magnitudeSquared_ = magnitude_ * magnitude_;
        } else {
            magnitudeSquared_ = x_ * x_ + y_ * y_ + z_ * z_;
        }
        flags_ |= kHasMagnitudeSquared;
    }

    return magnitudeSquared_;
#endif
}

double
Vector::getMagnitude() const
{
    return ::sqrt(getMagnitudeSquared());
#if 0
    if (!(flags_ & kHasMagnitude)) {
        magnitude_ = ::sqrt(getMagnitudeSquared());
        flags_ |= kHasMagnitude;
    }

    return magnitude_;
#endif
}

double
Vector::getDirection() const
{
    if (!x_ && !y_) {
        return 0.0;
    } else {
        return ::atan2(x_, y_);
    }
#if 0
    if (!(flags_ & kHasDirection)) {
        if (!x_ && !y_) {
            direction_ = 0.0;
        } else {
            direction_ = ::atan2(x_, y_);
        }
        flags_ |= kHasDirection;
    }

    return direction_;
#endif
}

double
Vector::getElevation() const
{
    double m = getMagnitude();
    if (!z_ || !m) {
        return  0.0;
    } else {
        return ::asin(z_ / m);
    }
#if 0
    if (!(flags_ & kHasElevation)) {
        double m = getMagnitude();
        if (!z_ || !m) {
            elevation_ = 0.0;
        } else {
            elevation_ = ::asin(z_ / m);
        }
        flags_ |= kHasElevation;
    }

    return elevation_;
#endif
}

Vector&
Vector::operator+=(const Vector& rhs)
{
    x_ += rhs.x_;
    y_ += rhs.y_;
    z_ += rhs.z_;
//    flags_ = 0;
    return *this;
}

Vector&
Vector::operator-=(const Vector& rhs)
{
    x_ -= rhs.x_;
    y_ -= rhs.y_;
    z_ -= rhs.z_;
//    flags_ = 0;
    return *this;
}

Vector&
Vector::operator*=(double scale)
{
    x_ *= scale;
    y_ *= scale;
    z_ *= scale;
//    flags_ = 0;
    return *this;
}

Vector&
Vector::operator/=(double scale)
{
    static const double kMinimum = 1.0E-38;

    if (std::abs(scale) < kMinimum) {
        std::cerr << "Vector::operator/=(): attempt to divide by very small "
                  << "number - " << scale << std::endl;
        scale = scale < 0 ? -kMinimum : kMinimum;
    }

    return operator*=(1.0 / scale);
}
