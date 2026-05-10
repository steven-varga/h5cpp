// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_AXIS_ANGLE_OPS_H_
#define _GMTL_AXIS_ANGLE_OPS_H_

#include <gmtl/AxisAngle.h>

namespace gmtl
{

/** @ingroup Compare
 *  @name AxisAngle Comparitors
 *  @{
 */

/**
 * Compares 2 AxisAngles to see if they are exactly the same.
 *
 * @param a1   the first AxisAngle
 * @param a2   the second AxisAngle
 *
 * @return  true if a1 equals a2; false if they differ
 */
template<class DATA_TYPE>
inline bool operator==(const AxisAngle<DATA_TYPE>& a1,
                       const AxisAngle<DATA_TYPE>& a2)
{
   // @todo metaprogramming.
   if (a1[0] != a2[0]) return false;
   if (a1[1] != a2[1]) return false;
   if (a1[2] != a2[2]) return false;
   if (a1[3] != a2[3]) return false;
   return true;
}

/**
 * Compares 2 AxisAngles to see if they are NOT exactly the same.
 *
 * @param a1   the first AxisAngle
 * @param a2   the second AxisAngle
 *
 * @return  true if a1 does not equal a2; false if they are equal
 */
template<class DATA_TYPE>
inline bool operator!=(const AxisAngle<DATA_TYPE>& a1,
                       const AxisAngle<DATA_TYPE>& a2)
{
   return !(a1 == a2);
}

/**
 * Compares a1 and a2 to see if they are the same within the given epsilon
 * tolerance.
 *
 * @pre eps must be >= 0
 *
 * @param a1   the first vector
 * @param a2   the second vector
 * @param eps  the epsilon tolerance value
 *
 * @return  true if a1 equals a2 within tolerance; false if they differ
 */
template<class DATA_TYPE>
inline bool isEqual( const AxisAngle<DATA_TYPE>& a1,
                     const AxisAngle<DATA_TYPE>& a2, 
                     const DATA_TYPE eps = 0 )
{
   gmtlASSERT( eps >= (DATA_TYPE)0 );
   
   // @todo metaprogramming.
   if (!Math::isEqual( a1[0], a2[0], eps )) return false;
   if (!Math::isEqual( a1[1], a2[1], eps )) return false;
   if (!Math::isEqual( a1[2], a2[2], eps )) return false;
   if (!Math::isEqual( a1[3], a2[3], eps )) return false;
   return true;
}

// @todo write isEquiv function for AxisAngle


/** @} */

} // namespace

#endif
