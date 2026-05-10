// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_DEFINES_H
#define _GMTL_DEFINES_H

namespace gmtl
{
   /** use the values in this enum to index vector data 
    *  types (such as Vec, Point, Quat).
    *
    * <h3> "Example (access elements in a Vec3f):" </h3>
    * \code
    *    Vec3f vec;
    *    vec[Xelt] = 1.0f;
    *    vec[Yelt] = 3.0f;
    *    vec[Zelt] = 2.0f;
    * \endcode
    * @ingroup Defines
    */
   enum VectorIndex { Xelt = 0, Yelt = 1, Zelt = 2, Welt = 3 };

   /**
    * Used to describe where a point lies in relationship to a plane.
    * ON_PLANE means the point lies on the plane.
    * POS_SIDE means the point lies on the same side as the surface normal.
    * NEG_SIDE means the point lies on the opposite side as the ssurface normal.
    * @ingroup Defines
    */
   enum PlaneSide
   {
      ON_PLANE,
      POS_SIDE,
      NEG_SIDE
   };

   /** @ingroup Defines
    * @name Constants
    * @{
    */    
   const float GMTL_EPSILON = 1.0e-6f;
   const float GMTL_MAT_EQUAL_EPSILON = 0.001f;  // Epsilon for matrices to be equal
   const float GMTL_VEC_EQUAL_EPSILON = 0.0001f; // Epsilon for vectors to be equal
   /** @} */
   
#define GMTL_NEAR(x,y,eps) (gmtl::Math::abs((x)-(y))<(eps))

}

// Platform-specific settings.
#if defined(__sun) || defined(__APPLE__) || defined(__hpux) ||  \
    defined(_XOPEN_SOURCE)
#define NO_ACOSF 1
#define NO_ASINF 1
#define NO_TANF 1
#define NO_ATAN2F 1
#define NO_COSF 1
#define NO_SINF 1
#define NO_TANF 1
#define NO_SQRTF 1
#define NO_LOGF 1
#define NO_EXPF 1
#define NO_POWF 1
#define NO_CEILF 1
#define NO_FLOORF 1
#endif

#if defined(_MSC_VER) && _MSC_VER < 1310
#define GMTL_NO_METAPROG
#endif


#endif
