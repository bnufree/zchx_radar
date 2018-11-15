#ifndef SIDECAR_GEOMETRY_VECTOR_H // -*- C++ -*-
#define SIDECAR_GEOMETRY_VECTOR_H


namespace ZCHX {
namespace Geometry {

class UnitVector;

/** Tuple representing a vector in 3-D space. Supports basic vector/vector and vector/scalar operations.
    Borrowed from RWSL project. Calculates on demand magnitude and direction values.

    NOTE: not sure if elevation and Z axis are handled properly. Need to be
    vetted.
*/
class Vector {
public:
    /** Default constructor.
     */
    Vector();

    /** Constructor using component values.

        \param x length of vector along X axis

        \param y length of vector along Y axis

        \param z length of vector along Z axis
    */
    Vector(double x, double y, double z);

    /** Constructor using direction and magnitude values

        \param direction UnitVector representing the direction of the vector

        \param length the magnitude of the vector
    */
    Vector(const UnitVector& direction, double length);

    /** Constructor from two Vector objects. Calculates the difference between the two.

        \param from starting vector

        \param to ending vector
    */
    Vector(const Vector& from, const Vector& to);

    /** Obtain the X component

        \return length along X axis
    */
    double getX() const { return x_; }

    /** Obtain the Y component

        \return length along Y axis
    */
    double getY() const { return y_; }

    /** Obtain the Z component

        \return length along Z axis
    */
    double getZ() const { return z_; }

    /** Obtain the magnitude of the vector.

        \return SQRT(X*X + Y*Y + Z*Z)
    */
    double getMagnitude() const;

    /** Obtain the squared magnitude of the vector.

        \return X*X + Y*Y + Z*Z
    */
    double getMagnitudeSquared() const;

    /** Obtain the angle in radians of the vector in the X/Y plane and the Y axis (north)

        \return angle in radians
    */
    double getDirection() const;

    /** Obtain the angle in radians of the vector in the Z/Y plane and the Y axis (0 elevation). A well-formed
        vector would only have values in [0-PI/4].

        \return angle in radians
    */
    double getElevation() const;

    /** Add component values from another vector to this one

        \param rhs vector to add

        \return reference to self
    */
    Vector& operator+=(const Vector& rhs);

    /** Subtract component values of another vector from this one

        \param rhs vector to subtract

        \return reference to self
    */
    Vector& operator-=(const Vector& rhs);

    /** Multiply component vales by a given value

        \param scale value to multiply with

        \return reference to self
    */
    Vector& operator*=(double scale);

    /** Divide component vales by a given value

        \param scale value to divide with

        \return reference to self
    */
    Vector& operator/=(double scale);

    /** Write out a textual representation of the Vector to the given C++ output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const;

private:
    /** Internal flags that indicate which cache attributes are valid
     */
    enum Flags {
        kHasMagnitudeSquared = (1 << 0),
        kHasMagnitude = (1 << 1),
        kHasDirection = (1 << 2),
        kHasElevation = (1 << 3)
    };

    mutable uint32_t flags_;          ///< Cache validity flags
    double x_;                        ///< X component
    double y_;                        ///< Y component
    double z_;                        ///< Z component
    mutable double magnitudeSquared_; ///< Cached magnitude^2 value
    mutable double magnitude_;        ///< Cached magnitude value
    mutable double direction_;        ///< Cached direction angle
    mutable double elevation_;        ///< Cached elevation angle
};

/** Definintion of Vector + Vector

    \param lhs Vector to add

    \param rhs Vector to add

    \return Vector + Vector
*/
inline Vector
operator+(const Vector& lhs, const Vector& rhs)
{
    Vector tmp(lhs);
    return tmp += rhs;
}

/** Definintion of Vector - Vector

    \param lhs Vector to subtract

    \param rhs Vector to subtract

    \return Vector - Vector
*/
inline Vector
operator-(const Vector& lhs, const Vector& rhs)
{
    Vector tmp(lhs);
    return tmp -= rhs;
}

/** Definintion of Vector * scalar

    \param lhs Vector to multiply

    \param rhs scalar to multiply

    \return Vector * scalar
*/
inline Vector operator*(const Vector& lhs, double rhs)
{
    Vector tmp(lhs);
    return tmp *= rhs;
}

/** Definintion of scalar * Vector

    \param lhs scalar to multiply

    \param rhs Vector to multiply

    \return scalar * Vector
*/
inline Vector operator*(double lhs, const Vector& rhs)
{
    Vector tmp(rhs);
    return tmp *= lhs;
}

/** Definintion of Vector / scalar

    \param lhs Vector to divide

    \param rhs scalar to divide by

    \return Vector / scalar
*/
inline Vector
operator/(const Vector& lhs, double rhs)
{
    Vector tmp(lhs);
    return tmp /= rhs;
}

} // namespace Geometry
} // end namespace SideCar

#endif
