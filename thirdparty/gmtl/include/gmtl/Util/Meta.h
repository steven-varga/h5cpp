// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_METAPROGRAMMING_H
#define _GMTL_METAPROGRAMMING_H

#include <gmtl/Defines.h>

/*** STRINGIZE and JOIN macros */
/* Taken from boost (see boost.org) */

//
// Helper macro GMTL_STRINGIZE:
// Converts the parameter X to a string after macro replacement
// on X has been performed.
//
#define GMTL_STRINGIZE(X) GMTL_DO_STRINGIZE(X)
#define GMTL_DO_STRINGIZE(X) #X

//
// Helper macro GMTL_JOIN:
// The following piece of macro magic joins the two
// arguments together, even when one of the arguments is
// itself a macro (see 16.3.1 in C++ standard).  The key
// is that macro expansion of macro arguments does not
// occur in BOOST_DO_JOIN2 but does in BOOST_DO_JOIN.
//
#define GMTL_JOIN( X, Y ) GMTL_DO_JOIN( X, Y )
#define GMTL_DO_JOIN( X, Y ) GMTL_DO_JOIN2(X,Y)
#define GMTL_DO_JOIN2( X, Y ) X##Y


/** Meta programming classes */
namespace gmtl
{
   /** @ingroup Meta */
   //@{

   /** A lightweight identifier you can pass to overloaded functions
    *  to typefy them.
    *
    *  Type2Type lets you transport the type information about T to functions
    */
   template <typename T>
   struct Type2Type
   {
      typedef T OriginalType;
   };

   //@}

   /** @ingroup HelperMeta */
   //@{
   template <class T> inline void ignore_unused_variable_warning(const T&) { }

   //@}

} // end namespace

#ifndef GMTL_NO_METAPROG
namespace gmtl
{
namespace meta
{

// ------ LOOP UnRolling ------------ //
template<int ELT, typename T>
struct AssignVecUnrolled
{
   static void func(T& lVec, const T& rVec)
   {
      AssignVecUnrolled<ELT-1,T>::func(lVec, rVec);
      lVec[ELT] = rVec[ELT];
   }
};

template<typename T>
struct AssignVecUnrolled<0,T>
{
   static void func(T& lVec, const T& rVec)
   { lVec[0] = rVec[0]; }
};

// Template programs for array assignment unrolled
template<int ELT, typename T>
struct AssignArrayUnrolled
{
   static void func(T* lVec, const T* rVec)
   {
      AssignArrayUnrolled<ELT-1,T>::func(lVec, rVec);
      lVec[ELT] = rVec[ELT];
   }
};

template<typename T>
struct AssignArrayUnrolled<0,T>
{
   static void func(T* lVec, const T* rVec)
   { lVec[0] = rVec[0]; }
};

}  // namespace meta
}  // namespace gmtl
#endif /* ! GMTL_NO_METAPROG */

#endif
