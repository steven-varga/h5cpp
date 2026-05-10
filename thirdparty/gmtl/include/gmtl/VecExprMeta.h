// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_VEC_EXPR_META_H
#define _GMTL_VEC_EXPR_META_H

#include <gmtl/Util/Meta.h>
#include <gmtl/VecOpsMeta.h>
#include <gmtl/VecBase.h>

/** Expression template classes for vec operations
 */
namespace gmtl
{
namespace meta
{
/** @ingroup VecExprMeta */
//@{


// ------------ Expression templates primitives ----------- //
// Expression templates for vectors
//
// NOTE: These could probably be optimized more in the future

// Concepts:
//
// VectorExpression:
//    types:
//    interface:
//       TYPE eval(unsigned i):  Returns evaluation of expression at elt i


/** template to hold a scalar argument. */
template <typename T>
struct ScalarArg
{
   typedef T DataType;

   const T mScalar;

   inline ScalarArg(const T scalar) : mScalar(scalar) {}
   inline T operator[](const unsigned) const
   { return mScalar; }
};

template <typename T>
inline ScalarArg<T> makeScalarArg(T val)
{ return ScalarArg<T>(val); }

// --- TRAITS --- //
/** Traits class for expression template parameters.
* NOTE: These types are VERY important to the performance of the code.
*    They allow the compiler to optimize (ie. eliminate) much code.
*/
template<typename T>
struct ExprTraits
{
   typedef const T ExprRef;     // Refer using a constant reference
};

template<typename T, unsigned SIZE>
struct ExprTraits< VecBase<T, SIZE, ScalarArg<T> > >
{
   typedef const VecBase<T,SIZE,ScalarArg<T> > ExprRef;
};

template<typename T, unsigned SIZE>
struct ExprTraits< VecBase<T, SIZE, DefaultVecTag> >
{
   typedef const VecBase<T,SIZE,DefaultVecTag>& ExprRef;
};


// -- Expressions -- //
/** Binary vector expression.
*
* Stores the two vector expressions to process.
*/
template <typename EXP1_T, typename EXP2_T, typename OP>
struct VecBinaryExpr
{
   typedef typename EXP1_T::DataType DataType;

   typename ExprTraits<EXP1_T>::ExprRef Exp1;
   typename ExprTraits<EXP2_T>::ExprRef Exp2;

   inline VecBinaryExpr(const EXP1_T& e1, const EXP2_T& e2) : Exp1(e1), Exp2(e2) {;}
   inline DataType operator[](const unsigned i) const
   { return OP::eval(Exp1[i], Exp2[i]); }
};

/** Unary vector expression.
* Holds the vector expression and unary operation to apply to it.
*/
template <typename EXP1_T, typename OP>
struct VecUnaryExpr
{
   typedef typename EXP1_T::DataType DataType;

   typename ExprTraits<EXP1_T>::ExprRef Exp1;

   inline VecUnaryExpr(const EXP1_T& e1) : Exp1(e1) {;}
   inline DataType operator[](const unsigned i) const
   { return OP::eval(Exp1[i]); }
};

// --- Operations --- //
/* Binary operations to add two vector expressions. */
struct VecPlusBinary
{
   template <typename T>
   static inline T eval(const T a1, const T a2)
   { return a1+a2; }
};

/* Binary operations to subtract two vector expressions. */
struct VecMinusBinary
{
   template <typename T>
   static inline T eval(const T a1, const T a2)
   { return a1-a2; }
};

struct VecMultBinary
{
   template<typename T>
   static inline T eval(const T a1, const T a2)
   { return a1 * a2; }
};

struct VecDivBinary
{
   template<typename T>
   static inline T eval(const T a1, const T a2)
   { return a1/a2; }
};

/** Negation of the values. */
struct VecNegUnary
{
   template <typename T>
   static inline T eval(const T a1)
   { return -a1; }
};


/*
template<typename T, unsigned SIZE, typename R1, typename R2>
inline VecBase<T,SIZE, VecBinaryExpr<VecBase<T,SIZE,R1>, VecBase<T,SIZE,R2>, VecPlusBinary> >
sum(const VecBase<T,SIZE,R1>& v1, const VecBase<T,SIZE,R2>& v2)
{
   return VecBase<T,SIZE,
               VecBinaryExpr<VecBase<T,SIZE,R1>,
                             VecBase<T,SIZE,R2>,
                             VecPlusBinary> >( VecBinaryExpr<VecBase<T,SIZE,R1>,
                                                             VecBase<T,SIZE,R2>,
                                                             VecPlusBinary>(v1,v2) );
}


template<typename T, unsigned SIZE, typename R1>
inline VecBase<T,SIZE, VecBinaryExpr<VecBase<T,SIZE,R1>, VecBase<T,SIZE,ScalarArg<T> >, VecPlusBinary> >
sum(const VecBase<T,SIZE,R1>& v1, const T& arg)
{
   return VecBase<T,SIZE,
               VecBinaryExpr<VecBase<T,SIZE,R1>,
                             VecBase<T,SIZE,ScalarArg<T> >,
                             VecPlusBinary> >( VecBinaryExpr<VecBase<T,SIZE,R1>,
                                                             VecBase<T,SIZE,ScalarArg<T> >,
                                                             VecPlusBinary>(v1,ScalarArg<T>(arg)) );
}
*/

//@}


} // namespace meta
} // end namespace


#endif
