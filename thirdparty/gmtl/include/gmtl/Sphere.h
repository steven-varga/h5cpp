// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_SPHERE_H_
#define _GMTL_SPHERE_H_

#include <gmtl/Point.h>

namespace gmtl
{

/**
 * Describes a sphere in 3D space by its center point and its radius.
 *
 * @param DATA_TYPE     the internal type used for the point and radius
 * @ingroup Types
 */
template<class DATA_TYPE>
class Sphere
{
public:
   typedef DATA_TYPE DataType;

public:
   /**
    * Constructs a sphere centered at the origin with a radius of 0.
    */
   Sphere()
      : mRadius( 0 )
   {}

   /**
    * Constructs a sphere with the given center and radius.
    *
    * @param center     the point at which to center the sphere
    * @param radius     the radius of the sphere
    */
   Sphere( const Point<DATA_TYPE, 3>& center, const DATA_TYPE& radius )
      : mCenter( center ), mRadius( radius )
   {}

   /**
    * Constructs a duplicate of the given sphere.
    *
    * @param sphere     the sphere to make a copy of
    */
   Sphere( const Sphere<DATA_TYPE>& sphere )
      : mCenter( sphere.mCenter ), mRadius( sphere.mRadius )
   {}

   /**
    * Gets the center of the sphere.
    *
    * @return  the center point of the sphere
    */
   const Point<DATA_TYPE, 3>& getCenter() const
   {
      return mCenter;
   }

   /**
    * Gets the radius of the sphere.
    *
    * @return  the radius of the sphere
    */
   const DATA_TYPE& getRadius() const
   {
      return mRadius;
   }

   /**
    * Sets the center point of the sphere.
    *
    * @param center     the new point at which to center the sphere
    */
   void setCenter( const Point<DATA_TYPE, 3>& center )
   {
      mCenter = center;
   }

   /**
    * Sets the radius of the sphere.
    *
    * @param radius     the new radius of the sphere
    */
   void setRadius( const DATA_TYPE& radius )
   {
      mRadius = radius;
   }

public:
   /**
    * The center of the sphere.
    */
   Point<DATA_TYPE, 3> mCenter;

   /**
    * The radius of the sphere.
    */
   DATA_TYPE mRadius;
};

// --- helper types --- //
typedef Sphere<float>   Spheref;
typedef Sphere<double>  Sphered;

};

#endif
