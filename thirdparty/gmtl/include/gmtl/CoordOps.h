// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_COORD_OPS_H_
#define _GMTL_COORD_OPS_H_

#include <gmtl/Coord.h>

namespace gmtl
{
/** @ingroup Compare Coord 
 * @name Coord Comparitors
 * @{
 */

   /** Compare two coordinate frames for equality.
    * @param c1   the first Coord
    * @param c2   the second Coord
    * @return     true if c1 is the same as c2, false otherwise
    */
   template <typename POS_TYPE, typename ROT_TYPE>
   inline bool operator==( const Coord<POS_TYPE, ROT_TYPE>& c1, 
                           const Coord<POS_TYPE, ROT_TYPE>& c2 )
   {
      return bool( c1.getPos() == c2.getPos() &&
                   c1.getRot() == c2.getRot() );
   }

   /** Compare two coordinate frames for not-equality.
    * @param c1   the first Coord
    * @param c2   the second Coord
    * @return     true if c1 is different from c2, false otherwise
    */
   template <typename POS_TYPE, typename ROT_TYPE>
   inline bool operator!=( const Coord<POS_TYPE, ROT_TYPE>& c1, 
                           const Coord<POS_TYPE, ROT_TYPE>& c2 )
   {
      return !operator==( c1, c2 );
   }

   /** Compare two coordinate frames for equality with a given tolerance.
    * @param c1   the first Coord
    * @param c2   the second Coord
    * @param tol  the tolerance coordinate frame of the same type as c1 and c2
    * @return     true if c1 is equal within a tolerance of c2, false otherwise
    */
   template <typename POS_TYPE, typename ROT_TYPE>
   inline bool isEqual( const Coord<POS_TYPE, ROT_TYPE>& c1, 
                        const Coord<POS_TYPE, ROT_TYPE>& c2, 
                        typename Coord<POS_TYPE, ROT_TYPE>::DataType tol = 0 )
   {
      return bool( isEqual( c1.getPos(), c2.getPos(), tol ) &&
                   isEqual( c1.getRot(), c2.getRot(), tol )     );
   }
/** @} */

}

#endif
