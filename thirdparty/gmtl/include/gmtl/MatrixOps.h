// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_MATRIXOPS_H_
#define _GMTL_MATRIXOPS_H_

#include <iostream>         // for std::cerr
#include <algorithm>        // needed for std::swap
#include <gmtl/Matrix.h>
#include <gmtl/Math.h>
#include <gmtl/Vec.h>
#include <gmtl/VecOps.h>
#include <gmtl/Util/Assert.h>

namespace gmtl
{
/** @ingroup Ops
 * @name Matrix Operations
 * @{
 */

   /** Make identity matrix out the matrix.
    * @post Every element is 0 except the matrix's diagonal, whose elements are 1.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& identity( Matrix<DATA_TYPE, ROWS, COLS>& result )
   {
      if(result.mState != Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY)   // if not already ident
      {
         // TODO: mp
         for (unsigned int r = 0; r < ROWS; ++r)
         for (unsigned int c = 0; c < COLS; ++c)
            result( r, c ) = (DATA_TYPE)0.0;

         // TODO: mp
         for (unsigned int x = 0; x < Math::Min( COLS, ROWS ); ++x)
            result( x, x ) = (DATA_TYPE)1.0;

         result.mState = Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY;
//         result.mState = Matrix<DATA_TYPE, ROWS, COLS>::FULL;
      }

      return result;
   }


   /** zero out the matrix.
    * @post every element is 0.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& zero( Matrix<DATA_TYPE, ROWS, COLS>& result )
   {
      if (result.mState == Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY)
      {
         for (unsigned int x = 0; x < Math::Min( ROWS, COLS ); ++x)
         {
            result( x, x ) = (DATA_TYPE)0;
         }
      }
      else
      {
         for (unsigned int x = 0; x < ROWS*COLS; ++x)
         {
            result.mData[x] = (DATA_TYPE)0;
         }
      }
      result.mState = Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL;
      return result;
   }


   /** matrix multiply.
    *  @PRE: With regard to size (ROWS/COLS): if lhs is m x p, and rhs is p x n, then result is m x n (mult func undefined otherwise)
    *  @POST: returns a m x n sized matrix
    *  @post: result = lhs * rhs  (where rhs is applied first)
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned INTERNAL, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& mult( Matrix<DATA_TYPE, ROWS, COLS>& result,
                 const Matrix<DATA_TYPE, ROWS, INTERNAL>& lhs,
                 const Matrix<DATA_TYPE, INTERNAL, COLS>& rhs )
   {
      Matrix<DATA_TYPE, ROWS, COLS> ret_mat; // prevent aliasing
      zero( ret_mat );

      // p. 150 Numerical Analysis (second ed.)
      // if A is m x p, and B is p x n, then AB is m x n
      // (AB)ij  =  [k = 1 to p] (a)ik (b)kj     (where:  1 <= i <= m, 1 <= j <= n)
      for (unsigned int i = 0; i < ROWS; ++i)           // 1 <= i <= m
      for (unsigned int j = 0; j < COLS; ++j)           // 1 <= j <= n
      for (unsigned int k = 0; k < INTERNAL; ++k)       // [k = 1 to p]
         ret_mat( i, j ) += lhs( i, k ) * rhs( k, j );

      // track state
      ret_mat.mState = combineMatrixStates( lhs.mState, rhs.mState );
      return result = ret_mat;
   }

   /** matrix * matrix.
    *  @PRE: With regard to size (ROWS/COLS): if lhs is m x p, and rhs is p x n, then result is m x n (mult func undefined otherwise)
    *  @POST: returns a m x n sized matrix == lhs * rhs (where rhs is applied first)
    *  returns a temporary, is slower.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned INTERNAL, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS> operator*( const Matrix<DATA_TYPE, ROWS, INTERNAL>& lhs,
                                                   const Matrix<DATA_TYPE, INTERNAL, COLS>& rhs )
   {
      Matrix<DATA_TYPE, ROWS, COLS> temporary;
      return mult( temporary, lhs, rhs );
   }

   /** matrix subtraction (algebraic operation for matrix).
    *  @PRE: if lhs is m x n, and rhs is m x n, then result is m x n (mult func undefined otherwise)
    *  @POST: returns a m x n matrix
    *  @TODO: <B>enforce the sizes with templates...</b>
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& sub( Matrix<DATA_TYPE, ROWS, COLS>& result,
                 const Matrix<DATA_TYPE, ROWS, COLS>& lhs,
                 const Matrix<DATA_TYPE, ROWS, COLS>& rhs )
   {
      // p. 150 Numerical Analysis (second ed.)
      // if A is m x n, and B is m x n, then AB is m x n
      // (A - B)ij  = (a)ij - (b)ij     (where:  1 <= i <= m, 1 <= j <= n)
      for (unsigned int i = 0; i < ROWS; ++i)           // 1 <= i <= m
      for (unsigned int j = 0; j < COLS; ++j)           // 1 <= j <= n
         result( i, j ) = lhs( i, j ) - rhs( i, j );

      // track state
      result.mState = combineMatrixStates( lhs.mState, rhs.mState );
      return result;
   }

   /** matrix addition (algebraic operation for matrix).
    *  @PRE: if lhs is m x n, and rhs is m x n, then result is m x n (mult func undefined otherwise)
    *  @POST: returns a m x n matrix
    *  TODO: <B>enforce the sizes with templates...</b>
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& add( Matrix<DATA_TYPE, ROWS, COLS>& result,
                 const Matrix<DATA_TYPE, ROWS, COLS>& lhs,
                 const Matrix<DATA_TYPE, ROWS, COLS>& rhs )
   {
      // p. 150 Numerical Analysis (second ed.)
      // if A is m x n, and B is m x n, then AB is m x n
      // (A - B)ij  = (a)ij + (b)ij     (where:  1 <= i <= m, 1 <= j <= n)
      for (unsigned int i = 0; i < ROWS; ++i)           // 1 <= i <= m
      for (unsigned int j = 0; j < COLS; ++j)           // 1 <= j <= n
         result( i, j ) = lhs( i, j ) + rhs( i, j );

      // track state
      result.mState = combineMatrixStates( lhs.mState, rhs.mState );
      return result;
   }

   /** matrix postmultiply.
    * @PRE: args must both be n x n (this function is undefined otherwise)
    * @POST: result' = result * operand
    */
   template <typename DATA_TYPE, unsigned SIZE>
   inline Matrix<DATA_TYPE, SIZE, SIZE>& postMult( Matrix<DATA_TYPE, SIZE, SIZE>& result,
                                                     const Matrix<DATA_TYPE, SIZE, SIZE>& operand )
   {
      return mult( result, result, operand );
   }

   /** matrix preMultiply.
    * @PRE: args must both be n x n (this function is undefined otherwise)
    * @POST: result' = operand * result
    */
   template <typename DATA_TYPE, unsigned SIZE>
   inline Matrix<DATA_TYPE, SIZE, SIZE>& preMult( Matrix<DATA_TYPE, SIZE, SIZE>& result,
                                                  const Matrix<DATA_TYPE, SIZE, SIZE>& operand )
   {
      return mult( result, operand, result );
   }

   /** matrix postmult (operator*=).
    * does a postmult on the matrix.
    * @PRE: args must both be n x n sized (this function is undefined otherwise)
    * @POST: result' = result * operand  (where operand is applied first)
    */
   template <typename DATA_TYPE, unsigned SIZE>
   inline Matrix<DATA_TYPE, SIZE, SIZE>& operator*=( Matrix<DATA_TYPE, SIZE, SIZE>& result,
                                                     const Matrix<DATA_TYPE, SIZE, SIZE>& operand )
   {
      return postMult( result, operand );
   }

   /** matrix scalar mult.
    *  mult each elt in a matrix by a scalar value.
    *  @POST: result = mat * scalar
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& mult( Matrix<DATA_TYPE, ROWS, COLS>& result, const Matrix<DATA_TYPE, ROWS, COLS>& mat, const DATA_TYPE& scalar )
   {
      for (unsigned i = 0; i < ROWS * COLS; ++i)
         result.mData[i] = mat.mData[i] * scalar;
      result.mState = mat.mState;
      return result;
   }

   /** matrix scalar mult.
    * mult each elt in a matrix by a scalar value.
    *  @POST: result *= scalar
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& mult( Matrix<DATA_TYPE, ROWS, COLS>& result, DATA_TYPE scalar )
   {
      for (unsigned i = 0; i < ROWS * COLS; ++i)
         result.mData[i] *= scalar;
      return result;
   }

   /** matrix scalar mult (operator*=).
    * multiply matrix elements by a scalar
    * @POST: result *= scalar
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& operator*=( Matrix<DATA_TYPE, ROWS, COLS>& result, const DATA_TYPE& scalar )
   {
      return mult( result, scalar );
   }

   /** matrix transpose in place.
    *  @PRE:  needs to be an N x N matrix
    *  @POST: flip along diagonal
    */
   template <typename DATA_TYPE, unsigned SIZE>
   Matrix<DATA_TYPE, SIZE, SIZE>& transpose( Matrix<DATA_TYPE, SIZE, SIZE>& result )
   {
      // p. 27 game programming gems #1
      for (unsigned c = 0; c < SIZE; ++c)
         for (unsigned r = c + 1; r < SIZE; ++r)
            std::swap( result( r, c ), result( c, r ) );

      return result;
   }

   /** matrix transpose from one type to another (i.e. 3x4 to 4x3)
    *  @PRE:  source needs to be an M x N matrix, while dest needs to be N x M
    *  @POST: flip along diagonal
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   Matrix<DATA_TYPE, ROWS, COLS>& transpose( Matrix<DATA_TYPE, ROWS, COLS>& result, const Matrix<DATA_TYPE, COLS, ROWS>& source )
   {
      // in case result is == source... :(
      Matrix<DATA_TYPE, COLS, ROWS> temp = source;

      // p. 149 Numerical Analysis (second ed.)
      for (unsigned i = 0; i < ROWS; ++i)
      {
         for (unsigned j = 0; j < COLS; ++j)
         {
            result( i, j ) = temp( j, i );
         }
      }
      result.mState = temp.mState;
      return result;
   }

   /** translational matrix inversion.
    *  Matrix inversion that acts on a translational matrix (matrix with only translation)
    *  Check for error with Matrix::isError().
    * @pre: 4x3, 4x4 matrices only
    * @post: result' = inv( result )
    * @post: If inversion failed, then error bit is set within the Matrix.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& invertTrans( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                       const Matrix<DATA_TYPE, ROWS, COLS>& src )
   {
      gmtlASSERT( ROWS == COLS || COLS == ROWS+1 && "invertTrans supports NxN or Nx(N-1) matrices only" );

      if (&result != &src)
         result = src; // could optimise this a little more (skip the trans copy), favor simplicity for now...
      for (unsigned x = 0; x < (ROWS-1+(COLS-ROWS)); ++x)
      {
         result[x][3] = -result[x][3];
      }
      return result;
   }

   /** orthogonal matrix inversion.
    *  Matrix inversion that acts on a affine matrix (matrix with only trans, rot, uniform scale)
    *  Check for error with Matrix::isError().
    * @pre: any size matrix
    * @post: result' = inv( result )
    * @post: If inversion failed, then error bit is set within the Matrix.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& invertOrthogonal( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                       const Matrix<DATA_TYPE, ROWS, COLS>& src )
   {
      // in case result is == source... :(
      Matrix<DATA_TYPE, ROWS, COLS> temp = src;

      // if 3x4, 2x3, etc...  can't transpose the last column
      const unsigned int size = Math::Min( ROWS, COLS );

      // p. 149 Numerical Analysis (second ed.)
      for (unsigned i = 0; i < size; ++i)
      {
         for (unsigned j = 0; j < size; ++j)
         {
            result( i, j ) = temp( j, i );
         }
      }
      result.mState = temp.mState;
      return result;
   }

   /** affine matrix inversion.
    *  Matrix inversion that acts on a 4x3 affine matrix (matrix with only trans, rot, uniform scale)
    *  Check for error with Matrix::isError().
    * @pre: 3x3, 4x3, 4x4 matrices only
    * @POST: result' = inv( result )
    * @POST: If inversion failed, then error bit is set within the Matrix.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& invertAffine( Matrix<DATA_TYPE, ROWS, COLS>& result,
                                                       const Matrix<DATA_TYPE, ROWS, COLS>& source )
   {
      static const float eps = 0.00000001f;

      // in case &result is == &source... :(
      Matrix<DATA_TYPE, ROWS, COLS> src = source;

      // The rotational part of the matrix is simply the transpose of the
      // original matrix.
      for (int x = 0; x < 3; ++x)
      for (int y = 0; y < 3; ++y)
      {
         result[x][y] = src[y][x];
      }

      // do non-uniform scale inversion
      if (src.mState & Matrix<DATA_TYPE, ROWS, COLS>::NON_UNISCALE)
      {
         DATA_TYPE l0 = gmtl::lengthSquared( gmtl::Vec<DATA_TYPE, 3>( result[0][0], result[0][1], result[0][2] ) );
         DATA_TYPE l1 = gmtl::lengthSquared( gmtl::Vec<DATA_TYPE, 3>( result[1][0], result[1][1], result[1][2] ) );
         DATA_TYPE l2 = gmtl::lengthSquared( gmtl::Vec<DATA_TYPE, 3>( result[2][0], result[2][1], result[2][2] ) );
         if (gmtl::Math::abs( l0 ) > eps) l0 = 1.0f / l0;
         if (gmtl::Math::abs( l1 ) > eps) l1 = 1.0f / l1;
         if (gmtl::Math::abs( l2 ) > eps) l2 = 1.0f / l2;
         // apply the inverse scale to the 3x3
         // for each axis: normalize it (1/length), and then mult by inverse scale (1/length)
         result[0][0] *= l0;
         result[0][1] *= l0;
         result[0][2] *= l0;
         result[1][0] *= l1;
         result[1][1] *= l1;
         result[1][2] *= l1;
         result[2][0] *= l2;
         result[2][1] *= l2;
         result[2][2] *= l2;
      }

      // handle matrices with translation
      if (COLS == 4)
      {
         // The right column vector of the matrix should always be [ 0 0 0 s ]
         // this represents some shear values
         result[3][0] = result[3][1] = result[3][2] = 0;

         // The translation components of the original matrix.
         const DATA_TYPE& tx = src[0][3];
         const DATA_TYPE& ty = src[1][3];
         const DATA_TYPE& tz = src[2][3];


         // Rresult = -(Tm * Rm) to get the translation part of the inverse
         if (ROWS == 4)
         {
            // invert scale.
            const DATA_TYPE tw = (gmtl::Math::abs( src[3][3] ) > eps) ? 1.0f / src[3][3] : 0.0f;

            // handle uniform scale in Nx4 matrices
            result[0][3] = -( result[0][0] * tx + result[0][1] * ty + result[0][2] * tz ) * tw;
            result[1][3] = -( result[1][0] * tx + result[1][1] * ty + result[1][2] * tz ) * tw;
            result[2][3] = -( result[2][0] * tx + result[2][1] * ty + result[2][2] * tz ) * tw;
            result[3][3] = tw;
         }
         else if (ROWS == 3)
         {
            result[0][3] = -( result[0][0] * tx + result[0][1] * ty + result[0][2] * tz );
            result[1][3] = -( result[1][0] * tx + result[1][1] * ty + result[1][2] * tz );
            result[2][3] = -( result[2][0] * tx + result[2][1] * ty + result[2][2] * tz );
         }
      }



      result.mState = src.mState;

      return result;
   }

   /** Full matrix inversion using Gauss-Jordan elimination.
    *  Check for error with Matrix::isError().
    * @POST: result' = inv( result )
    * @POST: If inversion failed, then error bit is set within the Matrix.
    */
   template <typename DATA_TYPE, unsigned SIZE>
   inline Matrix<DATA_TYPE, SIZE, SIZE>& invertFull_GJ( Matrix<DATA_TYPE, SIZE, SIZE>& result, const Matrix<DATA_TYPE, SIZE, SIZE>& src )
   {
      //gmtlASSERT( ROWS == COLS && "invertFull only works with nxn matrices" );

      const DATA_TYPE pivot_eps(1e-20);         // Epsilon for the pivot value test (delta with test against zero)

      // Computer inverse of matrix using a Gaussian-Jordan elimination.
      // Uses max pivot at each point
      // See: "Essential Mathmatics for Games" for description

      // Do this invert in place
      result = src;
      unsigned swapped[SIZE];       // Track swaps. swapped[3] = 2 means that row 3 was swapped with row 2

      unsigned pivot;

      // --- Gaussian elimination step --- //
      // For each column and row
      for(pivot=0; pivot<SIZE;++pivot)
      {
         unsigned    pivot_row(pivot);
         DATA_TYPE   pivot_value(gmtl::Math::abs(result(pivot_row, pivot)));    // Initialize to beginning of current row

         // find pivot row - (max pivot element)
         for(unsigned pr=pivot+1;pr<SIZE;++pr)
         {
            const DATA_TYPE cur_val(gmtl::Math::abs(result(pr,pivot)));   // get value at current point
            if (cur_val > pivot_value)
            {
               pivot_row = pr;
               pivot_value = cur_val;
            }
         }

         if(gmtl::Math::isEqual(DATA_TYPE(0),pivot_value,pivot_eps))
         {
            std::cerr << "*** pivot = " << pivot_value << " in mat_inv. ***\n";
            result.setError();
            return result;
         }

         // Check for swap of pivot rows
         swapped[pivot] = pivot_row;
         if(pivot_row != pivot)
         {
            for(unsigned c=0;c<SIZE;++c)
            {  std::swap(result(pivot,c), result(pivot_row,c)); }
         }
         // ASSERT: row to use in now in "row" (check that row starts with max pivot value found)
         gmtlASSERT(gmtl::Math::isEqual(pivot_value,gmtl::Math::abs(result(pivot,pivot)),DATA_TYPE(0.00001)));

         // Compute pivot factor
         const DATA_TYPE mult_factor(1.0f/pivot_value);

         // Set pivot row values
         for(unsigned c=0;c<SIZE;++c)
         {  result(pivot,c) *= mult_factor; }
         result(pivot,pivot) = mult_factor;    // Copy the 1/pivot since we are inverting in place

         // Clear pivot column in other rows (since we are in place)
         // - Subtract current row times result(r,col) so that column element becomes 0
         for(unsigned row=0;row<SIZE;++row)
         {
            if(row==pivot)     // Don't subtract from our row
            { continue; }

            const DATA_TYPE sub_mult_factor(result(row,pivot));

            // Clear the pivot column's element (for invers in place)
            // ends up being set to -sub_mult_factor*pivotInverse
            result(row,pivot) = 0;

            // subtract the pivot row from this row
            for(unsigned col=0;col<SIZE;++col)
            {   result(row,col) -= (sub_mult_factor*result(pivot,col)); }
         }
      } // end: gaussian substitution


      // Now undo the swaps in column direction in reverse order
      unsigned p(SIZE);
      do
      {
         --p;
         gmtlASSERT(p<SIZE);

         // If row was swapped
         if(swapped[p] != p)
         {
            // Swap the column with same index
            for(unsigned r=0; r<SIZE; ++r)
            { std::swap(result(r, p), result(r, swapped[p])); }
         }
      }
      while(p>0);

      return result;
   }


   /** full matrix inversion.
    *  Check for error with Matrix::isError().
    * @POST: result' = inv( result )
    * @POST: If inversion failed, then error bit is set within the Matrix.
    */
   template <typename DATA_TYPE, unsigned SIZE>
   inline Matrix<DATA_TYPE, SIZE, SIZE>& invertFull_orig( Matrix<DATA_TYPE, SIZE, SIZE>& result, const Matrix<DATA_TYPE, SIZE, SIZE>& src )
   {
      /*---------------------------------------------------------------------------*
       | mat_inv: Compute the inverse of a n x n matrix, using the maximum pivot   |
       |          strategy.  n <= MAX1.                                            |
       *---------------------------------------------------------------------------*

         Parameters:
             a        a n x n square matrix
             b        inverse of input a.
             n        dimenstion of matrix a.
      */

      const DATA_TYPE* a = src.getData();
      DATA_TYPE* b = result.mData;

      int   n(SIZE);
      int    i, j, k;
      int    r[SIZE], c[SIZE], row[SIZE], col[SIZE];
      DATA_TYPE  m[SIZE][SIZE*2], pivot, max_m, tmp_m, fac;

      /* Initialization */
      for ( i = 0; i < n; i ++ )
      {
         r[ i] = c[ i] = 0;
         row[ i] = col[ i] = 0;
      }

      /* Set working matrix */
      for ( i = 0; i < n; i++ )
      {
         for ( j = 0; j < n; j++ )
         {
            m[ i][ j] = a[ i * n + j];
            m[ i][ j + n] = ( i == j ) ? (DATA_TYPE)1.0 : (DATA_TYPE)0.0 ;
         }
      }

      /* Begin of loop */
      for ( k = 0; k < n; k++ )
      {
         /* Choosing the pivot */
         for ( i = 0, max_m = 0; i < n; i++ )
         {
            if ( row[ i]  )
               continue;
            for ( j = 0; j < n; j++ )
            {
               if ( col[ j] )
                  continue;
               tmp_m = gmtl::Math::abs( m[ i][ j]);
               if ( tmp_m > max_m)
               {
                  max_m = tmp_m;
                  r[ k] = i;
                  c[ k] = j;
               }
            }
         }
         row[ r[k] ] = col[ c[k] ] = 1;
         pivot = m[ r[ k] ][ c[ k] ];


         if ( gmtl::Math::abs( pivot) <= 1e-20)
         {
            std::cerr << "*** pivot = " << pivot << " in mat_inv. ***\n";
            result.setError();
            return result;
         }

         /* Normalization */
         for ( j = 0; j < 2*n; j++ )
         {
            if ( j == c[ k] )
               m[ r[ k]][ j] = (DATA_TYPE)1.0;
            else
               m[ r[ k]][ j] /= pivot;
         }

         /* Reduction */
         for ( i = 0; i < n; i++ )
         {
            if ( i == r[ k] )
               continue;

            for ( j=0, fac = m[ i][ c[k]]; j < 2*n; j++ )
            {
               if ( j == c[ k] )
                  m[ i][ j] = (DATA_TYPE)0.0;
               else
                  m[ i][ j] -= fac * m[ r[k]][ j];
            }
         }
      }

      /* Assign inverse to a matrix */
      for ( i = 0; i < n; i++ )
         for ( j = 0; j < n; j++ )
            row[ i] = ( c[ j] == i ) ? r[ j] : row[ i];

      for ( i = 0; i < n; i++ )
         for ( j = 0; j < n; j++ )
            b[ i * n +  j] = m[ row[ i]][ j + n];

      // It worked
      result.mState = src.mState;
      return result;
   }


   /** Invert method.
    * Calls invertFull_orig to do the work.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& invertFull( Matrix<DATA_TYPE, ROWS, COLS>& result, const Matrix<DATA_TYPE, ROWS, COLS>& src )
   {
      return invertFull_orig(result,src);
   }

   /** smart matrix inversion.
    *  Does matrix inversion by intelligently selecting what type of inversion to use depending
    *  on the types of operations your Matrix has been through.
    *
    *  5 types of inversion: FULL, AFFINE, ORTHONORMAL, ORTHOGONAL, IDENTITY.
    *
    *  Check for error with Matrix::isError().
    * @POST: result' = inv( result )
    * @POST: If inversion failed, then error bit is set within the Matrix.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& invert( Matrix<DATA_TYPE, ROWS, COLS>& result, const Matrix<DATA_TYPE, ROWS, COLS>& src )
   {
      if (src.mState == Matrix<DATA_TYPE, ROWS, COLS>::IDENTITY )
         return result = src;
      else if (src.mState == Matrix<DATA_TYPE, ROWS, COLS>::TRANS)
         return invertTrans( result, src );
      else if (src.mState == Matrix<DATA_TYPE, ROWS, COLS>::ORTHOGONAL)
         return invertOrthogonal( result, src );
      else if (src.mState == Matrix<DATA_TYPE, ROWS, COLS>::AFFINE ||
               src.mState == (Matrix<DATA_TYPE, ROWS, COLS>::AFFINE | Matrix<DATA_TYPE, ROWS, COLS>::NON_UNISCALE))
         return invertAffine( result, src );
      else
         return invertFull_orig( result, src );
   }

   /** smart matrix inversion (in place)
    *  Does matrix inversion by intelligently selecting what type of inversion to use depending
    *  on the types of operations your Matrix has been through.
    *
    *  5 types of inversion: FULL, AFFINE, ORTHONORMAL, ORTHOGONAL, IDENTITY.
    *
    *  Check for error with Matrix::isError().
    * @POST: result' = inv( result )
    * @POST: If inversion failed, then error bit is set within the Matrix.
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline Matrix<DATA_TYPE, ROWS, COLS>& invert( Matrix<DATA_TYPE, ROWS, COLS>& result )
   {
      return invert( result, result );
   }

/** @} */

/** @ingroup Compare
 * @name Matrix Comparitors
 * @{
 */

   /** Tests 2 matrices for equality
    *  @param lhs    The first matrix
    *  @param rhs    The second matrix
    *  @pre Both matrices must be of the same size.
    *  @return true if the matrices have the same element values; false otherwise
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline bool operator==( const Matrix<DATA_TYPE, ROWS, COLS>& lhs, const Matrix<DATA_TYPE, ROWS, COLS>& rhs )
   {
      for (unsigned int i = 0; i < ROWS*COLS; ++i)
      {
         if (lhs.mData[i] != rhs.mData[i])
         {
            return false;
         }
      }

      return true;

      /*  Would like this
      return( lhs[0] == rhs[0] &&
              lhs[1] == rhs[1] &&
              lhs[2] == rhs[2] );
      */
   }

   /** Tests 2 matrices for inequality
    *  @param lhs    The first matrix
    *  @param rhs    The second matrix
    *  @pre Both matrices must be of the same size.
    *  @return false if the matrices differ on any element value; true otherwise
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline bool operator!=( const Matrix<DATA_TYPE, ROWS, COLS>& lhs, const Matrix<DATA_TYPE, ROWS, COLS>& rhs )
   {
      return bool( !(lhs == rhs) );
   }

   /** Tests 2 matrices for equality within a tolerance
    *  @param lhs    The first matrix
    *  @param rhs    The second matrix
    *  @param eps    The tolerance value
    *  @pre Both matrices must be of the same size.
    *  @return true if the matrices' elements are within the tolerance value of each other; false otherwise
    */
   template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
   inline bool isEqual( const Matrix<DATA_TYPE, ROWS, COLS>& lhs, const Matrix<DATA_TYPE, ROWS, COLS>& rhs, const DATA_TYPE eps = 0 )
   {
      gmtlASSERT( eps >= (DATA_TYPE)0 );

      for (unsigned int i = 0; i < ROWS*COLS; ++i)
      {
         if (!Math::isEqual( lhs.mData[i], rhs.mData[i], eps ))
            return false;
      }
      return true;
   }
/** @} */

} // end of namespace gmtl

#endif
