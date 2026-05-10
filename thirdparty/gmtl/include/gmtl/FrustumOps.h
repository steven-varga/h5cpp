// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_FRUSTUM_OPS_H_
#define _GMTL_FRUSTUM_OPS_H_

#include <gmtl/Defines.h>
#include <gmtl/Frustum.h>
#include <gmtl/Math.h>


namespace gmtl
{

template<class DATA_TYPE>
void normalize(Frustum<DATA_TYPE>& f)
{
   for ( unsigned int i = 0; i < 6; ++i )
   {
      Vec<DATA_TYPE, 3> n = f.mPlanes[i].getNormal();
      DATA_TYPE o = f.mPlanes[i].getOffset();
      DATA_TYPE len = Math::sqrt( n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
      n[0] /= len;
      n[1] /= len;
      n[2] /= len;
      o /= len;
      f.mPlanes[i].setNormal(n);
      f.mPlanes[i].setOffset(o);
   }
}

}


#endif /* _GMTL_FRUSTUM_OPS_H_ */
