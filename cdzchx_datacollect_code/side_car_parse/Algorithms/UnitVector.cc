#include "zchxRadarUtils.h"

#include "UnitVector.h"
#include "Vector.h"

using namespace ZCHX::Geometry;

UnitVector::UnitVector() : flags_(kHasX | kHasY | kHasZ), direction_(0.0), elevation_(0.0), x_(1.0), y_(0.0), z_(0.0)
{
    ;
}

UnitVector::UnitVector(double direction, double elevation) :
    flags_(0), direction_(::ZchxRadarUtils::normalizeRadians(direction)), elevation_(::ZchxRadarUtils::normalizeRadians(elevation))
{
    ;
}

UnitVector::UnitVector(const Vector& vector) :
    flags_(0), direction_(vector.getDirection()), elevation_(vector.getElevation())
{
    ;
}

double
UnitVector::getX() const
{
    if (!(flags_ & kHasX)) {
        flags_ |= kHasX;
        x_ = ::sin(direction_);
    }

    return x_;
}

double
UnitVector::getY() const
{
    if (!(flags_ & kHasY)) {
        flags_ |= kHasY;
        y_ = ::cos(direction_);
    }

    return y_;
}

double
UnitVector::getZ() const
{
    if (!(flags_ & kHasZ)) {
        flags_ |= kHasZ;
        z_ = ::sin(elevation_);
    }

    return z_;
}

Vector operator*(const UnitVector& lhs, double rhs)
{
    return Vector(lhs, rhs);
}

Vector operator*(double lhs, const UnitVector& rhs)
{
    return Vector(rhs, lhs);
}
