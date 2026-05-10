// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_LINESEGOPS_H_
#define _GMTL_LINESEGOPS_H_

#include <gmtl/LineSeg.h>
#include <gmtl/RayOps.h>

namespace gmtl {

/**
 * Finds the closest point on the line segment to a given point.
 *
 * @param lineseg    the line segment to test
 * @param pt         the point which to test against lineseg
 *
 * @return  the point on the line segment closest to pt
 */
template< class DATA_TYPE >
Point<DATA_TYPE, 3> findNearestPt( const LineSeg<DATA_TYPE>& lineseg,
                                   const Point<DATA_TYPE, 3>& pt )
{
   // result = origin + dir * dot((pt-origin), dir)
   return ( lineseg.mOrigin + lineseg.mDir *
            dot(pt - lineseg.mOrigin, lineseg.mDir) / lengthSquared(lineseg.mDir) );
}

/**
 * Computes the shortest distance from the line segment to the given point.
 *
 * @param lineseg    the line segment to test
 * @param pt         the point which to test against lineseg
 *
 * @return  the shortest distance from pt to lineseg
 */
template< class DATA_TYPE >
inline DATA_TYPE distance( const LineSeg<DATA_TYPE>& lineseg,
                           const Point<DATA_TYPE, 3>& pt )
{
   return gmtl::length(gmtl::Vec<DATA_TYPE, 3>(pt - findNearestPt(lineseg, pt)));
}

/**
 * Computes the shortest distance from the line segment to the given point.
 *
 * @param lineseg    the line segment to test
 * @param pt         the point which to test against lineseg
 *
 * @return  the squared shortest distance from pt to lineseg (value is squared, this func is slightly faster since it doesn't involve a sqrt)
 */
template< class DATA_TYPE >
inline DATA_TYPE distanceSquared( const LineSeg<DATA_TYPE>& lineseg,
                           const Point<DATA_TYPE, 3>& pt )
{
   return gmtl::lengthSquared(gmtl::Vec<DATA_TYPE, 3>(pt - findNearestPt(lineseg, pt)));
}


} // namespace gmtl
#endif
