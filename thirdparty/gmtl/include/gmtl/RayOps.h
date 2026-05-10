// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_RAYOPS_H_
#define _GMTL_RAYOPS_H_

#include <gmtl/Ray.h>

namespace gmtl {

//--- Ray Comparitor ---//
/**
 * Compare two line segments to see if they are EXACTLY the same.
 *
 * @param ls1     the first Ray to compare
 * @param ls2     the second Ray to compare
 *
 * @return  true if they are equal, false otherwise
 */
template< class DATA_TYPE >
inline bool operator==( const Ray<DATA_TYPE>& ls1, const Ray<DATA_TYPE>& ls2 )
{
   return ( (ls1.mOrigin == ls2.mOrigin) && (ls1.mDir == ls2.mDir) );
}

/**
 * Compare two line segments to see if they are not EXACTLY the same.
 *
 * @param ls1     the first Ray to compare
 * @param ls2     the second Ray to compare
 *
 * @return  true if they are not equal, false otherwise
 */
template< class DATA_TYPE >
inline bool operator!=( const Ray<DATA_TYPE>& ls1,
                        const Ray<DATA_TYPE>& ls2 )
{
   return ( ! (ls1 == ls2) );
}

/**
 * Compare two line segments to see if the are the same within the given
 * tolerance.
 *
 * @param ls1     the first Ray to compare
 * @param ls2     the second Ray to compare
 * @param eps     the tolerance value to use
 *
 * @pre eps must be >= 0
 *
 * @return  true if they are equal within the tolerance, false otherwise
 */
template< class DATA_TYPE >
inline bool isEqual( const Ray<DATA_TYPE>& ls1,
                     const Ray<DATA_TYPE>& ls2,
                     const DATA_TYPE& eps )
{
   gmtlASSERT( eps >= 0 );
   return ( (isEqual(ls1.mOrigin, ls2.mOrigin, eps)) &&
            (isEqual(ls1.mDir, ls2.mDir, eps)) );
}

} // namespace gmtl
#endif
