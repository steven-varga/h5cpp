// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_GENERATE_H_
#define _GMTL_GENERATE_H_

#include <gmtl/Defines.h>
#include <gmtl/Util/Assert.h>
#include <gmtl/Util/StaticAssert.h>

#include <gmtl/Vec.h>    // for Vec
#include <gmtl/VecOps.h> // for lengthSquared
#include <gmtl/Quat.h>
#include <gmtl/QuatOps.h>
#include <gmtl/Coord.h>
#include <gmtl/Matrix.h>
#include <gmtl/Util/Meta.h>
#include <gmtl/Math.h>
#include <gmtl/Xforms.h>

#include <gmtl/EulerAngle.h>
#include <gmtl/AxisAngle.h>

namespace gmtl
{
   /** @ingroup Generate
    *  @name Vec Generators
    *  @{
    */

   /** create a vector from the vector component of a quaternion
    * @returns a vector of the quaternion's axis. quat = [v,0] = [v0,v1,v2,0]
    */
   template <typename DATA_TYPE>
   inline Vec<DATA_TYPE, 3> makeVec( const Quat<DATA_TYPE>& quat )
   {
      return Vec<DATA_TYPE, 3>( quat[Xelt], quat[Yelt], quat[Zelt] );
   }

   /** create a normalized vector from the given vector.
    */
   template <typename DATA_TYPE, unsigned SIZE>
   inline Vec<DATA_TYPE, SIZE> makeNormal( Vec<DATA_TYPE, SIZE> vec )
   {
      normalize( vec );
      return vec;
   }

   /**
    * Computes the cross product between v1 and v2 and returns the result. Note
    * that this only applies to 3-dimensional vectors.
    *
    * @pre  v1 and v2 must be 3-D vectors
    * @post result = v1 x v2
    *
    * @param v1   the first vector
    * @param v2   the second vector
    *
    * @return  the result of the cross product between v1 and v2
    */
   template<class DATA_TYPE>
   Vec<DATA_TYPE,3> makeCross(const Vec<DATA_TYPE, 3>& v1,
                              const Vec<DATA_TYPE, 3>& v2)
   {
      return Vec<DATA_TYPE,3>( ((v1[Yelt]*v2[Zelt]) - (v1[Zelt]*v2[Yelt])),
                               ((v1[Zelt]*v2[Xelt]) - (v1[Xelt]*v2[Zelt])),
                               ((v1[Xelt]*v2[Yelt]) - (v1[Yelt]*v2[Xelt])) );
   }

   /** Set vector using translation portion of the matrix.
    * @pre if making an n x n matrix, then for
    *    - <b>vector is homogeneous:</b> SIZE of vector needs to equal number of Matrix ROWS - 1
    *    - <b>vector has scale component:</b> SIZE of vector needs to equal number of Matrix ROWS
    * if making an n x n+1 matrix, then for
    *    - <b>vector is homogeneous:</b> SIZE of vector needs to equal number of Matrix ROWS
    *    - <b>vector has scale component:</b> SIZE of vector needs to equal number of Matrix ROWS + 1
    * @post if preconditions are not met, then function is undefined (will not compile)
    */
   template<typename VEC_TYPE, typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline VEC_TYPE& setTrans( VEC_TYPE& result, const Matrix<DATA_TYPE, ROWS, COLS>& arg )
   {
      // ASSERT: There are as many

      // if n x n   then (homogeneous case) vecsize == rows-1 or (scale component case) vecsize == rows
      // if n x n+1 then (homogeneous case) vecsize == rows   or (scale component case) vecsize == rows+1
      gmtlASSERT( ((ROWS == COLS && ( VEC_TYPE::Size == (ROWS-1) ||  VEC_TYPE::Size == ROWS)) ||
               (COLS == (ROWS+1) && ( VEC_TYPE::Size == ROWS ||  VEC_TYPE::Size == (ROWS+1)))) &&
              "preconditions not met for vector size in call to makeTrans.  Read your documentation." );

      // homogeneous case...
      if ((ROWS == COLS &&  VEC_TYPE::Size == ROWS)              // Square matrix and vec so assume homogeneous vector. ex. 4x4 with vec 4
          || (COLS == (ROWS+1) &&  VEC_TYPE::Size == (ROWS+1)))  // ex: 3x4 with vec4
      {
         result[VEC_TYPE::Size-1] = 1.0f;
      }

      // non-homogeneous case... (SIZE == ROWS),
      //else
      //{}

      for (unsigned x = 0; x < COLS - 1; ++x)
      {
         result[x] = arg( x, COLS - 1 );
      }

      return result;
   }

   /** @} */

   /** @ingroup Generate
    *  @name Quat Generators
    *  @{
    */

   /** Set pure quaternion
   * @pre vec should be normalized
   * @param quat     any quaternion object
   * @param vec      a normalized vector representing an axis
   * @post quat will have vec as its axis, and no rotation
   */
   template <typename DATA_TYPE>
   inline Quat<DATA_TYPE>& setPure( Quat<DATA_TYPE>& quat, const Vec<DATA_TYPE, 3>& vec )
   {
      quat.set( vec[0], vec[1], vec[2], 0 );
      return quat;
   }

   /** create a pure quaternion
   * @pre vec should be normalized
   * @param vec      a normalized vector representing an axis
   * @return a quaternion with vec as its axis, and no rotation
   * @post quat = [v,0] = [v0,v1,v2,0]
   */
   template <typename DATA_TYPE>
   inline Quat<DATA_TYPE> makePure( const Vec<DATA_TYPE, 3>& vec )
   {
      return Quat<DATA_TYPE>( vec[0], vec[1], vec[2], 0 );
   }

   /** Normalize a quaternion
    * @param quat       a quaternion
    * @post quat is normalized
    */
   template <typename DATA_TYPE>
   inline Quat<DATA_TYPE> makeNormal( const Quat<DATA_TYPE>& quat )
   {
      Quat<DATA_TYPE> temporary( quat );
      return normalize( temporary );
   }

   /** quaternion complex conjugate.
    *  @param quat      any quaternion object
    *  @post set result to the complex conjugate of result.
    *  @post result'[x,y,z,w] == result[-x,-y,-z,w]
    *  @see Quat
    */
   template <typename DATA_TYPE>
   inline Quat<DATA_TYPE> makeConj( const Quat<DATA_TYPE>& quat )
   {
      Quat<DATA_TYPE> temporary( quat );
      return conj( temporary );
   }

   /** create quaternion from the inverse of another quaternion.
    *  @param quat      any quaternion object
    *  @return    a quaternion that is the multiplicative inverse of quat
    *  @see Quat
    */
   template <typename DATA_TYPE>
   inline Quat<DATA_TYPE> makeInvert( const Quat<DATA_TYPE>& quat )
   {
      Quat<DATA_TYPE> temporary( quat );
      return invert( temporary );
   }

   /** Convert an AxisAngle to a Quat.  sets a rotation quaternion from an angle and an axis.
    * @pre AxisAngle::axis must be normalized to length == 1 prior to calling this.
    * @post q = [ cos(rad/2), sin(rad/2) * [x,y,z] ]
    */
   template <typename DATA_TYPE>
   inline Quat<DATA_TYPE>& set( Quat<DATA_TYPE>& result, const AxisAngle<DATA_TYPE>& axisAngle )
   {
      gmtlASSERT( (Math::isEqual( lengthSquared( axisAngle.getAxis() ), (DATA_TYPE)1.0, (DATA_TYPE)0.0001 )) &&
                   "you must pass in a normalized vector to setRot( quat, rad, vec )" );

      DATA_TYPE half_angle = axisAngle.getAngle() * (DATA_TYPE)0.5;
      DATA_TYPE sin_half_angle = Math::sin( half_angle );

      result[Welt] = Math::cos( half_angle );
      result[Xelt] = sin_half_angle * axisAngle.getAxis()[0];
      result[Yelt] = sin_half_angle * axisAngle.getAxis()[1];
      result[Zelt] = sin_half_angle * axisAngle.getAxis()[2];

      // should automagically be normalized (unit magnitude) now...
      return result;
   }

   /** Redundant duplication of the set(quat,axisangle) function, this is provided only for template compatibility.
    *  unless you're writing template functions, you should use set(quat,axisangle).
    */
   template <typename DATA_TYPE>
   inline Quat<DATA_TYPE>& setRot( Quat<DATA_TYPE>& result, const AxisAngle<DATA_TYPE>& axisAngle )
   {
      return gmtl::set( result, axisAngle );
   }

   /** Convert an EulerAngle rotation to a Quaternion rotation.
    * Sets a rotation quaternion using euler angles (each angle in radians).
    * @pre pass in your angles in the same order as the RotationOrder you specify
    */
   template <typename DATA_TYPE, typename ROT_ORDER>
   inline Quat<DATA_TYPE>& set( Quat<DATA_TYPE>& result, const EulerAngle<DATA_TYPE, ROT_ORDER>& euler )
   {
      // this might be faster if put into the switch statement... (testme)
      const int& order = ROT_ORDER::ID;
      const DATA_TYPE xRot = (order == XYZ::ID) ? euler[0] : ((order == ZXY::ID) ? euler[1] : euler[2]);
      const DATA_TYPE yRot = (order == XYZ::ID) ? euler[1] : ((order == ZXY::ID) ? euler[2] : euler[1]);
      const DATA_TYPE zRot = (order == XYZ::ID) ? euler[2] : ((order == ZXY::ID) ? euler[0] : euler[0]);

      // this could be written better for each rotation order, but this is really general...
      Quat<DATA_TYPE> qx, qy, qz;

      // precompute half angles
      DATA_TYPE xOver2 = xRot * (DATA_TYPE)0.5;
      DATA_TYPE yOver2 = yRot * (DATA_TYPE)0.5;
      DATA_TYPE zOver2 = zRot * (DATA_TYPE)0.5;

      // set the pitch quat
      qx[Xelt] = Math::sin( xOver2 );
      qx[Yelt] = (DATA_TYPE)0.0;
      qx[Zelt] = (DATA_TYPE)0.0;
      qx[Welt] = Math::cos( xOver2 );

      // set the yaw quat
      qy[Xelt] = (DATA_TYPE)0.0;
      qy[Yelt] = Math::sin( yOver2 );
      qy[Zelt] = (DATA_TYPE)0.0;
      qy[Welt] = Math::cos( yOver2 );

      // set the roll quat
      qz[Xelt] = (DATA_TYPE)0.0;
      qz[Yelt] = (DATA_TYPE)0.0;
      qz[Zelt] = Math::sin( zOver2 );
      qz[Welt] = Math::cos( zOver2 );

      // compose the three in pyr order...
      switch (order)
      {
      case XYZ::ID: result = qx * qy * qz; break;
      case ZYX::ID: result = qz * qy * qx; break;
      case ZXY::ID: result = qz * qx * qy; break;
      default:
         gmtlASSERT( false && "unknown rotation order passed to setRot" );
         break;
      }

      // ensure the quaternion is normalized
      normalize( result );
      return result;
   }

   /** Redundant duplication of the set(quat,eulerangle) function, this is provided only for template compatibility.
    *  unless you're writing template functions, you should use set(quat,eulerangle).
    */
   template <typename DATA_TYPE, typename ROT_ORDER>
   inline Quat<DATA_TYPE>& setRot( Quat<DATA_TYPE>& result, const EulerAngle<DATA_TYPE, ROT_ORDER>& euler )
   {
      return gmtl::set( result, euler );
   }

   /** Convert a Matrix to a Quat.
    *  Sets the rotation quaternion using the given matrix
    *  (3x3, 3x4, 4x3, or 4x4 are all valid sizes).
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Quat<DATA_TYPE>& set( Quat<DATA_TYPE>& quat, const Matrix<DATA_TYPE, ROWS, COLS>& mat  )
   {
      gmtlASSERT( ((ROWS == 3 && COLS == 3) ||
               (ROWS == 3 && COLS == 4) ||
               (ROWS == 4 && COLS == 3) ||
               (ROWS == 4 && COLS == 4)) &&
               "pre conditions not met on set( quat, pos, mat ) which only sets a quaternion to the rotation part of a 3x3, 3x4, 4x3, or 4x4 matrix." );

      DATA_TYPE tr( mat( 0, 0 ) + mat( 1, 1 ) + mat( 2, 2 ) ), s( 0.0f );

      // If diagonal is positive
      if (tr > (DATA_TYPE)0.0)
      {
         s = Math::sqrt( tr + (DATA_TYPE)1.0 );
         quat[Welt] = s * (DATA_TYPE)0.5;
         s = DATA_TYPE(0.5) / s;

         quat[Xelt] = (mat( 2, 1 ) - mat( 1, 2 )) * s;
         quat[Yelt] = (mat( 0, 2 ) - mat( 2, 0 )) * s;
         quat[Zelt] = (mat( 1, 0 ) - mat( 0, 1 )) * s;
      }

      // when Diagonal is negative
      else
      {
         static const unsigned int nxt[3] = { 1, 2, 0 };
         unsigned int i( Xelt ), j, k;

         if (mat( 1, 1 ) > mat( 0, 0 ))
            i = 1;

         if (mat( 2, 2 ) > mat( i, i ))
            i = 2;

         j = nxt[i];
         k = nxt[j];

         s = Math::sqrt( (mat( i, i )-(mat( j, j )+mat( k, k ))) + (DATA_TYPE)1.0 );

         DATA_TYPE q[4];
         q[i] = s * (DATA_TYPE)0.5;

         if (s != (DATA_TYPE)0.0)
            s = DATA_TYPE(0.5) / s;

         q[3] = (mat( k, j ) - mat( j, k )) * s;
         q[j] = (mat( j, i ) + mat( i, j )) * s;
         q[k] = (mat( k, i ) + mat( i, k )) * s;

         quat[Xelt] = q[0];
         quat[Yelt] = q[1];
         quat[Zelt] = q[2];
         quat[Welt] = q[3];
      }

      return quat;
   }

   /** Redundant duplication of the set(quat,mat) function, this is provided only for template compatibility.
    *  unless you're writing template functions, you should use set(quat,mat).
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Quat<DATA_TYPE>& setRot( Quat<DATA_TYPE>& result, const Matrix<DATA_TYPE, ROWS, COLS>& mat )
   {
      return gmtl::set( result, mat );
   }
   /** @} */

   /** @ingroup Generate
    *  @name AxisAngle Generators
    *  @{
    */

   /** Convert a rotation quaternion to an AxisAngle.
    *
    * @post returns an angle in radians, and a normalized axis equivilent to the quaternion's rotation.
    * @post returns rad and xyz such that
    * <ul>
    * <li>  rad = acos( w ) * 2.0
    * <li>  vec = v / (asin( w ) * 2.0)   [where v is the xyz or vector component of the quat]
    * </ul>
    * @post axisAngle = quat;
    */
   template <typename DATA_TYPE>
   inline AxisAngle<DATA_TYPE>& set( AxisAngle<DATA_TYPE>& axisAngle, Quat<DATA_TYPE> quat )
   {
      // set sure we don't get a NaN result from acos...
      if (Math::abs( quat[Welt] ) > (DATA_TYPE)1.0)
      {
         gmtl::normalize( quat );
      }
      gmtlASSERT( Math::abs( quat[Welt] ) <= (DATA_TYPE)1.0 && "acos returns NaN when quat[Welt] > 1, try normalizing your quat." );

      // [acos( w ) * 2.0, v / (asin( w ) * 2.0)]

      // set the angle - aCos is mathematically defined to be between 0 and PI
      DATA_TYPE rad = Math::aCos( quat[Welt] ) * (DATA_TYPE)2.0;
      axisAngle.setAngle( rad );

      // set the axis: (use sin(rad) instead of asin(w))
      DATA_TYPE sin_half_angle = Math::sin( rad * (DATA_TYPE)0.5 );
      if (sin_half_angle >= (DATA_TYPE)0.0001) // because (PI >= rad >= 0)
      {
         DATA_TYPE sin_half_angle_inv = DATA_TYPE(1.0) / sin_half_angle;
         Vec<DATA_TYPE, 3> axis( quat[Xelt] * sin_half_angle_inv,
                                 quat[Yelt] * sin_half_angle_inv,
                                 quat[Zelt] * sin_half_angle_inv );
         normalize( axis );
         axisAngle.setAxis( axis );
      }

      // avoid NAN
      else
      {
         // one of the terms should be a 1,
         // so we can maintain unit-ness
         // in case w is 0 (which here w is 0)
         axisAngle.setAxis( gmtl::Vec<DATA_TYPE, 3>(
                             DATA_TYPE( 1.0 ) /*- gmtl::Math::abs( quat[Welt] )*/,
                            (DATA_TYPE)0.0,
                            (DATA_TYPE)0.0 ) );
      }
      return axisAngle;
   }

   /** Redundant duplication of the set(axisangle,quat) function, this is provided only for template compatibility.
    *  unless you're writing template functions, you should use set(axisangle,quat) for clarity.
    */
   template <typename DATA_TYPE>
   inline AxisAngle<DATA_TYPE>& setRot( AxisAngle<DATA_TYPE>& result, Quat<DATA_TYPE> quat )
   {
      return gmtl::set( result, quat );
   }

   /** make the axis of an AxisAngle normalized */
   template <typename DATA_TYPE>
   AxisAngle<DATA_TYPE> makeNormal( const AxisAngle<DATA_TYPE>& a )
   {
      return AxisAngle<DATA_TYPE>( a.getAngle(), makeNormal( a.getAxis() ) );
   }
   /** @} */

   /** @ingroup Generate
    *  @name EulerAngle Generators
    *  @{
    */

   /** Convert Matrix to EulerAngle.
    *  Set the Euler Angle from the given rotation portion (3x3) of the matrix.
    *
    * @param input  order, mat
    * @param output param0, param1, param2
    *
    * @pre pass in your args in the same order as the RotationOrder you specify
    * @post this function only reads 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    *       NOTE: Angles are returned in radians (this is always true in GMTL).
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, typename ROT_ORDER >
   inline EulerAngle<DATA_TYPE, ROT_ORDER>& set( EulerAngle<DATA_TYPE, ROT_ORDER>& euler,
                    const Matrix<DATA_TYPE, ROWS, COLS>& mat )
   {
      // @todo set this a compile time assert...
      gmtlASSERT( ROWS >= 3 && COLS >= 3 && ROWS <= 4 && COLS <= 4 &&
            "this is undefined for Matrix smaller than 3x3 or bigger than 4x4" );

      // @todo metaprogram this!
      const int& order = ROT_ORDER::ID;
      switch (order)
      {
      case XYZ::ID:
         {
#if 0
            euler[2] = Math::aTan2( -mat(0,1), mat(0,0) );       // -(-cy*sz)/(cy*cz) - cy falls out
            euler[0] = Math::aTan2( -mat(1,2), mat(2,2) );       // -(sx*cy)/(cx*cy) - cy falls out
            DATA_TYPE cz = Math::cos( euler[2] );
            euler[1] = Math::aTan2( mat(0,2), mat(0,0) / cz );   // (sy)/((cy*cz)/cz)
#else
            DATA_TYPE x(0), y(0), z(0);
            y = Math::aSin( mat(0,2));
            if (y < gmtl::Math::PI_OVER_2)
            {
               if(y > -gmtl::Math::PI_OVER_2)
               {
                  x = Math::aTan2(-mat(1,2),mat(2,2));
                  z = Math::aTan2(-mat(0,1),mat(0,0));
               }
               else  // Non-unique (x - z constant)
               {
                  x = -Math::aTan2(mat(1,0), mat(1,1));
                  z = 0;
               }
            }
            else
            {
               // not unique (x + z constant)
               x = Math::aTan2(mat(1,0), mat(1,1));
               z = 0;
            }
            euler[0] = x;
            euler[1] = y;
            euler[2] = z;

#endif
         }
         break;
      case ZYX::ID:
         {
#if 0
            euler[0] = Math::aTan2( mat(1,0), mat(0,0) );        // (cy*sz)/(cy*cz) - cy falls out
            euler[2] = Math::aTan2( mat(2,1), mat(2,2) );        // (sx*cy)/(cx*cy) - cy falls out
            DATA_TYPE sx = Math::sin( euler[2] );
            euler[1] = Math::aTan2( -mat(2,0), mat(2,1) / sx );  // -(-sy)/((sx*cy)/sx)
#else
            DATA_TYPE x(0), y(0), z(0);

            y = Math::aSin(-mat(2,0));
            if(y < Math::PI_OVER_2)
            {
               if(y>-Math::PI_OVER_2)
               {
                  z = Math::aTan2(mat(1,0), mat(0,0));
                  x = Math::aTan2(mat(2,1), mat(2,2));
               }
               else  // not unique (x + z constant)
               {
                  z = Math::aTan2(-mat(0,1),-mat(0,2));
                  x = 0;
               }
            }
            else  // not unique (x - z constant)
            {
               z = -Math::aTan2(mat(0,1), mat(0,2));
               x = 0;
            }
            euler[0] = z;
            euler[1] = y;
            euler[2] = x;
#endif
         }
         break;
      case ZXY::ID:
         {
#if 0
         // Extract the rotation directly from the matrix
            DATA_TYPE x_angle;
            DATA_TYPE y_angle;
            DATA_TYPE z_angle;
            DATA_TYPE cos_y, sin_y;
            DATA_TYPE cos_x, sin_x;
            DATA_TYPE cos_z, sin_z;

            sin_x = mat(2,1);
            x_angle = Math::aSin( sin_x );     // Get x angle
            cos_x = Math::cos( x_angle );

            // Check if cos_x = Zero
            if (cos_x != 0.0f)     // ASSERT: cos_x != 0
            {
                  // Get y Angle
               cos_y = mat(2,2) / cos_x;
               sin_y = -mat(2,0) / cos_x;
               y_angle = Math::aTan2( cos_y, sin_y );

                  // Get z Angle
               cos_z = mat(1,1) / cos_x;
               sin_z = -mat(0,1) / cos_x;
               z_angle = Math::aTan2( cos_z, sin_z );
            }
            else
            {
               // Arbitrarily set z_angle = 0
               z_angle = 0;

                  // Get y Angle
               cos_y = mat(0,0);
               sin_y = mat(1,0);
               y_angle = Math::aTan2( cos_y, sin_y );
            }

            euler[1] = x_angle;
            euler[2] = y_angle;
            euler[0] = z_angle;
#else
            DATA_TYPE x,y,z;

            x = Math::aSin(mat(2,1));
            if(x < Math::PI_OVER_2)
            {
               if(x > -Math::PI_OVER_2)
               {
                  z = Math::aTan2(-mat(0,1), mat(1,1));
                  y = Math::aTan2(-mat(2,0), mat(2,2));
               }
               else  // not unique (y - z constant)
               {
                  z = -Math::aTan2(mat(0,2), mat(0,0));
                  y = 0;
               }
            }
            else  // not unique (y + z constant)
            {
               z = Math::aTan2(mat(0,2), mat(0,0));
               y = 0;
            }
            euler[0] = z;
            euler[1] = x;
            euler[2] = y;
#endif
         }
         break;
      default:
         gmtlASSERT( false && "unknown rotation order passed to setRot" );
         break;
      }
      return euler;
   }

   /** Redundant duplication of the set(eulerangle,quat) function, this is provided only for template compatibility.
    *  unless you're writing template functions, you should use set(eulerangle,quat) for clarity.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, typename ROT_ORDER >
   inline EulerAngle<DATA_TYPE, ROT_ORDER>& setRot( EulerAngle<DATA_TYPE, ROT_ORDER>& result, const Matrix<DATA_TYPE, ROWS, COLS>& mat )
   {
      return gmtl::set( result, mat );
   }
   /** @} */

   /** @ingroup Generate
    *  @name Matrix Generators
    *  @{
    */

   /** Set an arbitrary projection matrix
    *  @result: set a projection matrix (similar to glFrustum).
    */
   template <typename T >
   inline Matrix<T, 4,4>& setFrustum( Matrix<T, 4, 4>& result,
                                                   T left, T top, T right,
                                                   T bottom, T nr, T fr )
   {
      result.mData[0] = ( T( 2.0 ) * nr ) / ( right - left );
      result.mData[1] = T( 0.0 );
      result.mData[2] = T( 0.0 );
      result.mData[3] = T( 0.0 );

      result.mData[4] = T( 0.0 );
      result.mData[5] = ( T( 2.0 ) * nr ) / ( top - bottom );
      result.mData[6] = T( 0.0 );
      result.mData[7] = T( 0.0 );

      result.mData[8] = ( right + left ) / ( right - left );
      result.mData[9] = ( top + bottom ) / ( top - bottom );
      result.mData[10] = -( fr + nr ) / ( fr - nr );
      result.mData[11] = T( -1.0 );

      result.mData[12] = T( 0.0 );
      result.mData[13] = T( 0.0 );
      result.mData[14] = -( T( 2.0 ) * fr * nr ) / ( fr - nr );
      result.mData[15] = T( 0.0 );

      result.mState = Matrix<T, 4, 4>::FULL; // track state

      return result;
   }

   /** Set an orthographic projection matrix
    *  creates a transformation that produces a parallel projection matrix.
    *  @nr = -nr is the value of the near clipping plane (positive value for near is negative in the z direction)
    *  @fr = -fr is the value of the far clipping plane (positive value for far is negative in the z direction)
    *  @result: set a orthographic matrix (similar to glOrtho).
    */
   template <typename T >
   inline Matrix<T, 4,4>& setOrtho( Matrix<T,4,4>& result,
                                          T left, T top, T right,
                                          T bottom, T nr, T fr )
   {
      result.mData[1] = result.mData[2] = result.mData[3] =
      result.mData[4] = result.mData[6] = result.mData[7] =
      result.mData[8] = result.mData[9] = result.mData[11] = T(0);

      T rl_inv = T(1) / (right - left);
      T tb_inv = T(1) / (top - bottom);
      T nf_inv = T(1) / (fr - nr);

      result.mData[0] =  T(2) * rl_inv;
      result.mData[5] =  T(2) * tb_inv;
      result.mData[10] = -T(2) * nf_inv;

      result.mData[12] = -(right + left) * rl_inv;
      result.mData[13] = -(top + bottom) * tb_inv;
      result.mData[14] = -(fr + nr) * nf_inv;
      result.mData[15] = T(1);

      return result;
   }

   /** Set a symmetric perspective projection matrix
    * @param fovy
    *   The field of view angle, in degrees, about the y-axis.
    * @param aspect
    *   The aspect ratio that determines the field of view about the x-axis.
    *   The aspect ratio is the ratio of x (width) to y (height).
    * @param zNear
    *   The distance from the viewer to the near clipping plane (always positive).
    * @param zFar
    *   The distance from the viewer to the far clipping plane (always positive).
    * @result Set matrix to perspective transform
    */
   template <typename T>
   inline Matrix<T, 4,4>& setPerspective( Matrix<T, 4, 4>& result,
                                                 T fovy, T aspect, T nr, T fr )
   {
      gmtlASSERT( nr > 0 && fr > nr && "invalid near and far values" );
      T theta = Math::deg2Rad( fovy * T( 0.5 ) );
      T tangentTheta = Math::tan( theta );

      // tan(theta) = right / nr
      // top = tan(theta) * nr
      // right = tan(theta) * nr * aspect

      T top = tangentTheta * nr;
      T right = top * aspect; // aspect determines the fieald of view in the x-axis

      // TODO: args need to match...
      return setFrustum( result, -right, top, right, -top, nr, fr );
   }


   /*
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned SIZE, typename REP >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setTrans( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                   const VecBase<DATA_TYPE, SIZE, REP>& trans )
   {
      const Vec<DATA_TYPE,SIZE,meta::DefaultVecTag> temp_vec(trans);
      return setTrans(result,trans);
   }
   */

   /** Set matrix translation from vec.
    * @pre if making an n x n matrix, then for
    *    - <b>vector is homogeneous:</b> SIZE of vector needs to equal number of Matrix ROWS - 1
    *    - <b>vector has scale component:</b> SIZE of vector needs to equal number of Matrix ROWS
    * if making an n x n+1 matrix, then for
    *    - <b>vector is homogeneous:</b> SIZE of vector needs to equal number of Matrix ROWS
    *    - <b>vector has scale component:</b> SIZE of vector needs to equal number of Matrix ROWS + 1
    * @post if preconditions are not met, then function is undefined (will not compile)
    */
#ifdef GMTL_NO_METAPROG
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned SIZE >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setTrans( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                   const VecBase<DATA_TYPE, SIZE>& trans )
#else
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned SIZE, typename REP >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setTrans( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                   const VecBase<DATA_TYPE, SIZE, REP>& trans )
#endif
   {
      /* @todo make this a compile time assert... */
      // if n x n   then (homogeneous case) vecsize == rows-1 or (scale component case) vecsize == rows
      // if n x n+1 then (homogeneous case) vecsize == rows   or (scale component case) vecsize == rows+1
      gmtlASSERT( ((ROWS == COLS && (SIZE == (ROWS-1) || SIZE == ROWS)) ||
               (COLS == (ROWS+1) && (SIZE == ROWS || SIZE == (ROWS+1)))) &&
              "preconditions not met for vector size in call to makeTrans.  Read your documentation." );

      // homogeneous case...
      if ((ROWS == COLS && SIZE == ROWS) /* Square matrix and vec so assume homogeneous vector. ex. 4x4 with vec 4 */
          || (COLS == (ROWS+1) && SIZE == (ROWS+1)))  /* ex: 3x4 with vec4 */
      {
         DATA_TYPE homog_val = trans[SIZE-1];
         for (unsigned x = 0; x < COLS - 1; ++x)
            result( x, COLS - 1 ) = trans[x] / homog_val;
      }

      // non-homogeneous case...
      else
      {
         for (unsigned x = 0; x < COLS - 1; ++x)
            result( x, COLS - 1 ) = trans[x];
      }
      // track state, only override identity
      switch (result.mState)
      {
      case Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL: result.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE; break;
      case Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY:   result.mState = Matrix<DATA_TYPE, ROWS, COLS>::TRANS; break;
      }
      return result;
   }

   /** Set the scale part of a matrix.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned SIZE >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setScale( Matrix<DATA_TYPE, ROWS, COLS>& result, const Vec<DATA_TYPE, SIZE>& scale )
   {
      gmtlASSERT( ((SIZE == (ROWS-1) && SIZE == (COLS-1)) || (SIZE == (ROWS-1) && SIZE == COLS) || (SIZE == (COLS-1) && SIZE == ROWS)) && "the scale params must fit within the matrix, check your sizes." );
      for (unsigned x = 0; x < SIZE; ++x)
      {
         result( x, x ) = scale[x];
      }
      // track state: affine matrix with non-uniform scale now.
      result.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE;
      result.mState |= Matrix<DATA_TYPE, ROWS, COLS>::NON_UNISCALE;
      return result;
   }

   /** Sets the scale part of a matrix.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setScale( Matrix<DATA_TYPE, ROWS, COLS>& result, const DATA_TYPE scale )
   {
      for (unsigned x = 0; x < Math::Min( ROWS, COLS, Math::Max( ROWS, COLS ) - 1 ); ++x) // account for 2x4 or other weird sizes...
      {
         result( x, x ) = scale;
      }
      // track state: affine matrix with non-uniform scale now.
      result.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE;
      result.mState |= Matrix<DATA_TYPE, ROWS, COLS>::NON_UNISCALE;
      return result;
   }

   /** Create a scale matrix.
    *  @param scale  You'll typically pass in a Vec or a float here.
    *  @seealso      setScale() for all possible argument types for this function.
    */
   template <typename MATRIX_TYPE, typename INPUT_TYPE>
   inline MATRIX_TYPE makeScale( const INPUT_TYPE& scale,
                               Type2Type< MATRIX_TYPE > t = Type2Type< MATRIX_TYPE >() )
   {
      gmtl::ignore_unused_variable_warning(t);
      MATRIX_TYPE temporary;
      return setScale( temporary, scale );
   }

   /** Set the rotation portion of a rotation matrix using an axis and an angle (in radians).
    *  Only writes to the rotation matrix (3x3) defined by the rotation part of M
    * @post this function only writes to 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    * @pre you must pass a normalized vector in for the axis, results are undefined if not.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setRot( Matrix<DATA_TYPE, ROWS, COLS>& result, const AxisAngle<DATA_TYPE>& axisAngle )
   {
      /* @todo set this a compile time assert... */
      gmtlASSERT( ROWS >= 3 && COLS >= 3 && ROWS <= 4 && COLS <= 4 &&
                     "this func is undefined for Matrix smaller than 3x3 or bigger than 4x4" );
      gmtlASSERT( Math::isEqual( lengthSquared( axisAngle.getAxis() ), (DATA_TYPE)1.0, (DATA_TYPE)0.001 ) /* &&
                     "you must pass in a normalized vector to setRot( mat, rad, vec )" */ );

      // GGI: pg 466
      DATA_TYPE s = Math::sin( axisAngle.getAngle() );
      DATA_TYPE c = Math::cos( axisAngle.getAngle() );
      DATA_TYPE t = DATA_TYPE( 1.0 ) - c;
      DATA_TYPE x = axisAngle.getAxis()[0];
      DATA_TYPE y = axisAngle.getAxis()[1];
      DATA_TYPE z = axisAngle.getAxis()[2];

      /* From: Introduction to robotic.  Craig.  Pg. 52 */
      result( 0, 0 ) = (t*x*x)+c;     result( 0, 1 ) = (t*x*y)-(s*z); result( 0, 2 ) = (t*x*z)+(s*y);
      result( 1, 0 ) = (t*x*y)+(s*z); result( 1, 1 ) = (t*y*y)+c;     result( 1, 2 ) = (t*y*z)-(s*x);
      result( 2, 0 ) = (t*x*z)-(s*y); result( 2, 1 ) = (t*y*z)+(s*x); result( 2, 2 ) = (t*z*z)+c;

      // track state
      switch (result.mState)
      {
      case Matrix<DATA_TYPE, ROWS, COLS>::TRANS:    result.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE; break;
      case Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY: result.mState = Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL; break;
      }
      return result;
   }

   /** Set (only) the rotation part of a matrix using an EulerAngle (angles are in radians).
    * @post this function only produces 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    * @see EulerAngle for angle ordering (usually ordered based on RotationOrder)
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, typename ROT_ORDER >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setRot( Matrix<DATA_TYPE, ROWS, COLS>& result, const EulerAngle<DATA_TYPE, ROT_ORDER>& euler )
   {
      // @todo set this a compile time assert...
      gmtlASSERT( ROWS >= 3 && COLS >= 3 && ROWS <= 4 && COLS <= 4 && "this is undefined for Matrix smaller than 3x3 or bigger than 4x4" );

      // this might be faster if put into the switch statement... (testme)
      const int& order = ROT_ORDER::ID;
      const DATA_TYPE xRot = (order == XYZ::ID) ? euler[0] : ((order == ZXY::ID) ? euler[1] : euler[2]);
      const DATA_TYPE yRot = (order == XYZ::ID) ? euler[1] : ((order == ZXY::ID) ? euler[2] : euler[1]);
      const DATA_TYPE zRot = (order == XYZ::ID) ? euler[2] : ((order == ZXY::ID) ? euler[0] : euler[0]);

      DATA_TYPE sx = Math::sin( xRot );  DATA_TYPE cx = Math::cos( xRot );
      DATA_TYPE sy = Math::sin( yRot );  DATA_TYPE cy = Math::cos( yRot );
      DATA_TYPE sz = Math::sin( zRot );  DATA_TYPE cz = Math::cos( zRot );

      // @todo metaprogram this!
      switch (order)
      {
      case XYZ::ID:
         // Derived by simply multiplying out the matrices by hand X * Y * Z
         result( 0, 0 ) = cy*cz;             result( 0, 1 ) = -cy*sz;            result( 0, 2 ) = sy;
         result( 1, 0 ) = sx*sy*cz + cx*sz;  result( 1, 1 ) = -sx*sy*sz + cx*cz; result( 1, 2 ) = -sx*cy;
         result( 2, 0 ) = -cx*sy*cz + sx*sz; result( 2, 1 ) = cx*sy*sz + sx*cz;  result( 2, 2 ) = cx*cy;
         break;
      case ZYX::ID:
         // Derived by simply multiplying out the matrices by hand Z * Y * Z
         result( 0, 0 ) = cy*cz; result( 0, 1 ) = -cx*sz + sx*sy*cz; result( 0, 2 ) = sx*sz + cx*sy*cz;
         result( 1, 0 ) = cy*sz; result( 1, 1 ) = cx*cz + sx*sy*sz;  result( 1, 2 ) = -sx*cz + cx*sy*sz;
         result( 2, 0 ) = -sy;   result( 2, 1 ) = sx*cy;             result( 2, 2 ) = cx*cy;
         break;
      case ZXY::ID:
         // Derived by simply multiplying out the matrices by hand Z * X * Y
         result( 0, 0 ) = cy*cz - sx*sy*sz; result( 0, 1 ) = -cx*sz; result( 0, 2 ) = sy*cz + sx*cy*sz;
         result( 1, 0 ) = cy*sz + sx*sy*cz; result( 1, 1 ) = cx*cz;  result( 1, 2 ) = sy*sz - sx*cy*cz;
         result( 2, 0 ) = -cx*sy;           result( 2, 1 ) = sx;     result( 2, 2 ) = cx*cy;
         break;
      default:
         gmtlASSERT( false && "unknown rotation order passed to setRot" );
         break;
      }

      // track state
      switch (result.mState)
      {
      case Matrix<DATA_TYPE, ROWS, COLS>::TRANS:    result.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE; break;
      case Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY: result.mState = Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL; break;
      }
      return result;
   }

   /** Convert an AxisAngle to a rotation matrix.
    * @post this function only writes to 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    * @pre AxisAngle must be normalized (the axis part), results are undefined if not.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline Matrix<DATA_TYPE, ROWS, COLS>& set( Matrix<DATA_TYPE, ROWS, COLS>& result, const AxisAngle<DATA_TYPE>& axisAngle )
   {
      gmtl::identity( result );
      return setRot( result, axisAngle );
   }

   /** Convert an EulerAngle to a rotation matrix.
    * @post this function only writes to 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS, typename ROT_ORDER >
   inline Matrix<DATA_TYPE, ROWS, COLS>& set( Matrix<DATA_TYPE, ROWS, COLS>& result, const EulerAngle<DATA_TYPE, ROT_ORDER>& euler )
   {
      gmtl::identity( result );
      return setRot( result, euler );
   }

   /**
    * Extracts the Y axis rotation information from the matrix.
    * @post Returned value is from -PI to PI, where 0 is none.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline DATA_TYPE makeYRot( const Matrix<DATA_TYPE, ROWS, COLS>& mat )
   {
      const gmtl::Vec<DATA_TYPE, 3> forward_point(0.0f, 0.0f, -1.0f);   // -Z
      const gmtl::Vec<DATA_TYPE, 3> origin_point(0.0f, 0.0f, 0.0f);
      gmtl::Vec<DATA_TYPE, 3> end_point, start_point;

      gmtl::xform(end_point, mat, forward_point);
      gmtl::xform(start_point, mat, origin_point);
      gmtl::Vec<DATA_TYPE, 3> direction_vector = end_point - start_point;

      // Constrain the direction to XZ-plane only.
      direction_vector[1] = 0.0f;                  // Eliminate Y value
      gmtl::normalize(direction_vector);
      DATA_TYPE y_rot = gmtl::Math::aCos(gmtl::dot(direction_vector,
                                                   forward_point));

      gmtl::Vec<DATA_TYPE, 3> which_side = gmtl::makeCross(forward_point,
                                                           direction_vector);

      // If direction vector to "right" (negative) side of forward
      if ( which_side[1] < 0.0f )
      {
         y_rot = -y_rot;
      }

      return y_rot;
   }

   /**
    * Extracts the X-axis rotation information from the matrix.
    * @post Returned value is from -PI to PI, where 0 is no rotation.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline DATA_TYPE makeXRot( const Matrix<DATA_TYPE, ROWS, COLS>& mat )
   {
      const gmtl::Vec<DATA_TYPE, 3> forward_point(0.0f, 0.0f, -1.0f);   // -Z
      const gmtl::Vec<DATA_TYPE, 3> origin_point(0.0f, 0.0f, 0.0f);
      gmtl::Vec<DATA_TYPE, 3> end_point, start_point;

      gmtl::xform(end_point, mat, forward_point);
      gmtl::xform(start_point, mat, origin_point);
      gmtl::Vec<DATA_TYPE, 3> direction_vector = end_point - start_point;

      // Constrain the direction to YZ-plane only.
      direction_vector[0] = 0.0f;                  // Eliminate X value
      gmtl::normalize(direction_vector);
      DATA_TYPE x_rot = gmtl::Math::aCos(gmtl::dot(direction_vector,
                                                   forward_point));

      gmtl::Vec<DATA_TYPE, 3> which_side = gmtl::makeCross(forward_point,
                                                           direction_vector);

      // If direction vector to "bottom" (negative) side of forward
      if ( which_side[0] < 0.0f )
      {
         x_rot = -x_rot;
      }

      return x_rot;
   }

   /**
    * Extracts the Z-axis rotation information from the matrix.
    * @post Returned value is from -PI to PI, where 0 is no rotation.
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline DATA_TYPE makeZRot( const Matrix<DATA_TYPE, ROWS, COLS>& mat )
   {
      const gmtl::Vec<DATA_TYPE, 3> forward_point(1.0f, 0.0f, 0.0f);   // +x
      const gmtl::Vec<DATA_TYPE, 3> origin_point(0.0f, 0.0f, 0.0f);
      gmtl::Vec<DATA_TYPE, 3> end_point, start_point;

      gmtl::xform(end_point, mat, forward_point);
      gmtl::xform(start_point, mat, origin_point);
      gmtl::Vec<DATA_TYPE, 3> direction_vector = end_point - start_point;

      // Constrain the direction to XY-plane only.
      direction_vector[2] = 0.0f;                  // Eliminate Z value
      gmtl::normalize(direction_vector);
      DATA_TYPE z_rot = gmtl::Math::aCos(gmtl::dot(direction_vector,
                                                   forward_point));

      gmtl::Vec<DATA_TYPE, 3> which_side = gmtl::makeCross(forward_point,
                                                           direction_vector);

      // If direction vector to "right" (negative) side of forward
      if ( which_side[2] < 0.0f )
      {
         z_rot = -z_rot;
      }

      return z_rot;
   }

   /** create a rotation matrix that will rotate from SrcAxis to DestAxis.
    *  xSrcAxis, ySrcAxis, zSrcAxis is the base rotation to go from and defaults to xSrcAxis(1,0,0), ySrcAxis(0,1,0), zSrcAxis(0,0,1) if you only pass in 3 axes.
    *
    *  If the two coordinate frames are labeled: SRC and TARGET, the matrix produced is:  src_M_target
    *  this means that it will transform a point in TARGET to SRC but moves the base frame from SRC to TARGET.
    *
    *  @pre pass in 3 axes, and setDirCos will give you the rotation from MATRIX_IDENTITY to DestAxis
    *  @pre pass in 6 axes, and setDirCos will give you the rotation from your 3-axis rotation to your second 3-axis rotation
    *  @post this function only produces 3x3, 3x4, 4x3, and 4x4 matrices
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setDirCos( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                     const Vec<DATA_TYPE, 3>& xDestAxis,
                                                     const Vec<DATA_TYPE, 3>& yDestAxis, const Vec<DATA_TYPE, 3>& zDestAxis,
                                                     const Vec<DATA_TYPE, 3>& xSrcAxis = Vec<DATA_TYPE, 3>(1,0,0),
                                                     const Vec<DATA_TYPE, 3>& ySrcAxis = Vec<DATA_TYPE, 3>(0,1,0),
                                                     const Vec<DATA_TYPE, 3>& zSrcAxis = Vec<DATA_TYPE, 3>(0,0,1) )
   {
      // @todo set this a compile time assert...
      gmtlASSERT( ROWS >= 3 && COLS >= 3 && ROWS <= 4 && COLS <= 4 && "this is undefined for Matrix smaller than 3x3 or bigger than 4x4" );

      DATA_TYPE Xa, Xb, Xc;    // Direction cosines of the secondary x-axis
      DATA_TYPE Ya, Yb, Yc;    // Direction cosines of the secondary y-axis
      DATA_TYPE Za, Zb, Zc;    // Direction cosines of the secondary z-axis

      Xa = dot(xDestAxis, xSrcAxis);  Xb = dot(xDestAxis, ySrcAxis);  Xc = dot(xDestAxis, zSrcAxis);
      Ya = dot(yDestAxis, xSrcAxis);  Yb = dot(yDestAxis, ySrcAxis);  Yc = dot(yDestAxis, zSrcAxis);
      Za = dot(zDestAxis, xSrcAxis);  Zb = dot(zDestAxis, ySrcAxis);  Zc = dot(zDestAxis, zSrcAxis);

      // Set the matrix correctly
      result( 0, 0 ) = Xa; result( 0, 1 ) = Ya; result( 0, 2 ) = Za;
      result( 1, 0 ) = Xb; result( 1, 1 ) = Yb; result( 1, 2 ) = Zb;
      result( 2, 0 ) = Xc; result( 2, 1 ) = Yc; result( 2, 2 ) = Zc;

      // track state
      switch (result.mState)
      {
      case Matrix<DATA_TYPE, ROWS, COLS>::TRANS:    result.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE; break;
      case Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY: result.mState = Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL; break;
      }

      return result;
   }

   /** set the matrix given the raw coordinate axes.
    * @post this function only produces 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    * @post these axes are copied direct to the 3x3 in the matrix
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline Matrix<DATA_TYPE, ROWS, COLS>& setAxes( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                  const Vec<DATA_TYPE, 3>& xAxis,
                                                  const Vec<DATA_TYPE, 3>& yAxis,
                                                  const Vec<DATA_TYPE, 3>& zAxis )
   {
      // @todo set this a compile time assert...
      gmtlASSERT( ROWS >= 3 && COLS >= 3 && ROWS <= 4 && COLS <= 4 && "this is undefined for Matrix smaller than 3x3 or bigger than 4x4" );

      result( 0, 0 ) = xAxis[0];
      result( 1, 0 ) = xAxis[1];
      result( 2, 0 ) = xAxis[2];

      result( 0, 1 ) = yAxis[0];
      result( 1, 1 ) = yAxis[1];
      result( 2, 1 ) = yAxis[2];

      result( 0, 2 ) = zAxis[0];
      result( 1, 2 ) = zAxis[1];
      result( 2, 2 ) = zAxis[2];

      // track state
      switch (result.mState)
      {
      case Matrix<DATA_TYPE, ROWS, COLS>::TRANS:    result.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE; break;
      case Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY: result.mState = Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL; break;
      }
      return result;
   }

   /** set the matrix given the raw coordinate axes.
    * @post this function only produces 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    * @post these axes are copied direct to the 3x3 in the matrix
    */
   template< typename ROTATION_TYPE >
   inline ROTATION_TYPE makeAxes( const Vec<typename ROTATION_TYPE::DataType, 3>& xAxis,
                                  const Vec<typename ROTATION_TYPE::DataType, 3>& yAxis,
                                  const Vec<typename ROTATION_TYPE::DataType, 3>& zAxis,
                                  Type2Type< ROTATION_TYPE > t = Type2Type< ROTATION_TYPE >() )
   {
      gmtl::ignore_unused_variable_warning(t);
      ROTATION_TYPE temporary;
      return setAxes( temporary, xAxis, yAxis, zAxis );
   }

   /** create a matrix transposed from the source.
    *  @post returns the transpose of m
    *  @see Quat
    */
   template < typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline Matrix<DATA_TYPE, ROWS, COLS> makeTranspose( const Matrix<DATA_TYPE, ROWS, COLS>& m )
   {
      Matrix<DATA_TYPE, ROWS, COLS> temporary( m );
      return transpose( temporary );
   }

   /**
    * Creates a matrix that is the inverse of the given source matrix.
    *
    * @param src     the matrix to compute the inverse of
    *
    * @return  the inverse of source
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   inline Matrix<DATA_TYPE, ROWS, COLS> makeInvert(const Matrix<DATA_TYPE, ROWS, COLS>& src)
   {
      Matrix<DATA_TYPE, ROWS, COLS> result;
      return invert( result, src );
   }

   /** Set the rotation portion of a matrix (3x3) from a rotation quaternion.
    * @pre only 3x3, 3x4, 4x3, or 4x4 matrices are allowed, function is undefined otherwise.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   Matrix<DATA_TYPE, ROWS, COLS>& setRot( Matrix<DATA_TYPE, ROWS, COLS>& mat, const Quat<DATA_TYPE>& q )
   {
      gmtlASSERT( ((ROWS == 3 && COLS == 3) ||
               (ROWS == 3 && COLS == 4) ||
               (ROWS == 4 && COLS == 3) ||
               (ROWS == 4 && COLS == 4)) &&
               "pre conditions not met on set( mat, quat ) which only sets a quaternion to the rotation part of a 3x3, 3x4, 4x3, or 4x4 matrix." );

      // From Watt & Watt
      DATA_TYPE wx, wy, wz, xx, yy, yz, xy, xz, zz, xs, ys, zs;

      xs = q[Xelt] + q[Xelt]; ys = q[Yelt] + q[Yelt]; zs = q[Zelt] + q[Zelt];
      xx = q[Xelt] * xs;      xy = q[Xelt] * ys;      xz = q[Xelt] * zs;
      yy = q[Yelt] * ys;      yz = q[Yelt] * zs;      zz = q[Zelt] * zs;
      wx = q[Welt] * xs;      wy = q[Welt] * ys;      wz = q[Welt] * zs;

      mat( 0, 0 ) = DATA_TYPE(1.0) - (yy + zz);
      mat( 1, 0 ) = xy + wz;
      mat( 2, 0 ) = xz - wy;

      mat( 0, 1 ) = xy - wz;
      mat( 1, 1 ) = DATA_TYPE(1.0) - (xx + zz);
      mat( 2, 1 ) = yz + wx;

      mat( 0, 2 ) = xz + wy;
      mat( 1, 2 ) = yz - wx;
      mat( 2, 2 ) = DATA_TYPE(1.0) - (xx + yy);

      // track state
      switch (mat.mState)
      {
      case Matrix<DATA_TYPE, ROWS, COLS>::TRANS:    mat.mState = Matrix<DATA_TYPE, ROWS, COLS>::AFFINE; break;
      case Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY: mat.mState = Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL; break;
      }
      return mat;
   }

   /** Convert a Coord to a Matrix
    * Note: It is set directly, but this is equivalent to T*R where T is the translation matrix and R is the rotation matrix.
    * @see Coord
    * @see Matrix
    */
   template <typename DATATYPE, typename POS_TYPE, typename ROT_TYPE, unsigned MATCOLS, unsigned MATROWS >
   inline Matrix<DATATYPE, MATROWS, MATCOLS>& set( Matrix<DATATYPE, MATROWS, MATCOLS>& mat,
                                                   const Coord<POS_TYPE, ROT_TYPE>& coord )
   {
      // set to ident first...
      gmtl::identity( mat );

      // set translation
      // @todo metaprogram this out for 3x3 and 2x2 matrices
      if (MATCOLS == 4)
      {
         setTrans( mat, coord.getPos() );// filtered (only sets the trans part).
      }
      setRot( mat, coord.getRot() ); // filtered (only sets the rot part).
      return mat;
   }

   /** Convert a Quat to a rotation Matrix.
    *  @pre only 3x3, 3x4, 4x3, or 4x4 matrices are allowed, function is undefined otherwise.
    *  @post Matrix is entirely overwritten.
    *  @todo Implement using setRot
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   Matrix<DATA_TYPE, ROWS, COLS>& set( Matrix<DATA_TYPE, ROWS, COLS>& mat, const Quat<DATA_TYPE>& q )
   {
      if (ROWS == 4)
      {
         mat( 3, 0 ) = DATA_TYPE(0.0);
         mat( 3, 1 ) = DATA_TYPE(0.0);
         mat( 3, 2 ) = DATA_TYPE(0.0);
      }

      if (COLS == 4)
      {
         mat( 0, 3 ) = DATA_TYPE(0.0);
         mat( 1, 3 ) = DATA_TYPE(0.0);
         mat( 2, 3 ) = DATA_TYPE(0.0);
      }

      if (ROWS == 4 && COLS == 4)
         mat( 3, 3 ) = DATA_TYPE(1.0);

      // track state
      mat.mState = Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY;

      return setRot( mat, q );
   }

   /** @} */

   /** @ingroup Generate
    *  @name Coord Generators
    *  @{
    */

   /** convert Matrix to Coord */
   template <typename DATATYPE, typename POS_TYPE, typename ROT_TYPE, unsigned MATCOLS, unsigned MATROWS >
   inline Coord<POS_TYPE, ROT_TYPE>& set( Coord<POS_TYPE, ROT_TYPE>& eulercoord, const Matrix<DATATYPE, MATROWS, MATCOLS>& mat )
   {
      gmtl::setTrans( eulercoord.pos(), mat );
      gmtl::set( eulercoord.rot(), mat );
      return eulercoord;
   }

   /** Redundant duplication of the set(coord,mat) function, this is provided only for template compatibility.
    *  unless you're writing template functions, you should use set(coord,mat) for clarity.
    */
   template <typename DATATYPE, typename POS_TYPE, typename ROT_TYPE, unsigned MATCOLS, unsigned MATROWS >
   inline Coord<POS_TYPE, ROT_TYPE>& setRot( Coord<POS_TYPE, ROT_TYPE>& result, const Matrix<DATATYPE, MATROWS, MATCOLS>& mat )
   {
      return gmtl::set( result, mat );
   }

   /** @} */

   /** @ingroup Generate
     *  @name Generic Generators (any type)
     *  @{
     */

   /** Construct an object from another object of a different type.
    *  This allows us to automatically convert from any type to any
    *  other type.
    *  @pre must have a set() function defined that converts between the
    *       two types.
    *  @param src    the object to use for creation
    *  @return a new object with values based on the src variable
    *  @see OpenSGGenerate.h for an example
    */
   template <typename TARGET_TYPE, typename SOURCE_TYPE>
   inline TARGET_TYPE make( const SOURCE_TYPE& src,
                           Type2Type< TARGET_TYPE > t = Type2Type< TARGET_TYPE >() )
   {
      gmtl::ignore_unused_variable_warning(t);
      TARGET_TYPE target;
      return gmtl::set( target, src );
   }

   /** Create a rotation datatype from another rotation datatype.
    * @post converts the source rotation to a to another type (usually Matrix, Quat, Euler, AxisAngle),
    * @post returns a temporary object.
    */
   template <typename ROTATION_TYPE, typename SOURCE_TYPE >
   inline ROTATION_TYPE makeRot( const SOURCE_TYPE& coord,
                                Type2Type< ROTATION_TYPE > t = Type2Type< ROTATION_TYPE >() )
   {
      gmtl::ignore_unused_variable_warning(t);
      ROTATION_TYPE temporary;
      return gmtl::set( temporary, coord );
   }

   /** Create a rotation matrix or quaternion (or any other rotation data type) using direction cosines.
    *
    *  If the two coordinate frames are labeled: SRC and TARGET, the matrix produced is:  src_M_target
    *  this means that it will transform a point in TARGET to SRC but moves the base frame from SRC to TARGET.
    *
    * @param DestAxis required to specify
    * @param SrcAxis optional to specify
    * @pre specify 1 axis (3 vectors), or 2 axes (6 vectors).
    * @post Creates a rotation from SrcAxis to DestAxis
    * @post this function only produces 3x3, 3x4, 4x3, and 4x4 matrices, and is undefined otherwise
    */
   template< typename ROTATION_TYPE >
   inline ROTATION_TYPE makeDirCos( const Vec<typename ROTATION_TYPE::DataType, 3>& xDestAxis,
                                  const Vec<typename ROTATION_TYPE::DataType, 3>& yDestAxis,
                                  const Vec<typename ROTATION_TYPE::DataType, 3>& zDestAxis,
                                  const Vec<typename ROTATION_TYPE::DataType, 3>& xSrcAxis = Vec<typename ROTATION_TYPE::DataType, 3>(1,0,0),
                                  const Vec<typename ROTATION_TYPE::DataType, 3>& ySrcAxis = Vec<typename ROTATION_TYPE::DataType, 3>(0,1,0),
                                  const Vec<typename ROTATION_TYPE::DataType, 3>& zSrcAxis = Vec<typename ROTATION_TYPE::DataType, 3>(0,0,1),
                               Type2Type< ROTATION_TYPE > t = Type2Type< ROTATION_TYPE >() )
   {
      gmtl::ignore_unused_variable_warning(t);
      ROTATION_TYPE temporary;
      return setDirCos( temporary, xDestAxis, yDestAxis, zDestAxis, xSrcAxis, ySrcAxis, zSrcAxis );
   }

   /**
    * Make a translation datatype from another translation datatype.
    * Typically this is from Matrix to Vec or Vec to Matrix.
    * This function reads only translation information from the src datatype.
    *
    * @param arg  the matrix to extract the translation from
    *
    * @pre if making an n x n matrix, then for
    *    - <b>vector is homogeneous:</b> SIZE of vector needs to equal number of Matrix ROWS - 1
    *    - <b>vector has scale component:</b> SIZE of vector needs to equal number of Matrix ROWS
    * <br>if making an n x n+1 matrix, then for
    *    - <b>vector is homogeneous:</b> SIZE of vector needs to equal number of Matrix ROWS
    *    - <b>vector has scale component:</b> SIZE of vector needs to equal number of Matrix ROWS + 1
    * @post if preconditions are not met, then function is undefined (will not compile)
    */
   template<typename TRANS_TYPE, typename SRC_TYPE >
   inline TRANS_TYPE makeTrans( const SRC_TYPE& arg,
                             Type2Type< TRANS_TYPE > t = Type2Type< TRANS_TYPE >())
   {
      gmtl::ignore_unused_variable_warning(t);
      TRANS_TYPE temporary;
      return setTrans( temporary, arg );
   }

   /** Create a rotation datatype that will xform first vector to the second.
    *  @pre  each vec needs to be normalized.
    *  @post This function returns a temporary object.
    */
   template< typename ROTATION_TYPE >
   inline ROTATION_TYPE makeRot( const Vec<typename ROTATION_TYPE::DataType, 3>& from,
                                 const Vec<typename ROTATION_TYPE::DataType, 3>& to )
   {
      ROTATION_TYPE temporary;
      return setRot( temporary, from, to );
   }

   /** set a rotation datatype that will xform first vector to the second.
    *  @pre  each vec needs to be normalized.
    *  @post generate rotation datatype that is the rotation between the vectors.
    *  @note: only sets the rotation component of result, if result is a matrix, only sets the 3x3.
    */
   template <typename DEST_TYPE, typename DATA_TYPE>
   inline DEST_TYPE& setRot( DEST_TYPE& result, const Vec<DATA_TYPE, 3>& from, const Vec<DATA_TYPE, 3>& to )
   {
      // @todo should assert that DEST_TYPE::DataType == DATA_TYPE
      const DATA_TYPE epsilon = (DATA_TYPE)0.00001;

      gmtlASSERT( gmtl::Math::isEqual( gmtl::length( from ), (DATA_TYPE)1.0, epsilon ) &&
                  gmtl::Math::isEqual( gmtl::length( to ), (DATA_TYPE)1.0, epsilon ) /* &&
                  "input params not normalized" */);

      DATA_TYPE cosangle = dot( from, to );

      // if cosangle is close to 1, so the vectors are close to being coincident
      // Need to generate an angle of zero with any vector we like
      // We'll choose identity (no rotation)
      if ( Math::isEqual( cosangle, (DATA_TYPE)1.0, epsilon ) )
      {
         return result = DEST_TYPE();
      }

      // vectors are close to being opposite, so rotate one a little...
      else if ( Math::isEqual( cosangle, (DATA_TYPE)-1.0, epsilon ) )
      {
         Vec<DATA_TYPE, 3> to_rot( to[0] + (DATA_TYPE)0.3, to[1] - (DATA_TYPE)0.15, to[2] - (DATA_TYPE)0.15 ), axis;
         normalize( cross( axis, from, to_rot ) ); // setRot requires normalized vec
         DATA_TYPE angle = Math::aCos( cosangle );
         return setRot( result, gmtl::AxisAngle<DATA_TYPE>( angle, axis ) );
      }

      // This is the usual situation - take a cross-product of vec1 and vec2
      // and that is the axis around which to rotate.
      else
      {
         Vec<DATA_TYPE, 3> axis;
         normalize( cross( axis, from, to ) ); // setRot requires normalized vec
         DATA_TYPE angle = Math::aCos( cosangle );
         return setRot( result, gmtl::AxisAngle<DATA_TYPE>( angle, axis ) );
      }
   }

   /** @} */

   /** Accesses a particular row in the matrix by copying the values in the row
    * into the given vector.
    *
    * @param dest       the vector in which the values of the row will be placed
    * @param src        the matrix being accessed
    * @param row        the row of the matrix to access
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   void setRow(Vec<DATA_TYPE, COLS>& dest, const Matrix<DATA_TYPE, ROWS, COLS>& src, unsigned row)
   {
      for (unsigned i=0; i<COLS; ++i)
      {
         dest[i] = src[row][i];
      }
   }

   /** Accesses a particular row in the matrix by creating a new vector
    * containing the values in the given matrix.
    *
    * @param src        the matrix being accessed
    * @param row        the row of the matrix to access
    *
    * @return  a vector containing the values in the requested row
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   Vec<DATA_TYPE, COLS> makeRow(const Matrix<DATA_TYPE, ROWS, COLS>& src, unsigned row)
   {
      Vec<DATA_TYPE, COLS> result;
      setRow(result, src, row);
      return result;
   }

   /** Accesses a particular column in the matrix by copying the values in the
    * column into the given vector.
    *
    * @param dest       the vector in which the values of the column will be placed
    * @param src        the matrix being accessed
    * @param col        the column of the matrix to access
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   void setColumn(Vec<DATA_TYPE, ROWS>& dest, const Matrix<DATA_TYPE, ROWS, COLS>& src, unsigned col)
   {
      for (unsigned i=0; i<ROWS; ++i)
      {
         dest[i] = src[i][col];
      }
   }

   /** Accesses a particular column in the matrix by creating a new vector
    * containing the values in the given matrix.
    *
    * @param src        the matrix being accessed
    * @param col        the column of the matrix to access
    *
    * @return  a vector containing the values in the requested column
    */
   template< typename DATA_TYPE, unsigned ROWS, unsigned COLS >
   Vec<DATA_TYPE, ROWS> makeColumn(const Matrix<DATA_TYPE, ROWS, COLS>& src, unsigned col)
   {
      Vec<DATA_TYPE, ROWS> result;
      setColumn(result, src, col);
      return result;
   }

} // end gmtl namespace.

#endif
