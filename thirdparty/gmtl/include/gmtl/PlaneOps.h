// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_PLANEOPS_H_
#define _GMTL_PLANEOPS_H_

#include <gmtl/Defines.h>
#include <gmtl/Plane.h>
#include <gmtl/Math.h>

namespace gmtl
{

/** @ingroup Ops Plane 
 * @name Plane Operations
 * @{
 */
       
/**
 * Computes the distance from the plane to the point.
 *
 * @param plane      the plane to compare the point to it
 * @param pt         a point in space
 *
 * @return  the distance from the point to the plane
 */
template< class DATA_TYPE >
DATA_TYPE distance( const Plane<DATA_TYPE>& plane, const Point<DATA_TYPE, 3>& pt )
{
   return ( dot(plane.mNorm, static_cast< Vec<DATA_TYPE, 3> >(pt)) - plane.mOffset );
}

/**
 * Determines which side of the plane the given point lies. This operation is
 * done with ZERO tolerance.
 *
 * @param plane      the plane to compare the point to
 * @param pt         the point to test
 *
 * @return  the PlaneSide enum describing on which side of the plane the point
 *          lies
 */
template< class DATA_TYPE >
PlaneSide whichSide( const Plane<DATA_TYPE>& plane,
                     const Point<DATA_TYPE, 3>& pt )
{
   DATA_TYPE dist = distance( plane, pt );

   if ( dist < DATA_TYPE(0) )
      return NEG_SIDE;
   else if ( dist > DATA_TYPE(0) )
      return POS_SIDE;
   else
      return ON_PLANE;
}

/**
 * Determines which side of the plane the given point lies with the given
 * epsilon tolerance.
 *
 * @param plane      the plane to compare the point to
 * @param pt         the point to test
 * @param eps        the epsilon tolerance to use while testing
 *
 * @return  the PlaneSide enum describing on which side of the plane the point
 *          lies
 */
template< class DATA_TYPE >
PlaneSide whichSide( const Plane<DATA_TYPE>& plane,
                     const Point<DATA_TYPE, 3>& pt,
                     const DATA_TYPE& eps )
{
   DATA_TYPE dist = distance( plane, pt );

   if ( dist < eps )
      return NEG_SIDE;
   else if ( dist > eps )
      return POS_SIDE;
   else
      return ON_PLANE;
}

/**
 * Finds the point on the plane that is nearest to the given point. As a
 * convenience, the distance between pt and result is returned.
 *
 * @param plane  [in]   the plane to compare the point to
 * @param pt     [in]   the point to test
 * @param result [out]  the point on plane closest to pt
 *
 * @return  the distance between pt and result
 */
template< class DATA_TYPE >
DATA_TYPE findNearestPt( const Plane<DATA_TYPE>& plane,
                         const Point<DATA_TYPE, 3>& pt,
                         Point<DATA_TYPE, 3>& result )
{
   // GGI:  p297
   // GGII: p223
   gmtlASSERT( isNormalized(plane.mNorm) );   // Assert: Normalized
   DATA_TYPE dist_to_plane(0);
   dist_to_plane = plane.mOffset + dot( plane.mNorm, static_cast< Vec<DATA_TYPE, 3> >(pt) );
   result = pt - (plane.mNorm * dist_to_plane);
   return dist_to_plane;
}

/**
 * Mirror the point by the plane
 */
template< class DATA_TYPE, unsigned SIZE>
void reflect( Point<DATA_TYPE, SIZE>& result,
              const Plane<DATA_TYPE>& plane,
              const Point<DATA_TYPE, SIZE>& point )
{
   gmtl::Point<DATA_TYPE, SIZE> point_on_plane;
   findNearestPt( plane, point, point_on_plane );
   gmtl::Vec<DATA_TYPE, SIZE> dir = point_on_plane - point;
   result = point + (dir * DATA_TYPE(2.0f));
}
/** @} */

/** @ingroup Compare Plane
 * @name Plane Comparitors
 * @{
 */

/**
 * Compare two planes to see if they are EXACTLY the same. In other words, this
 * comparison is done with zero tolerance.
 *
 * @param p1      the first plane to compare
 * @param p2      the second plane to compare
 *
 * @return  true if they are equal, false otherwise
 */
template< class DATA_TYPE >
inline bool operator==( const Plane<DATA_TYPE>& p1, const Plane<DATA_TYPE>& p2 )
{
   return ( (p1.mNorm == p2.mNorm) && (p1.mOffset == p2.mOffset) );
}

/**
 * Compare two planes to see if they are not EXACTLY the same. In other words,
 * this comparison is done with zero tolerance.
 *
 * @param p1      the first plane to compare
 * @param p2      the second plane to compare
 *
 * @return  true if they are not equal, false otherwise
 */
template< class DATA_TYPE >
inline bool operator!=( const Plane<DATA_TYPE>& p1, const Plane<DATA_TYPE>& p2 )
{
   return (! (p1 == p2));
}

/**
 * Compare two planes to see if they are the same within the given tolerance.
 *
 * @param p1      the first plane to compare
 * @param p2      the second plane to compare
 * @param eps     the tolerance value to use
 *
 * @pre eps must be >= 0
 *
 * @return  true if they are equal within a tolerance, false otherwise
 */
template< class DATA_TYPE >
inline bool isEqual( const Plane<DATA_TYPE>& p1, const Plane<DATA_TYPE>& p2,
                     const DATA_TYPE& eps )
{
   gmtlASSERT( eps >= 0 );
   return ( (isEqual(p1.mNorm, p2.mNorm, eps)) &&
            (Math::isEqual(p1.mOffset, p2.mOffset, eps)) );
}
/** @} */


} // namespace gmtl

#endif

