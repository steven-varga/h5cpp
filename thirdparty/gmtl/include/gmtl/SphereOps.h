// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_SPHEREOPS_H_
#define _GMTL_SPHEREOPS_H_

#include <gmtl/Sphere.h>
#include <gmtl/VecOps.h>
#include <gmtl/Math.h>

namespace gmtl
{

/** @ingroup Compare Sphere 
 * @name Sphere Comparitors
 * @{
 */

/**
 * Compare two spheres to see if they are EXACTLY the same. 
 *
 * @param s1      the first sphere to compare
 * @param s2      the second sphere to compare
 *
 * @return  true if they are equal, false otherwise
 */
template< class DATA_TYPE >
inline bool operator==( const Sphere<DATA_TYPE>& s1, const Sphere<DATA_TYPE>& s2 )
{
   return ( (s1.mCenter == s2.mCenter) && (s1.mRadius == s2.mRadius) );
}

/**
 * Compare two spheres to see if they are not EXACTLY the same. 
 *
 * @param s1      the first sphere to compare
 * @param s2      the second sphere to compare
 *
 * @return  true if they are not equal, false otherwise
 */
template< class DATA_TYPE >
inline bool operator!=( const Sphere<DATA_TYPE>& s1, const Sphere<DATA_TYPE>& s2 )
{
   return (! (s1 == s2));
}

/**
 * Compare two spheres to see if they are the same within the given tolerance.
 *
 * @param s1      the first sphere to compare
 * @param s2      the second sphere to compare
 * @param eps     the tolerance value to use
 *
 * @pre eps must be >= 0
 *
 * @return  true if they are equal within a tolerance, false otherwise
 */
template< class DATA_TYPE >
inline bool isEqual( const Sphere<DATA_TYPE>& s1, const Sphere<DATA_TYPE>& s2, const DATA_TYPE& eps )
{
   gmtlASSERT( eps >= 0 );
   return ( (isEqual(s1.mCenter, s2.mCenter, eps)) &&
            (Math::isEqual(s1.mRadius, s2.mRadius, eps)) );
}
/** @} */

} // namespace gmtl

#endif

