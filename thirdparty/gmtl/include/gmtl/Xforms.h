// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_XFORMS_H_
#define _GMTL_XFORMS_H_

#include <gmtl/Point.h>
#include <gmtl/Vec.h>
#include <gmtl/Matrix.h>
#include <gmtl/MatrixOps.h>
#include <gmtl/Quat.h>
#include <gmtl/QuatOps.h>
#include <gmtl/Ray.h>
#include <gmtl/LineSeg.h>
#include <gmtl/Util/StaticAssert.h>

namespace gmtl
{
   /** @ingroup Transforms
    *  @name Vector Transform (Quaternion)
    *  @{
    */
   
   /** transform a vector by a rotation quaternion.
    * @pre give a vector, and a rotation quaternion (by definition, a rotation quaternion is normalized).
    * @param result     The vector to write the result into
    * @param rot        The quaternion
    * @param vector     The original vector to transform
    * @post v' = q P(v) q*  (where result is v', rot is q, and vector is v.  q* is conj(q), and P(v) is pure quaternion made from v)
    * @see game programming gems #1 p199
    * @see shoemake siggraph notes
    * @notes for the implementation, inv and conj should both work for the "q*" in "Rv = q P(v) q*"
    *        but conj is actually faster so we usually choose that.
    * @notes also note, that if the input quat wasn't normalized (and thus isn't a rotation quat),
    *        then this might not give the correct result, since conj and invert is only equiv when normalized...
    */
   template <typename DATA_TYPE>
   inline VecBase<DATA_TYPE, 3>& xform( VecBase<DATA_TYPE, 3>& result, const Quat<DATA_TYPE>& rot, const VecBase<DATA_TYPE, 3>& vector )
   {
      // check preconditions...
      gmtlASSERT( Math::isEqual( length( rot ), (DATA_TYPE)1.0, (DATA_TYPE)0.0001 ) && "must pass a rotation quaternion to xform(result,quat,vec) - by definition, a rotation quaternion is normalized).  if you need non-rotation quaternion support, let us know." );

      // easiest to write and understand (slowest too)
      //return result_vec = makeVec( rot * makePure( vector ) * makeConj( rot ) );

      // completely hand expanded
      // (faster by 28% in gcc 2.96 debug mode.)
      // (faster by 35% in gcc 2.96 opt3 mode (78% for doubles))
      Quat<DATA_TYPE> rot_conj( -rot[Xelt], -rot[Yelt], -rot[Zelt], rot[Welt] );
      Quat<DATA_TYPE> pure( vector[0], vector[1], vector[2], (DATA_TYPE)0.0 );
      Quat<DATA_TYPE> temp(
         pure[Welt]*rot_conj[Xelt] + pure[Xelt]*rot_conj[Welt] + pure[Yelt]*rot_conj[Zelt] - pure[Zelt]*rot_conj[Yelt],
         pure[Welt]*rot_conj[Yelt] + pure[Yelt]*rot_conj[Welt] + pure[Zelt]*rot_conj[Xelt] - pure[Xelt]*rot_conj[Zelt],
         pure[Welt]*rot_conj[Zelt] + pure[Zelt]*rot_conj[Welt] + pure[Xelt]*rot_conj[Yelt] - pure[Yelt]*rot_conj[Xelt],
         pure[Welt]*rot_conj[Welt] - pure[Xelt]*rot_conj[Xelt] - pure[Yelt]*rot_conj[Yelt] - pure[Zelt]*rot_conj[Zelt] );

      result.set(
         rot[Welt]*temp[Xelt] + rot[Xelt]*temp[Welt] + rot[Yelt]*temp[Zelt] - rot[Zelt]*temp[Yelt],
         rot[Welt]*temp[Yelt] + rot[Yelt]*temp[Welt] + rot[Zelt]*temp[Xelt] - rot[Xelt]*temp[Zelt],
         rot[Welt]*temp[Zelt] + rot[Zelt]*temp[Welt] + rot[Xelt]*temp[Yelt] - rot[Yelt]*temp[Xelt] );
      return result;
   }

   /** transform a vector by a rotation quaternion.
    * @pre give a vector, and a rotation quaternion (by definition, a rotation quaternion is normalized).
    * @param rot        The quaternion
    * @param vector     The original vector to transform
    * @return  the resulting vector transformed by the quaternion
    * @post v' = q P(v) q*  (where result is v', rot is q, and vector is v.  q* is conj(q), and P(v) is pure quaternion made from v)
    */
   template <typename DATA_TYPE>
   inline VecBase<DATA_TYPE, 3> operator*( const Quat<DATA_TYPE>& rot, const VecBase<DATA_TYPE, 3>& vector )
   {
      VecBase<DATA_TYPE, 3> temporary;
      return xform( temporary, rot, vector );
   }


   /** transform a vector by a rotation quaternion.
    * @pre give a vector, and a rotation quaternion (by definition, a rotation quaternion is normalized).
    * @param rot        The quaternion
    * @param vector     The original vector to transform
    * @post v' = q P(v) q*  (where result is v', rot is q, and vector is v.  q* is conj(q), and P(v) is pure quaternion made from v)
    */   
   template <typename DATA_TYPE>
   inline VecBase<DATA_TYPE, 3> operator*=(VecBase<DATA_TYPE, 3>& vector, const Quat<DATA_TYPE>& rot)
   {
      VecBase<DATA_TYPE, 3> temporary = vector;
      return xform( vector, rot, temporary);
   }


   /** @} */

   /** @ingroup Transforms
    *  @name Vector Transform (Matrix)
    *  @{
    */

   /** xform a vector by a matrix.
    *  Transforms a vector with a matrix, uses multiplication of [m x k] matrix by a [k x 1] matrix (the later also known as a Vector...).
    *  @param result        the vector to write the result in
    *  @param matrix        the transform matrix
    *  @param vector        the original vector
    *  @post This results in a rotational xform of the vector (assumes you know what you are doing -
    *  i.e. that you know that the last component of a vector by definition is 0.0, and changing
    *  this might make the xform different than what you may expect).
    *  @post returns a point same size as the matrix rows...  (v[r][1] = m[r][k] * v[k][1])
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Vec<DATA_TYPE, COLS>& xform( Vec<DATA_TYPE, COLS>& result, const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Vec<DATA_TYPE, COLS>& vector )
   {
      // do a standard [m x k] by [k x n] matrix multiplication (where n == 0).

      // reset vec to zero...
      result = Vec<DATA_TYPE, COLS>();

      for (unsigned iRow = 0; iRow < ROWS; ++iRow)
      for (unsigned iCol = 0; iCol < COLS; ++iCol)
         result[iRow] += matrix( iRow, iCol ) * vector[iCol];

      return result;
   }


   /** matrix * vector xform.
    *  multiplication of [m x k] matrix by a [k x 1] matrix (also known as a Vector...).
    *  @param matrix    the transform matrix
    *  @param vector    the original vector
    *  @return  the vector transformed by the matrix
    *  @post This results in a full matrix xform of the vector (assumes you know what you are doing -
    *  i.e. that you know that the last component of a vector by definition is 0.0, and changing
    *  this might make the xform different that what you may expect).
    *  @post returns a vec same size as the matrix rows...  (v[r][1] = m[r][k] * v[k][1])
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Vec<DATA_TYPE, COLS> operator*( const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Vec<DATA_TYPE, COLS>& vector )
   {
      // do a standard [m x k] by [k x n] matrix multiplication (where n == 0).
      Vec<DATA_TYPE, COLS> temporary;
      return xform( temporary, matrix, vector );
   }




   /** partially transform a partially specified vector by a matrix, assumes last elt of vector is 0 (the 0 makes it only partially transformed).
    *  Transforms a vector with a matrix, uses multiplication of [m x k] matrix by a [k-1 x 1] matrix (also known as a Vector [with w == 0 for vectors by definition] ).
    *  @param result        the vector to write the result in
    *  @param matrix        the transform matrix
    *  @param vector        the original vector
    *  @post the [k-1 x 1] vector you pass in is treated as a [vector, 0.0]
    *  @post This ends up being a partial xform using only the rotation from the matrix (vector xformed result is untranslated).
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned VEC_SIZE>
   inline Vec<DATA_TYPE, VEC_SIZE>& xform( Vec<DATA_TYPE, VEC_SIZE >& result, const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Vec<DATA_TYPE, VEC_SIZE >& vector )
   {
      GMTL_STATIC_ASSERT( VEC_SIZE == COLS - 1, Vec_of_wrong_size_for_xform );
      // do a standard [m x k] by [k x n] matrix multiplication (where n == 0).

      // copy the point to the correct size.
      Vec<DATA_TYPE, COLS> temp_vector, temp_result;
      for (unsigned x = 0; x < VEC_SIZE; ++x)
         temp_vector[x] = vector[x];
      temp_vector[COLS-1] = (DATA_TYPE)0.0; // by definition of a vector, set the last unspecified elt to 0.0

      // transform it.
      xform<DATA_TYPE, ROWS, COLS>( temp_result, matrix, temp_vector );

      // convert result back to vec<DATA_TYPE, VEC_SIZE>
      // some matrices will make the W param large even if this is a true vector,
      // we'll need to redistribute it to the other elts if W param is non-zero
      if (Math::isEqual( temp_result[VEC_SIZE], (DATA_TYPE)0, (DATA_TYPE)0.0001 ) == false)
      {
         DATA_TYPE w_coord_div = DATA_TYPE( 1.0 ) / temp_result[VEC_SIZE];
         for (unsigned x = 0; x < VEC_SIZE; ++x)
            result[x] = temp_result[x] * w_coord_div;
      }
      else
      {
         for (unsigned x = 0; x < VEC_SIZE; ++x)
            result[x] = temp_result[x];
      }

      return result;
   }

   /** matrix * partial vector, assumes last elt of vector is 0 (partial transform).
    *  @param matrix        the transform matrix
    *  @param vector        the original vector
    *  @return  the vector transformed by the matrix
    *  multiplication of [m x k] matrix by a [k-1 x 1] matrix (also known as a Vector [with w == 0 for vectors by definition] ).
    *  @post the [k-1 x 1] vector you pass in is treated as a [vector, 0.0]
    *  @post This ends up being a partial xform using only the rotation from the matrix (vector xformed result is untranslated).
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned COLS_MINUS_ONE>
   inline Vec<DATA_TYPE, COLS_MINUS_ONE> operator*( const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Vec<DATA_TYPE, COLS_MINUS_ONE>& vector )
   {
      Vec<DATA_TYPE, COLS_MINUS_ONE> temporary;
      return xform( temporary, matrix, vector );
   }

   /** @} */

   /** @ingroup Transforms
    *  @name Point Transform (Matrix)
    *  @{
    */


   /** transform point by a matrix.
    *  multiplication of [m x k] matrix by a [k x 1] matrix (also known as a Point...).
    *  @param result        the point to write the result in
    *  @param matrix        the transform matrix
    *  @param point         the original point
    *  @post This results in a full matrix xform of the point.
    *  @post returns a point same size as the matrix rows...  (p[r][1] = m[r][k] * p[k][1])
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Point<DATA_TYPE, COLS>& xform( Point<DATA_TYPE, COLS>& result, const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Point<DATA_TYPE, COLS>& point )
   {
      // do a standard [m x k] by [k x n] matrix multiplication (n == 1).

      // reset point to zero...
      result = Point<DATA_TYPE, COLS>();

      for (unsigned iRow = 0; iRow < ROWS; ++iRow)
      for (unsigned iCol = 0; iCol < COLS; ++iCol)
         result[iRow] += matrix( iRow, iCol ) * point[iCol];

      return result;
   }

   /** matrix * point.
    *  multiplication of [m x k] matrix by a [k x 1] matrix (also known as a Point...).
    *  @param matrix        the transform matrix
    *  @param point         the original point
    *  @return  the point transformed by the matrix
    *  @post This results in a full matrix xform of the point.
    *  @post returns a point same size as the matrix rows...  (p[r][1] = m[r][k] * p[k][1])
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Point<DATA_TYPE, COLS> operator*( const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Point<DATA_TYPE, COLS>& point )
   {
      Point<DATA_TYPE, COLS> temporary;
      return xform( temporary, matrix, point );
   }




   /** transform a partially specified point by a matrix, assumes last elt of point is 1.
    *  Transforms a point with a matrix, uses multiplication of [m x k] matrix by a [k-1 x 1] matrix (also known as a Point [with w == 1 for points by definition] ).
    *  @param result        the point to write the result in
    *  @param matrix        the transform matrix
    *  @param point         the original point
    *  @post the [k-1 x 1] point you pass in is treated as [point, 1.0]
    *  @post This results in a full matrix xform of the point.
    * @todo we need a PointOps.h operator*=(scalar) function
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned PNT_SIZE>
   inline Point<DATA_TYPE, PNT_SIZE>& xform( Point<DATA_TYPE, PNT_SIZE>& result, const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Point<DATA_TYPE, PNT_SIZE>& point )
   {
      //gmtlSERT( PNT_SIZE == COLS - 1 && "The precondition of this method is that the vector size must be one less than the number of columns in the matrix.  eg. if Mat<n,k>, then Vec<k-1>." );
      GMTL_STATIC_ASSERT( PNT_SIZE == COLS-1, Point_not_of_size_mat_col_minus_1_as_required_for_xform);

      // copy the point to the correct size.
      Point<DATA_TYPE, PNT_SIZE+1> temp_point, temp_result;
      for (unsigned x = 0; x < PNT_SIZE; ++x)
         temp_point[x] = point[x];
      temp_point[PNT_SIZE] = (DATA_TYPE)1.0; // by definition of a point, set the last unspecified elt to 1.0

      // transform it.
      xform<DATA_TYPE, ROWS, COLS>( temp_result, matrix, temp_point );

      // convert result back to pnt<DATA_TYPE, PNT_SIZE>
      // some matrices will make the W param large even if this is a true vector,
      // we'll need to redistribute it to the other elts if W param is non-zero
      if (Math::isEqual( temp_result[PNT_SIZE], (DATA_TYPE)0, (DATA_TYPE)0.0001 ) == false)
      {
         DATA_TYPE w_coord_div = DATA_TYPE( 1.0 ) / temp_result[PNT_SIZE];
         for (unsigned x = 0; x < PNT_SIZE; ++x)
            result[x] = temp_result[x] * w_coord_div;
      }
      else
      {
         for (unsigned x = 0; x < PNT_SIZE; ++x)
            result[x] = temp_result[x];
      }

      return result;
   }

   /** matrix * partially specified point.
    *  multiplication of [m x k] matrix by a [k-1 x 1] matrix (also known as a Point [with w == 1 for points by definition] ).
    *  @param matrix        the transform matrix
    *  @param point         the original point
    *  @return  the point transformed by the matrix
    *  @post the [k-1 x 1] vector you pass in is treated as a [point, 1.0]
    *  @post This results in a full matrix xform of the point.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned COLS_MINUS_ONE>
   inline Point<DATA_TYPE, COLS_MINUS_ONE> operator*( const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Point<DATA_TYPE, COLS_MINUS_ONE>& point )
   {
      Point<DATA_TYPE, COLS_MINUS_ONE> temporary;
      return xform( temporary, matrix, point );
   }

   /** point * a matrix
    *  multiplication of [m x k] matrix by a [k x 1] matrix (also known as a Point [with w == 1 for points by definition] ).
    *  @param matrix        the transform matrix
    *  @param point         the original point
    *  @return  the point transformed by the matrix
    *  @post This results in a full matrix xform of the point.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Point<DATA_TYPE, COLS> operator*( const Point<DATA_TYPE, COLS>& point, const Matrix<DATA_TYPE, ROWS, COLS>& matrix )
   {
      Point<DATA_TYPE, COLS> temporary;
      return xform( temporary, matrix, point );
   }


   /** point *= a matrix
    *  multiplication of [m x k] matrix by a [k x 1] matrix (also known as a Point [with w == 1 for points by definition] ).
    *  @param matrix        the transform matrix
    *  @param point         the original point
    *  @return  the point transformed by the matrix
    *  @post This results in a full matrix xform of the point.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Point<DATA_TYPE, COLS> operator*=(Point<DATA_TYPE, COLS>& point, const Matrix<DATA_TYPE, ROWS, COLS>& matrix)
   {
      Point<DATA_TYPE, COLS> temporary = point;
      return xform( point, matrix, temporary);
   }

   /** partial point *= a matrix
    *  multiplication of [m x k] matrix by a [k-1 x 1] matrix (also known as a Point [with w == 1 for points by definition] ).
    *  @param matrix        the transform matrix
    *  @param point         the original point
    *  @return  the point transformed by the matrix
    *  @post the [k-1 x 1] vector you pass in is treated as a [point, 1.0]
    *  @post This results in a full matrix xform of the point.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS, unsigned COLS_MINUS_ONE>
   inline Point<DATA_TYPE, COLS_MINUS_ONE>& operator*=( Point<DATA_TYPE, COLS_MINUS_ONE>& point, const Matrix<DATA_TYPE, ROWS, COLS>& matrix )
   {
      Point<DATA_TYPE, COLS_MINUS_ONE> temporary = point;
      return xform( point, matrix, temporary);
   }


   /** @} */

   /** transform ray by a matrix.
    *  multiplication of [m x k] matrix by two [k x 1] matrices (also known as a ray...).
    *  @param result        the ray to write the result in
    *  @param matrix        the transform matrix
    *  @param ray           the original ray
    *  @post This results in a full matrix xform of the ray.
    *  @post returns a ray same size as the matrix rows...  (p[r][1] = m[r][k] * p[k][1])
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Ray<DATA_TYPE>& xform( Ray<DATA_TYPE>& result, const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Ray<DATA_TYPE>& ray )
   {
      gmtl::Point<DATA_TYPE, 3> pos;
      gmtl::Vec<DATA_TYPE, 3> dir;
      result.setOrigin( xform( pos, matrix, ray.getOrigin() ) );
      result.setDir( xform( dir, matrix, ray.getDir() ) );
      return result;
   }

   /** ray * a matrix
    *  multiplication of [m x k] matrix by a ray.
    *  @param matrix        the transform matrix
    *  @param ray           the original ray
    *  @return  the ray transformed by the matrix
    *  @post This results in a full matrix xform of the ray.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Ray<DATA_TYPE> operator*( const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const Ray<DATA_TYPE>& ray )
   {
      Ray<DATA_TYPE> temporary;
      return xform( temporary, matrix, ray );
   }


   /** ray *= a matrix
    *  multiplication of [m x k] matrix by a ray.
    *  @param matrix        the transform matrix
    *  @param ray           the original ray
    *  @return  the ray transformed by the matrix
    *  @post This results in a full matrix xform of the ray.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Ray<DATA_TYPE>& operator*=( Ray<DATA_TYPE>& ray, const Matrix<DATA_TYPE, ROWS, COLS>& matrix )
   {
      Ray<DATA_TYPE> temporary = ray;
      return xform( ray, matrix, temporary);
   }

   /** transform seg by a matrix.
    *  multiplication of [m x k] matrix by two [k x 1] matrices (also known as a seg...).
    *  @param result        the seg to write the result in
    *  @param matrix        the transform matrix
    *  @param seg           the original seg
    *  @post This results in a full matrix xform of the seg.
    *  @post returns a seg same size as the matrix rows...  (p[r][1] = m[r][k] * p[k][1])
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline LineSeg<DATA_TYPE>& xform( LineSeg<DATA_TYPE>& result, const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const LineSeg<DATA_TYPE>& seg )
   {
      gmtl::Point<DATA_TYPE, 3> pos;
      gmtl::Vec<DATA_TYPE, 3> dir;
      result.setOrigin( xform( pos, matrix, seg.getOrigin() ) );
      result.setDir( xform( dir, matrix, seg.getDir() ) );
      return result;
   }

   /** seg * a matrix
    *  multiplication of [m x k] matrix by a seg.
    *  @param matrix        the transform matrix
    *  @param seg         the original ray
    *  @return  the seg transformed by the matrix
    *  @post This results in a full matrix xform of the seg.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline LineSeg<DATA_TYPE> operator*( const Matrix<DATA_TYPE, ROWS, COLS>& matrix, const LineSeg<DATA_TYPE>& seg )
   {
      LineSeg<DATA_TYPE> temporary;
      return xform( temporary, matrix, seg );
   }


   /** seg *= a matrix
    *  multiplication of [m x k] matrix by a seg.
    *  @param matrix        the transform matrix
    *  @param seg         the original point
    *  @return  the point transformed by the matrix
    *  @post This results in a full matrix xform of the point.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline LineSeg<DATA_TYPE>& operator*=( LineSeg<DATA_TYPE>& seg, const Matrix<DATA_TYPE, ROWS, COLS>& matrix )
   {
      LineSeg<DATA_TYPE> temporary = seg;
      return xform( seg, matrix, temporary);
   }




   // old xform stuff...
/*
// XXX: Assuming that there is no projective portion to the matrix or homogeneous coord
// NOTE: It is a vec, so we don't deal with the translation
Vec3 operator*(const Matrix& mat, const Vec3& vec)
{

   Vec3 ret_vec;
   for(int iRow=0;iRow<3;iRow++)
   {
      ret_vec[iRow] = (vec[0]* (mat[0][iRow]))
                    + (vec[1]* (mat[1][iRow]))
                    + (vec[2]* (mat[2][iRow]));
   }
   return ret_vec;
}

// XXX: Assuming no projective or homogeneous coord to the mat
Point3 operator*(const Matrix& mat, const Point3& point)
{
   Point3 ret_pt;
   for(int iRow=0;iRow<3;iRow++)
   {
      ret_pt[iRow] = (point[0]* (mat[0][iRow]))
                   + (point[1]* (mat[1][iRow]))
                   + (point[2]* (mat[2][iRow]))
                   + (mat[3][iRow]);
   }
   return ret_pt;
}

// Xform an OOB by a matrix
// NOTE: This will NOT work if the matrix has shear or scale
OOBox operator*(const Matrix& mat, const OOBox& box)
{
   OOBox ret_box;

   ret_box.center() = mat * box.center();

   ret_box.axis(0) = mat * ret_box.axis(0);
   ret_box.axis(1) = mat * ret_box.axis(1);
   ret_box.axis(2) = mat * ret_box.axis(2);

   ret_box.halfLen(0) = box.halfLen(0);
   ret_box.halfLen(1) = box.halfLen(1);
   ret_box.halfLen(2) = box.halfLen(2);

   return ret_box;
}
*/

}

#endif
