// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_QUAT_OPS_H_
#define _GMTL_QUAT_OPS_H_

#include <gmtl/Math.h>
#include <gmtl/Quat.h>

namespace gmtl
{
/** @ingroup Ops Quat 
 * @name Quat Operations
 * @{
 */
       
   /** product of two quaternions (quaternion product)
    *  multiplication of quats is much like multiplication of typical complex numbers.
    *  @post q1q2 = (s1 + v1)(s2 + v2)
    *  @post result = q1 * q2 (where q2 would be applied first to any xformed geometry)
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& mult( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      // Here is the easy to understand equation: (grassman product)
      // scalar_component = q1.s * q2.s - dot(q1.v, q2.v)
      // vector_component = q2.v * q1.s + q1.v * q2.s + cross(q1.v, q2.v)

      // Here is another version (euclidean product, just FYI)...
      // scalar_component = q1.s * q2.s + dot(q1.v, q2.v)
      // vector_component = q2.v * q1.s - q1.v * q2.s - cross(q1.v, q2.v)

      // Here it is, using vector algebra (grassman product)
      /*
      const float& w1( q1[Welt] ), w2( q2[Welt] );
      Vec3 v1( q1[Xelt], q1[Yelt], q1[Zelt] ), v2( q2[Xelt], q2[Yelt], q2[Zelt] );

      float w = w1 * w2 - v1.dot( v2 );
      Vec3 v = (w1 * v2) + (w2 * v1) + v1.cross( v2 );

      vec[Welt] = w;
      vec[Xelt] = v[0];
      vec[Yelt] = v[1];
      vec[Zelt] = v[2];
      */

      // Here is the same, only expanded... (grassman product)
      Quat<DATA_TYPE> temporary; // avoid aliasing problems...
      temporary[Xelt] = q1[Welt]*q2[Xelt] + q1[Xelt]*q2[Welt] + q1[Yelt]*q2[Zelt] - q1[Zelt]*q2[Yelt];
      temporary[Yelt] = q1[Welt]*q2[Yelt] + q1[Yelt]*q2[Welt] + q1[Zelt]*q2[Xelt] - q1[Xelt]*q2[Zelt];
      temporary[Zelt] = q1[Welt]*q2[Zelt] + q1[Zelt]*q2[Welt] + q1[Xelt]*q2[Yelt] - q1[Yelt]*q2[Xelt];
      temporary[Welt] = q1[Welt]*q2[Welt] - q1[Xelt]*q2[Xelt] - q1[Yelt]*q2[Yelt] - q1[Zelt]*q2[Zelt];

      // use a temporary, in case q1 or q2 is the same as self.
      result[Xelt] = temporary[Xelt];
      result[Yelt] = temporary[Yelt];
      result[Zelt] = temporary[Zelt];
      result[Welt] = temporary[Welt];

      // don't normalize, because it might not be rotation arithmetic we're doing
      // (only rotation quats have unit length)
      return result;
   }

   /** product of two quaternions (quaternion product)
    *  Does quaternion multiplication.
    *  @post temp' = q1 * q2 (where q2 would be applied first to any xformed geometry)
    *  @see Quat
    *  @todo metaprogramming on quat operator*()
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE> operator*( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      // (grassman product - see mult() for discussion)
      // don't normalize, because it might not be rotation arithmetic we're doing
      // (only rotation quats have unit length)
      return Quat<DATA_TYPE>( q1[Welt]*q2[Xelt] + q1[Xelt]*q2[Welt] + q1[Yelt]*q2[Zelt] - q1[Zelt]*q2[Yelt],
                              q1[Welt]*q2[Yelt] + q1[Yelt]*q2[Welt] + q1[Zelt]*q2[Xelt] - q1[Xelt]*q2[Zelt],
                              q1[Welt]*q2[Zelt] + q1[Zelt]*q2[Welt] + q1[Xelt]*q2[Yelt] - q1[Yelt]*q2[Xelt],
                              q1[Welt]*q2[Welt] - q1[Xelt]*q2[Xelt] - q1[Yelt]*q2[Yelt] - q1[Zelt]*q2[Zelt] );
   }

   /** quaternion postmult
    * @post result' = result * q2 (where q2 is applied first to any xformed geometry)
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& operator*=( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q2 )
   {
      return mult( result, result, q2 );
   }

   /** Vector negation - negate each element in the quaternion vector.
    * the negative of a rotation quaternion is geometrically equivelent
    * to the original. there exist 2 quats for every possible rotation.
    * @return returns the negation of the given quat.
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& negate( Quat<DATA_TYPE>& result )
   {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
      return result;
   }

   /** Vector negation - (operator-) return a temporary that is the negative of the given quat.
    * the negative of a rotation quaternion is geometrically equivelent
    * to the original. there exist 2 quats for every possible rotation.
    * @return returns the negation of the given quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE> operator-( const Quat<DATA_TYPE>& quat )
   {
      return Quat<DATA_TYPE>( -quat[0], -quat[1], -quat[2], -quat[3] );
   }

   /** vector scalar multiplication
    * @post result' = [qx*s, qy*s, qz*s, qw*s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& mult( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q, DATA_TYPE s )
   {
      result[0] = q[0] * s;
      result[1] = q[1] * s;
      result[2] = q[2] * s;
      result[3] = q[3] * s;
      return result;
   }

   /** vector scalar multiplication
    * @post result' = [qx*s, qy*s, qz*s, qw*s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE> operator*( const Quat<DATA_TYPE>& q, DATA_TYPE s )
   {
      Quat<DATA_TYPE> temporary;
      return mult( temporary, q, s );
   }

   /** vector scalar multiplication
    * @post result' = [resultx*s, resulty*s, resultz*s, resultw*s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& operator*=( Quat<DATA_TYPE>& q, DATA_TYPE s )
   {
      return mult( q, q, s );
   }

   /** quotient of two quaternions
    * @post result = q1 * (1/q2)  (where 1/q2 is applied first to any xform'd geometry)
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& div( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q1, Quat<DATA_TYPE> q2 )
   {
      // multiply q1 by the multiplicative inverse of the quaternion
      return mult( result, q1, invert( q2 ) );
   }

   /** quotient of two quaternions
    * @post result = q1 * (1/q2) (where 1/q2 is applied first to any xform'd geometry)
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE> operator/( const Quat<DATA_TYPE>& q1, Quat<DATA_TYPE> q2 )
   {
      return q1 * invert( q2 );
   }

   /** quotient of two quaternions
    * @post result = result * (1/q2)  (where 1/q2 is applied first to any xform'd geometry)
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& operator/=( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q2 )
   {
      return div( result, result, q2 );
   }

   /** quaternion vector scale
    * @post result = q / s
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& div( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q, DATA_TYPE s )
   {
      result[0] = q[0] / s;
      result[1] = q[1] / s;
      result[2] = q[2] / s;
      result[3] = q[3] / s;
      return result;
   }

   /** vector scalar division
    * @post result' = [qx/s, qy/s, qz/s, qw/s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE> operator/( const Quat<DATA_TYPE>& q, DATA_TYPE s )
   {
      Quat<DATA_TYPE> temporary;
      return div( temporary, q, s );
   }

   /** vector scalar division
    * @post result' = [resultx/s, resulty/s, resultz/s, resultw/s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& operator/=( const Quat<DATA_TYPE>& q, DATA_TYPE s )
   {
      return div( q, q, s );
   }

   /** vector addition
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& add( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      result[0] = q1[0] + q2[0];
      result[1] = q1[1] + q2[1];
      result[2] = q1[2] + q2[2];
      result[3] = q1[3] + q2[3];
      return result;
   }

   /** vector addition
    * @post result' = [qx+s, qy+s, qz+s, qw+s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE> operator+( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      Quat<DATA_TYPE> temporary;
      return add( temporary, q1, q2 );
   }

   /** vector addition
    * @post result' = [resultx+s, resulty+s, resultz+s, resultw+s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& operator+=( Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      return add( q1, q1, q2 );
   }

   /** vector subtraction
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& sub( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      result[0] = q1[0] - q2[0];
      result[1] = q1[1] - q2[1];
      result[2] = q1[2] - q2[2];
      result[3] = q1[3] - q2[3];
      return result;
   }

   /** vector subtraction
    * @post result' = [qx-s, qy-s, qz-s, qw-s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE> operator-( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      Quat<DATA_TYPE> temporary;
      return sub( temporary, q1, q2 );
   }

   /** vector subtraction
    * @post result' = [resultx-s, resulty-s, resultz-s, resultw-s]
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& operator-=( Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      return sub( q1, q1, q2 );
   }

   /** vector dot product between two quaternions.
    *  get the lengthSquared between two quat vectors...
    *  @post N(q) = x1*x2 + y1*y2 + z1*z2 + w1*w2
    *  @return dot product of q1 and q2
    *  @see Quat
    */
   template <typename DATA_TYPE>
   DATA_TYPE dot( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      return DATA_TYPE( (q1[0] * q2[0]) +
                        (q1[1] * q2[1]) +
                        (q1[2] * q2[2]) +
                        (q1[3] * q2[3])  );
   }

   /** quaternion "norm" (also known as vector length squared)
    *  using this can be faster than using length for some operations...
    *  @post returns the vector length squared
    *  @post N(q) = x^2 + y^2 + z^2 + w^2
    *  @post result = x*x + y*y + z*z + w*w
    *  @see Quat
    */
   template <typename DATA_TYPE>
   DATA_TYPE lengthSquared( const Quat<DATA_TYPE>& q )
   {
      return dot( q, q );
   }

   /** quaternion "absolute" (also known as vector length or magnitude)
    *  using this can be faster than using length for some operations...
    *  @post returns the magnitude of the 4D vector.
    *  @post result = sqrt( lengthSquared( q ) )
    *  @see Quat
    */
   template <typename DATA_TYPE>
   DATA_TYPE length( const Quat<DATA_TYPE>& q )
   {
      return Math::sqrt( lengthSquared( q ) );
   }

   /** set self to the normalized quaternion of self.
    *  @pre magnitude should be > 0, otherwise no calculation is done.
    *  @post result' = normalize( result ), where normalize makes length( result ) == 1
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& normalize( Quat<DATA_TYPE>& result )
   {
      DATA_TYPE l = length( result );

      // return if no magnitude (already as normalized as possible)
      if (l < (DATA_TYPE)0.0001)
         return result;

      DATA_TYPE l_inv = ((DATA_TYPE)1.0) / l;
      result[Xelt] *= l_inv;
      result[Yelt] *= l_inv;
      result[Zelt] *= l_inv;
      result[Welt] *= l_inv;

      return result;
   }

   /**
    * Determines if the given quaternion is normalized within the given tolerance. The
    * quaternion is normalized if its lengthSquared is 1.
    *
    * @param q1      the quaternion to test
    * @param eps     the epsilon tolerance
    *
    * @return  true if the quaternion is normalized, false otherwise
    */
   template< typename DATA_TYPE >
   bool isNormalized( const Quat<DATA_TYPE>& q1, const DATA_TYPE eps = 0.0001f )
   {
      return Math::isEqual( lengthSquared( q1 ), DATA_TYPE(1), eps );
   }

   /** quaternion complex conjugate.
    *  @post set result to the complex conjugate of result.
    *  @post q* = [s,-v]
    *  @post result'[x,y,z,w] == result[-x,-y,-z,w]
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& conj( Quat<DATA_TYPE>& result )
   {
      result[Xelt] = -result[Xelt];
      result[Yelt] = -result[Yelt];
      result[Zelt] = -result[Zelt];
      return result;
   }

   /** quaternion multiplicative inverse.
    *  @post self becomes the multiplicative inverse of self
    *  @post 1/q = q* / N(q)
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& invert( Quat<DATA_TYPE>& result )
   {
      // from game programming gems p198
      // do result = conj( q ) / norm( q )
      conj( result );

      // return if norm() is near 0 (divide by 0 would result in NaN)
      DATA_TYPE l = lengthSquared( result );
      if (l < (DATA_TYPE)0.0001)
         return result;

      DATA_TYPE l_inv = ((DATA_TYPE)1.0) / l;
      result[Xelt] *= l_inv;
      result[Yelt] *= l_inv;
      result[Zelt] *= l_inv;
      result[Welt] *= l_inv;
      return result;
   }

   /** complex exponentiation.
    *  @pre safe to pass self as argument
    *  @post sets self to the exponentiation of quat
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& exp( Quat<DATA_TYPE>& result )
   {
      DATA_TYPE len1, len2;

      len1 = Math::sqrt( result[Xelt] * result[Xelt] +
                         result[Yelt] * result[Yelt] +
                         result[Zelt] * result[Zelt] );
      if (len1 > (DATA_TYPE)0.0)
         len2 = Math::sin( len1 ) / len1;
      else
         len2 = (DATA_TYPE)1.0;

      result[Xelt] = result[Xelt] * len2;
      result[Yelt] = result[Yelt] * len2;
      result[Zelt] = result[Zelt] * len2;
      result[Welt] = Math::cos( len1 );

      return result;
   }

   /** complex logarithm
    *  @post sets self to the log of quat
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& log( Quat<DATA_TYPE>& result )
   {
      DATA_TYPE length;

      length = Math::sqrt( result[Xelt] * result[Xelt] +
                           result[Yelt] * result[Yelt] +
                           result[Zelt] * result[Zelt] );

      // avoid divide by 0
      if (Math::isEqual( result[Welt], (DATA_TYPE)0.0, (DATA_TYPE)0.00001 ) == false)
         length = Math::aTan( length / result[Welt] );
      else
         length = Math::PI_OVER_2;

      result[Welt] = (DATA_TYPE)0.0;
      result[Xelt] = result[Xelt] * length;
      result[Yelt] = result[Yelt] * length;
      result[Zelt] = result[Zelt] * length;
      return result;
   }
   
   /** WARNING: not implemented (do not use) */
   template <typename DATA_TYPE>
   void squad( Quat<DATA_TYPE>& result, DATA_TYPE t, const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2, const Quat<DATA_TYPE>& a, const Quat<DATA_TYPE>& b )
   {
      gmtlASSERT( false );
   }

   /** WARNING: not implemented (do not use) */
   template <typename DATA_TYPE>
   void meanTangent( Quat<DATA_TYPE>& result, const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2, const Quat<DATA_TYPE>& q3 )
   {
       gmtlASSERT( false );
   }


   
/** @} */
   
/** @ingroup Interp Quat 
 * @name Quaternion Interpolation
 * @{
 */


   /** spherical linear interpolation between two rotation quaternions.
    *  t is a value between 0 and 1 that interpolates between from and to.
    * @pre no aliasing problems to worry about ("result" can be "from" or "to" param).
    * @param adjustSign - If true, then slerp will operate by adjusting the sign of the slerp to take shortest path
    *
    * References:
    * <ul>
    * <li> From Adv Anim and Rendering Tech. Pg 364
    * </ul>
    * @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& slerp( Quat<DATA_TYPE>& result, const DATA_TYPE t, const Quat<DATA_TYPE>& from, const Quat<DATA_TYPE>& to, bool adjustSign=true)
   {
      const Quat<DATA_TYPE>& p = from; // just an alias to match q

      // calc cosine theta
      DATA_TYPE cosom = dot( from, to );

      // adjust signs (if necessary)
      Quat<DATA_TYPE> q;
      if (adjustSign && (cosom < (DATA_TYPE)0.0))
      {
         cosom = -cosom;
         q[0] = -to[0];   // Reverse all signs
         q[1] = -to[1];
         q[2] = -to[2];
         q[3] = -to[3];
      }
      else
      {
         q = to;
      }

      // Calculate coefficients
      DATA_TYPE sclp, sclq;
      if (((DATA_TYPE)1.0 - cosom) > (DATA_TYPE)0.0001) // 0.0001 -> some epsillon
      {
         // Standard case (slerp)
         DATA_TYPE omega, sinom;
         omega = gmtl::Math::aCos( cosom ); // extract theta from dot product's cos theta
         sinom = gmtl::Math::sin( omega );
         sclp  = gmtl::Math::sin( ((DATA_TYPE)1.0 - t) * omega ) / sinom;
         sclq  = gmtl::Math::sin( t * omega ) / sinom;
      }
      else
      {
         // Very close, do linear interp (because it's faster)
         sclp = (DATA_TYPE)1.0 - t;
         sclq = t;
      }

      result[Xelt] = sclp * p[Xelt] + sclq * q[Xelt];
      result[Yelt] = sclp * p[Yelt] + sclq * q[Yelt];
      result[Zelt] = sclp * p[Zelt] + sclq * q[Zelt];
      result[Welt] = sclp * p[Welt] + sclq * q[Welt];
      return result;
   }

   /** linear interpolation between two quaternions.
    *  t is a value between 0 and 1 that interpolates between from and to.
    * @pre no aliasing problems to worry about ("result" can be "from" or "to" param).
    * References:
    * <ul>
    * <li> From Adv Anim and Rendering Tech. Pg 364
    * </ul>
    *  @see Quat
    */
   template <typename DATA_TYPE>
   Quat<DATA_TYPE>& lerp( Quat<DATA_TYPE>& result, const DATA_TYPE t, const Quat<DATA_TYPE>& from, const Quat<DATA_TYPE>& to)
   {
      // just an alias to match q
      const Quat<DATA_TYPE>& p = from;

      // calc cosine theta
      DATA_TYPE cosom = dot( from, to );

      // adjust signs (if necessary)
      Quat<DATA_TYPE> q;
      if (cosom < (DATA_TYPE)0.0)
      {
         q[0] = -to[0];   // Reverse all signs
         q[1] = -to[1];
         q[2] = -to[2];
         q[3] = -to[3];
      }
      else
      {
         q = to;
      }

      // do linear interp
      DATA_TYPE sclp, sclq;
      sclp = (DATA_TYPE)1.0 - t;
      sclq = t;

      result[Xelt] = sclp * p[Xelt] + sclq * q[Xelt];
      result[Yelt] = sclp * p[Yelt] + sclq * q[Yelt];
      result[Zelt] = sclp * p[Zelt] + sclq * q[Zelt];
      result[Welt] = sclp * p[Welt] + sclq * q[Welt];
      return result;
   }

/** @} */

/** @ingroup Compare Quat 
 *  @name Quat Comparisons
 * @{
 */

   /** Compare two quaternions for equality.
    * @see isEqual( Quat, Quat )
    */
   template <typename DATA_TYPE>
   inline bool operator==( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      return bool( q1[0] == q2[0] &&
                   q1[1] == q2[1] &&
                   q1[2] == q2[2] &&
                   q1[3] == q2[3]  );
   }

   /** Compare two quaternions for not-equality.
    * @see isEqual( Quat, Quat )
    */
   template <typename DATA_TYPE>
   inline bool operator!=( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2 )
   {
      return !operator==( q1, q2 );
   }

   /** Compare two quaternions for equality with tolerance.
    */
   template <typename DATA_TYPE>
   bool isEqual( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2, DATA_TYPE tol = 0.0 )
   {
      return bool( Math::isEqual( q1[0], q2[0], tol ) &&
                   Math::isEqual( q1[1], q2[1], tol ) &&
                   Math::isEqual( q1[2], q2[2], tol ) &&
                   Math::isEqual( q1[3], q2[3], tol )    );
   }

   /** Compare two quaternions for geometric equivelence (with tolerance).
    * there exist 2 quats for every possible rotation: the original,
    * and its negative.  the negative of a rotation quaternion is geometrically
    * equivelent to the original.
    */
   template <typename DATA_TYPE>
   bool isEquiv( const Quat<DATA_TYPE>& q1, const Quat<DATA_TYPE>& q2, DATA_TYPE tol = 0.0 )
   {
      return bool( isEqual( q1, q2, tol ) || isEqual( q1, -q2, tol ) );
   }
   
   /** @} */
}

#endif
