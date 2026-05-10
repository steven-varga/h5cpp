// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_VEC_OPS_META_H
#define _GMTL_VEC_OPS_META_H

#include <gmtl/Util/Meta.h>

/** Meta programming classes for vec operations
 */
namespace gmtl
{
namespace meta
{
/** @ingroup VecOpsMeta */
//@{

/** meta class to unroll dot products. */
template<int ELT, typename T1, typename T2>
struct DotVecUnrolled
{
   static typename T1::DataType func(const T1& v1, const T2& v2)
   {  return (v1[ELT]*v2[ELT]) + DotVecUnrolled<ELT-1,T1,T2>::func(v1,v2); }
};

/** base cas for dot product unrolling. */
template<typename T1, typename T2>
struct DotVecUnrolled<0,T1,T2>
{
   static typename T1::DataType func(const T1& v1, const T2& v2)
   {   return (v1[0]*v2[0]); }
};

/** meta class to unroll length squared operation. */
template<int ELT, typename T>
struct LenSqrVecUnrolled
{
   static typename T::DataType func(const T& v)
   {  return (v[ELT]*v[ELT]) + LenSqrVecUnrolled<ELT-1,T>::func(v); }
};

/** base cas for dot product unrolling. */
template<typename T>
struct LenSqrVecUnrolled<0,T>
{
   static typename T::DataType func(const T& v)
   {   return (v[0]*v[0]); }
};

/** meta class to test vector equality. */
template<int ELT, typename VT>
struct EqualVecUnrolled
{
   static bool func(const VT& v1, const VT& v2)
   {  return (v1[ELT]==v2[ELT]) && EqualVecUnrolled<ELT-1,VT>::func(v1,v2); }
};

/** base cas for dot product unrolling. */
template<typename VT>
struct EqualVecUnrolled<0,VT>
{
   static bool func(const VT& v1, const VT& v2)
   {   return (v1[0]==v2[0]); }
};
//@}

} // namespace meta
} // end namespace


#endif
