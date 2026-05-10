// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_EULER_ANGLE_OPS_H_
#define _GMTL_EULER_ANGLE_OPS_H_

#include <gmtl/EulerAngle.h>

namespace gmtl
{

/** @ingroup Compare
 *  @name EulerAngle Comparitors
 *  @{
 */

/**
 * Compares 2 EulerAngles (component-wise) to see if they are exactly the same.
 *
 * @param e1   the first EulerAngle
 * @param e2   the second EulerAngle
 *
 * @return  true if e1 equals e2; false if they differ
 */
template<class DATA_TYPE, typename ROT_ORDER>
inline bool operator==(const EulerAngle<DATA_TYPE, ROT_ORDER>& e1,
                       const EulerAngle<DATA_TYPE, ROT_ORDER>& e2)
{
   // @todo metaprogramming.
   if (e1[0] != e2[0]) return false;
   if (e1[1] != e2[1]) return false;
   if (e1[2] != e2[2]) return false;
   return true;
}

/**
 * Compares e1 and e2 (component-wise) to see if they are NOT exactly the same.
 *
 * @param e1   the first EulerAngle
 * @param e2   the second EulerAngle
 *
 * @return  true if e1 does not equal e2; false if they are equal
 */
template<class DATA_TYPE, typename ROT_ORDER>
inline bool operator!=(const EulerAngle<DATA_TYPE, ROT_ORDER>& e1,
                       const EulerAngle<DATA_TYPE, ROT_ORDER>& e2)
{
   return(! (e1 == e2));
}

/**
 * Compares e1 and e2 (component-wise) to see if they are the same within a
 * given tolerance.
 *
 * @pre eps must be >= 0
 *
 * @param e1   the first EulerAngle
 * @param e2   the second EulerAngle
 * @param eps  the epsilon tolerance value, in radians
 *
 * @return  true if e1 is within the tolerance of e2; false if not
 */
template<class DATA_TYPE, typename ROT_ORDER>
inline bool isEqual( const EulerAngle<DATA_TYPE, ROT_ORDER>& e1,
                     const EulerAngle<DATA_TYPE, ROT_ORDER>& e2,
                     const DATA_TYPE eps = 0 )
{
   gmtlASSERT(eps >= (DATA_TYPE)0);
   
   // @todo metaprogramming.
   if (!Math::isEqual( e1[0], e2[0], eps )) return false;
   if (!Math::isEqual( e1[1], e2[1], eps )) return false;
   if (!Math::isEqual( e1[2], e2[2], eps )) return false;
   return true;
}

// @todo write isEquiv function for EulerAngle


/** @} */

} // namespace

#endif
