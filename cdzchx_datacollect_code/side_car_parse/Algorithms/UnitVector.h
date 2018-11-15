#ifndef SIDECAR_GEOMETRY_UNITVECTOR_H // -*- C++ -*-
#define SIDECAR_GEOMETRY_UNITVECTOR_H

#include <inttypes.h>

namespace ZCHX {
namespace Geometry {

class Vector;

/** Tuple representing a unit vector in 3-D space. A unit vector always has a magnitude of 1. Borrowed from RWSL
    project. Calculates on demand component (X,Y,Z) values.
*/
class UnitVector {
public:
    /** Default constructor.
     */
    UnitVector();

    /** Constructor using direction and elevation angles.

        \param direction angle in radians of the vector in the X/Y plane and
        the Y axis (north)

        \param elevation angle in radians of the vector in the Z/Y plane and
        the Y axis
    */
    UnitVector(double direction, double elevation);

    /** Conversion constructor from a Vector

        \param vector value to work with
    */
    UnitVector(const Vector& vector);

    /** Obtain the angle in radians of the vector in the X/Y plane and the Y axis (north)

        \return angle in radians
    */
    double getDirection() const { return direction_; }

    /** Obtain the angle in radians of the vector in the Z/Y plane and the Y axis (0 elevation). A well-formed
        unit vector would only have values in [0-PI/4].

        \return angle in radians
    */
    double getElevation() const { return elevation_; }

    /** Obtain the X component. Value in [0-1]

        \return length along X axis
    */
    double getX() const;

    /** Obtain the Y component. Value in [0-1]

        \return length along Y axis
    */
    double getY() const;

    /** Obtain the Z component. Value in [0-1]

        \return length along Z axis
    */
    double getZ() const;

private:
    /** Internal flags that indicate which cache attributes are valid
     */
    enum Flags { kHasX = (1 << 0), kHasY = (1 << 1), kHasZ = (1 << 2) };

    mutable uint32_t flags_; ///< Cache validity flags
    double direction_;       ///< Direction component (radians)
    double elevation_;       ///< Elevation component (radians)
    mutable double x_;       ///< Cached X component
    mutable double y_;       ///< Cached Y component
    mutable double z_;       ///< Cached Z component
};

/** Declaration of UnitVector * scalar

    \param lhs UnitVector to multiply

    \param rhs scalar to multiply

    \return Vector result
*/
Vector operator*(const UnitVector& lhs, double rhs);

/** Declaration of scalar * UnitVector

    \param lhs scalar to multiply

    \param rhs UnitVector to multiply

    \return Vector result
*/
Vector operator*(double lhs, const UnitVector& rhs);

} // end namespace Geometry
} // end namespace SideCar

#endif
