// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_TRIOPS_H_
#define _GMTL_TRIOPS_H_

#include <gmtl/Tri.h>
#include <gmtl/Generate.h>
#include <gmtl/VecOps.h>

namespace gmtl
{
/** @ingroup Ops Tri
 *  @name Triangle Operations
 * @{
 */

/**
 * Computes the point at the center of the given triangle.
 *
 * @param tri     the triangle to find the center of
 *
 * @return  the point at the center of the triangle
 */
template< class DATA_TYPE >
Point<DATA_TYPE, 3> center( const Tri<DATA_TYPE>& tri )
{
   const float one_third = (1.0f/3.0f);
   return (tri[0] + tri[1] + tri[2]) * DATA_TYPE(one_third);
}

/**
 * Computes the normal for this triangle.
 *
 * @param tri     the triangle for which to compute the normal
 *
 * @return  the normal vector for tri
 */
template< class DATA_TYPE >
Vec<DATA_TYPE, 3> normal( const Tri<DATA_TYPE>& tri )
{
   Vec<DATA_TYPE, 3> normal = makeCross( gmtl::Vec<DATA_TYPE,3>(tri[1] - tri[0]), gmtl::Vec<DATA_TYPE,3>(tri[2] - tri[0]) );
   normalize( normal );
   return normal;
}
/** @} */

/** @ingroup Compare Tri
 *  @name Triangle Comparitors
 *  @{
 */

/**
 * Compare two triangles to see if they are EXACTLY the same.
 *
 * @param tri1    the first triangle to compare
 * @param tri2    the second triangle to compare
 *
 * @return  true if they are equal, false otherwise
 */
template< class DATA_TYPE >
bool operator==( const Tri<DATA_TYPE>& tri1, const Tri<DATA_TYPE>& tri2 )
{
   return ( (tri1[0] == tri2[0]) &&
            (tri1[1] == tri2[1]) &&
            (tri1[2] == tri2[2]) );
}

/**
 * Compare two triangle to see if they are not EXACTLY the same.
 *
 * @param tri1    the first triangle to compare
 * @param tri2    the second triangle to compare
 *
 * @return  true if they are not equal, false otherwise
 */
template< class DATA_TYPE >
bool operator!=( const Tri<DATA_TYPE>& tri1, const Tri<DATA_TYPE>& tri2 )
{
   return (! (tri1 == tri2));
}

/**
 * Compare two triangles to see if they are the same within the given tolerance.
 *
 * @param tri1    the first triangle to compare
 * @param tri2    the second triangle to compare
 * @param eps     the tolerance value to use
 *
 * @pre  eps must be >= 0
 *
 * @return  true if they are equal within the tolerance, false otherwise
 */
template< class DATA_TYPE >
bool isEqual( const Tri<DATA_TYPE>& tri1, const Tri<DATA_TYPE>& tri2,
              const DATA_TYPE& eps )
{
   gmtlASSERT( eps >= 0 );
   return ( isEqual(tri1[0], tri2[0], eps) &&
            isEqual(tri1[1], tri2[1], eps) &&
            isEqual(tri1[2], tri2[2], eps) );
}
/** @} */

} // namespace gmtl

#endif

