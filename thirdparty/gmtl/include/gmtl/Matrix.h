// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_MATRIX_H_
#define _GMTL_MATRIX_H_

#include <gmtl/Defines.h>
#include <gmtl/Math.h>
#include <gmtl/Util/Assert.h>
#include <gmtl/Util/StaticAssert.h>

namespace gmtl
{

/**
 * State tracked NxM dimensional Matrix (ordered in memory by Column)
 *
 * <b>Memory mapping:</b>
 *
 * gmtl::Matrix stores its elements in column major order.
 * That is, it stores each column end-to-end in memory.
 *
 * Typically, for 3D transform matrices, the 3x3 rotation is
 * in the first three columns, while the translation is in the last column.
 *
 * This memory alignment is chosen for compatibility with the OpenGL graphics
 * API and others, which take matrices in this specific column major ordering
 * described above.
 *
 * See the interfaces for operator[r][c] and operator(r,c) for how to iterate
 * over columns and rows for a GMTL Matrix.
 *
 * <b>NOTES on Matrix memory layout and [][] accessors:</b>
 * <ul>
 * <li> gmtl Matrix memory is "column major" ordered, where columns are end
 *      to end in memory, while a C/C++ Matrix accessed the same way
 *      (using operator[][]) as a gmtl Matrix is "row major" ordered.
 *
 * <li> As a result, a gmtl matrix stores elements in memory transposed from
 *      the equivelent matrix defined using an array in the C/C++
 *      language, assuming they are accessed the same way (see example).
 * <ul>
 *    <li> Illustrative Example:                                           <br>
 *         Given two flavors of matrix, C/C++, and gmtl:                   <br>
 *             float cmat[n][m];   and    gmtl::Matrix<float, n, m> mat;   <br>
 *         Writing values into each, while accessing them the same:        <br>
 *             cmat[row][col] = mat[row][col] = some_values[x];            <br>
 *         Then reading values from the matrix array:                      <br>
 *             ((float*)cmat)   and    mat.getData()                       <br>
 *         <i>Will yield pointers to memory containing matrices that are the transpose of each other.</i>
 * </ul>
 * <li> In practice, the differences between GMTL and C/C++ defined matrices
 *      all depends how you iterate over your matrix.                                              <br>
 *      If gmtl is accessed mat[row][col] and C/C++ is accessed mat[col][row], then
 *      memory-wise, these two will yield the same memory mapping (column major as described above),
 *      thus, are equivelent and can both be used interchangably in many popular graphics APIs
 *      such as OpenGL, DirectX, and others.
 *
 * <li> In C/C++ access of a matrix via mat[row][col] yields this memory mapping after using ((float*)mat) to return it:<br>
 *  <pre>
 *    (0,0) (0,1) (0,2) (0,3)   <=== Contiguous memory arranged by row
 *    (1,0) (1,1) (1,2) (1,3)   <=== Contiguous
 *    (2,0) (2,1) (2,2) (2,3)   <=== Contiguous
 *    (3,0) (3,1) (3,2) (3,3)   <=== Contiguous
 *
 *  or linearly if you prefer:
 *    (0,0) (0,1) (0,2) (0,3) (1,0) (1,1) (1,2) (1,3) (2,0) (2,1) (2,2) (2,3) (3,0) (3,1) (3,2) (3,3)
 *  </pre>
 *
 * <li> In gmtl, access of a matrix via mat[row][col] yields this memory mapping after using getData() to return it:<br>
 *  <pre>
 *    (0,0) (0,1) (0,2) (0,3)
 *    (1,0) (1,1) (1,2) (1,3)
 *    (2,0) (2,1) (2,2) (2,3)
 *    (3,0) (3,1) (3,2) (3,3)
 *      ^     ^     ^     ^
 *    --1-----2-----3-----4---- Contiguous memory arranged by column
 *
 *  or linearly if you prefer:
 *    (0,0) (1,0) (2,0) (3,0) (0,1) (1,1) (2,1) (3,1) (0,2) (1,2) (2,2) (3,2) (0,3) (1,3) (2,3) (3,3)
 *  </pre>
 * </ul>
 *
 * <b>State Tracking:</b>
 *
 * The idea of a state-tracked matrix is that if we track the information
 * as it is stored into the matrix, then other operations could make more optimal
 * descisions based on the known state.  A good example is in matrix invertion,
 * a reletively costly operation for matrices.  However, if we know the matrix state
 * is (i.e.) ORTHOGONAL, then inversion becomes a simple transpose operation.
 * There are also optimizations with multiplication, as well as other.
 *
 * One side effect of this state tracking is that EVERY MATRIC FUNCTION NEEDS TO
 * TRACK STATE.  This means that anyone writing custom methods, or extentions to
 * gmtl, will need to pay close attention to matrix state.
 *
 * To facilitate state tracking in extensions, we've provided the function
 * gmtl::combineMatrixStates() to help in determining state based on two
 * combined matrices.
 * @see Matrix44f
 * @see Matrix44d
 * @ingroup Types
 */
template <typename DATA_TYPE, unsigned ROWS, unsigned COLS>
class Matrix
{
public:
   // This is a hack to work around a bug with GCC 3.3 on Mac OS X where
   // boost::is_polymorphic returns a false positive.  The details can be
   // found in the Boost.Python FAQ:
   //    http://www.boost.org/libs/python/doc/v2/faq.html#macosx
#if defined(__MACH__) && defined(__APPLE_CC__) && defined(__GNUC__) && \
    __GNUC__ == 3 && __GNUC_MINOR__ == 3
   bool dummy_;
#endif

   /** use this to declare single value types of the same type as this matrix.
    */
   typedef DATA_TYPE DataType;
   enum Params
   {
      Rows = ROWS, Cols = COLS
   };

   /** Helper class for Matrix op[].
   * This class encapsulates the row that the user is accessing
   * and implements a new op[] that passes the column to use
   */
   class RowAccessor
   {
   public:
      typedef DATA_TYPE DataType;

      RowAccessor(Matrix<DATA_TYPE,ROWS,COLS>* mat, const unsigned row)
         : mMat(mat), mRow(row)
      {
         gmtlASSERT(row < ROWS);
         gmtlASSERT(NULL != mat);
      }

      DATA_TYPE& operator[](const unsigned column)
      {
         gmtlASSERT(column < COLS);
         return (*mMat)(mRow,column);
      }

      Matrix<DATA_TYPE,ROWS,COLS>*  mMat;
      unsigned                      mRow;    /** The row being accessed */
   };

   /** Helper class for Matrix op[] const.
   * This class encapsulates the row that the user is accessing
   * and implements a new op[] that passes the column to use
   */
   class ConstRowAccessor
   {
   public:
      typedef DATA_TYPE DataType;

      ConstRowAccessor( const Matrix<DATA_TYPE,ROWS,COLS>* mat,
                        const unsigned row )
         : mMat( mat ), mRow( row )
      {
         gmtlASSERT( row < ROWS );
         gmtlASSERT( NULL != mat );
      }

      const DATA_TYPE& operator[](const unsigned column) const
      {
         gmtlASSERT(column < COLS);
         return (*mMat)(mRow,column);
      }

      const Matrix<DATA_TYPE,ROWS,COLS>*  mMat;
      unsigned                      mRow;    /** The row being accessed */
   };

   /** describes the xforms that this matrix has been through. */
   enum XformState
   {
      // identity matrix.
      IDENTITY = 1,

      // only translation, can simply negate that column
      TRANS = 2,

      // able to tranpose to get the inverse.  only rotation component is set
      ORTHOGONAL = 4,

      // orthogonal, and normalized axes.
      //ORTHONORMAL = 8,

      // leaves the homogeneous coordinate unchanged - that is, in which the last column is (0,0,0,s).
      // can include rotation, uniform scale, and translation, but no shearing or nonuniform scaling
      // This can optionally be combined with the NON_UNISCALE state to indicate there is also non-uniform scale
      AFFINE = 16,

      // AFFINE matrix with non-uniform scale, a matrix cannot
      // have this state without also having AFFINE (must be or'd together).
      NON_UNISCALE = 32,

      // fully set matrix containing more information than the above, or state is unknown,
      // or unclassifiable in terms of the above.
      FULL = 64,

      // error bit
      XFORM_ERROR = 128
   };

   /** Default Constructor (Identity constructor) */
   Matrix()
   {
      /** @todo mp */
      for (unsigned int r = 0; r < ROWS; ++r)
      {
         for (unsigned int c = 0; c < COLS; ++c)
         {   this->operator()( r, c ) = (DATA_TYPE)0.0; }
      }

      /** @todo mp */
      for (unsigned int x = 0; x < Math::Min( COLS, ROWS ); ++x)
      {  this->operator()( x, x ) = (DATA_TYPE)1.0; }

      /** @todo Set initial state to IDENTITY and test other stuff */
      mState = IDENTITY;
   }

   /** copy constructor */
   Matrix( const Matrix<DATA_TYPE, ROWS, COLS>& matrix )
   {
      this->set( matrix.getData() );
      mState = matrix.mState;
   }

   /** element wise setter for 2x2.
    * @note variable names specify the row,column number to put the data into
    *  @todo needs mp!!
    */
   void set( DATA_TYPE v00, DATA_TYPE v01,
             DATA_TYPE v10, DATA_TYPE v11 )
   {
      GMTL_STATIC_ASSERT( (ROWS == 2 && COLS == 2), Set_called_when_Matrix_not_of_size_2_2 );
      mData[0] = v00;
      mData[1] = v10;
      mData[2] = v01;
      mData[3] = v11;
      mState = FULL;
   }

   /** element wise setter for 2x3.
    * @todo needs mp!!
    */
   void set( DATA_TYPE v00, DATA_TYPE v01, DATA_TYPE v02,
             DATA_TYPE v10, DATA_TYPE v11, DATA_TYPE v12  )
   {
      GMTL_STATIC_ASSERT( (ROWS == 2 && COLS == 3), Set_called_when_Matrix_not_of_size_2_3 );
      mData[0] = v00;
      mData[1] = v10;
      mData[2] = v01;
      mData[3] = v11;
      mData[4] = v02;
      mData[5] = v12;
      mState = FULL;
   }

   /** element wise setter for 3x3.
    * @todo needs mp!!
    */
   void set( DATA_TYPE v00, DATA_TYPE v01, DATA_TYPE v02,
             DATA_TYPE v10, DATA_TYPE v11, DATA_TYPE v12,
             DATA_TYPE v20, DATA_TYPE v21, DATA_TYPE v22)
   {
      GMTL_STATIC_ASSERT( (ROWS == 3 && COLS == 3), Set_called_when_Matrix_not_of_size_3_3 );
      mData[0] = v00;
      mData[1] = v10;
      mData[2] = v20;

      mData[3] = v01;
      mData[4] = v11;
      mData[5] = v21;

      mData[6] = v02;
      mData[7] = v12;
      mData[8] = v22;
      mState = FULL;
   }

   /** element wise setter for 3x4.
    * @todo needs mp!!  currently no way for a 4x3, ....
    */
   void set( DATA_TYPE v00, DATA_TYPE v01, DATA_TYPE v02, DATA_TYPE v03,
             DATA_TYPE v10, DATA_TYPE v11, DATA_TYPE v12, DATA_TYPE v13,
             DATA_TYPE v20, DATA_TYPE v21, DATA_TYPE v22, DATA_TYPE v23)
   {
      GMTL_STATIC_ASSERT( (ROWS == 3 && COLS == 4), Set_called_when_Matrix_not_of_size_3_4 );
      mData[0] = v00;
      mData[1] = v10;
      mData[2] = v20;
      mData[3] = v01;
      mData[4] = v11;
      mData[5] = v21;
      mData[6] = v02;
      mData[7] = v12;
      mData[8] = v22;

      // right row
      mData[9]  = v03;
      mData[10] = v13;
      mData[11] = v23;
      mState = FULL;
   }

   /** element wise setter for 4x4.
    * @todo needs mp!!  currently no way for a 4x3, ....
    */
   void set( DATA_TYPE v00, DATA_TYPE v01, DATA_TYPE v02, DATA_TYPE v03,
             DATA_TYPE v10, DATA_TYPE v11, DATA_TYPE v12, DATA_TYPE v13,
             DATA_TYPE v20, DATA_TYPE v21, DATA_TYPE v22, DATA_TYPE v23,
             DATA_TYPE v30, DATA_TYPE v31, DATA_TYPE v32, DATA_TYPE v33 )
   {
      GMTL_STATIC_ASSERT( (ROWS == 4 && COLS == 4), Set_called_when_Matrix_not_of_size_4_4 );
      mData[0]  = v00;
      mData[1]  = v10;
      mData[2]  = v20;
      mData[4]  = v01;
      mData[5]  = v11;
      mData[6]  = v21;
      mData[8]  = v02;
      mData[9]  = v12;
      mData[10] = v22;

      // right row
      mData[12] = v03;
      mData[13] = v13;
      mData[14] = v23;

      // bottom row
      mData[3]  = v30;
      mData[7]  = v31;
      mData[11] = v32;
      mData[15] = v33;
      mState = FULL;
   }

   /** comma operator
    *  @todo implement this!
    */
   //void operator,()( DATA_TYPE b ) {}

   /** set the matrix to the given data.
    *  This function is useful to copy matrix data from another math library.
    *
    * <h3> "Example (to a matrix using an external math library):" </h3>
    * \code
    *    pfMatrix other_matrix;
    *    other_matrix.setRot( 90, 1, 0, 0 );
    *
    *    gmtl::Matrix44f mat;
    *    mat.set( other_matrix.getFloatPtr() );
    * \endcode
    *
    *  WARNING: this isn't really safe, size and datatype are not enforced by
    *           the compiler.
    * @pre data is in the native format of the gmtl::Matrix class, if not,
    *      then you might be able to use the setTranspose function.
    * @pre i.e. in a 4x4 data[0-3] is the 1st column, data[4-7] is 2nd, etc...
    */
   void set( const DATA_TYPE* data )
   {
      /** @todo mp */
      for (unsigned int x = 0; x < ROWS * COLS; ++x)
         mData[x] = data[x];
      mState = FULL;
   }

   /** set the matrix to the transpose of the given data.
    * normally set() takes raw matrix data in column by column order,
    * this function allows you to pass in row by row data.
    *
    * Normally you'll use this function if you want to use a float array
    * to init the matrix (see code example).
    *
    * <h3> "Example (to set a [15 -4 20] translation using float array):" </h3>
    * \code
    *    float data[] = { 1, 0, 0, 15,
    *                     0, 1, 0, -4,
    *                     0, 0, 1, 20,
    *                     0, 0, 0, 1   };
    *    gmtl::Matrix44f mat;
    *    mat.setTranspose( data );
    * \endcode
    *
    *  WARNING: this isn't really safe, size and datatype are not enforced by
    *           the compiler.
    * @pre ptr is in the transpose of the native format of the Matrix class
    * @pre i.e. in a 4x4 data[0-3] is the 1st row, data[4-7] is 2nd, etc...
    */
   void setTranspose( const DATA_TYPE* data )
   {
      /** @todo metaprog */
      for (unsigned int r = 0; r < ROWS; ++r)
      for (unsigned int c = 0; c < COLS; ++c)
         this->operator()( r, c ) = data[(r * COLS) + c];
      mState = FULL;
   }

   /** access [row, col] in the matrix
    *  WARNING: If you set data in the matrix (using this interface),
    *  you are required to set mState
    *  appropriately, failure to do so will result in incorrect
    *  calculations by other functions in GMTL.  If you are unsure
    *  about how to set mState, set it to FULL and you will be sure
    *  to get the correct result at the cost of some performance.
    */
   DATA_TYPE& operator()( const unsigned row, const unsigned column )
   {
      gmtlASSERT( (row < ROWS) && (column < COLS) );
      return mData[column*ROWS + row];
   }

   /** access [row, col] in the matrix (const version) */
   const DATA_TYPE&  operator()( const unsigned row, const unsigned column ) const
   {
      gmtlASSERT( (row < ROWS) && (column < COLS) );
      return mData[column*ROWS + row];
   }

   /** bracket operator
    *  WARNING: If you set data in the matrix (using this interface),
    *  you are required to set mState
    *  appropriately, failure to do so will result in incorrect
    *  calculations by other functions in GMTL.  If you are unsure
    *  about how to set mState, set it to FULL and you will be sure
    *  to get the correct result at the cost of some performance.
    */
   RowAccessor operator[]( const unsigned row )
   {
      return RowAccessor(this, row);
   }

   /** bracket operator (const version) */
   ConstRowAccessor operator[]( const unsigned row ) const
   {
      return ConstRowAccessor( this, row );
   }

   /*
   // bracket operator
   const DATA_TYPE& operator[]( const unsigned i ) const
   {
      gmtlASSERT( i < (ROWS*COLS) );
      return mData[i];
   }
   */

   /** Gets a DATA_TYPE pointer to the matrix data.
    * @return Returns a pointer to the head of the matrix data.
    */
   const DATA_TYPE*  getData() const { return (DATA_TYPE*)mData; }

   bool isError()
   {
      return mState & XFORM_ERROR;
   }
   void setError()
   {
      mState |= XFORM_ERROR;
   }

   void setState(int state)
   { mState = state; }

public:
   /** Column major.  In other words {Column1, Column2, Column3, Column4} in memory
    * access element mData[column][row]
    *  WARNING: If you set data in the matrix (using this interface),
    *  you are required to set mState appropriately,
    *  failure to do so will result in incorrect
    *  calculations by other functions in GMTL.  If you are unsure
    *  about how to set mState, set it to FULL and you will be sure
    *  to get the correct result at the cost of some performance.
    */
   DATA_TYPE mData[COLS*ROWS];

   /** describes what xforms are in this matrix */
   int mState;
};

typedef Matrix<float, 2, 2> Matrix22f;
typedef Matrix<double, 2, 2> Matrix22d;
typedef Matrix<float, 2, 3> Matrix23f;
typedef Matrix<double, 2, 3> Matrix23d;
typedef Matrix<float, 3, 3> Matrix33f;
typedef Matrix<double, 3, 3> Matrix33d;
typedef Matrix<float, 3, 4> Matrix34f;
typedef Matrix<double, 3, 4> Matrix34d;
typedef Matrix<float, 4, 4> Matrix44f;
typedef Matrix<double, 4, 4> Matrix44d;

/** 32bit floating point 2x2 identity matrix */
const Matrix22f MAT_IDENTITY22F = Matrix22f();

/** 64bit floating point 2x2 identity matrix */
const Matrix22d MAT_IDENTITY22D = Matrix22d();

/** 32bit floating point 2x2 identity matrix */
const Matrix23f MAT_IDENTITY23F = Matrix23f();

/** 64bit floating point 2x2 identity matrix */
const Matrix23d MAT_IDENTITY23D = Matrix23d();

/** 32bit floating point 3x3 identity matrix */
const Matrix33f MAT_IDENTITY33F = Matrix33f();

/** 64bit floating point 3x3 identity matrix */
const Matrix33d MAT_IDENTITY33D = Matrix33d();

/** 32bit floating point 3x4 identity matrix */
const Matrix34f MAT_IDENTITY34F = Matrix34f();

/** 64bit floating point 3x4 identity matrix */
const Matrix34d MAT_IDENTITY34D = Matrix34d();

/** 32bit floating point 4x4 identity matrix */
const Matrix44f MAT_IDENTITY44F = Matrix44f();

/** 64bit floating point 4x4 identity matrix */
const Matrix44d MAT_IDENTITY44D = Matrix44d();

/** utility function for use by matrix operations.
 *  given two matrices, when combined with set(..) or xform(..) types of operations,
 *  compute what matrixstate will the resulting matrix have?
 */
inline int combineMatrixStates( int state1, int state2 )
{
   switch (state1)
   {
   case Matrix44f::IDENTITY:
      switch (state2)
      {
      case Matrix44f::XFORM_ERROR: return state2;
      case Matrix44f::NON_UNISCALE: return Matrix44f::XFORM_ERROR;
      default: return state2;
      }
   case Matrix44f::TRANS:
      switch (state2)
      {
      case Matrix44f::IDENTITY: return state1;
      case Matrix44f::ORTHOGONAL: return Matrix44f::AFFINE;
      case Matrix44f::NON_UNISCALE: return Matrix44f::XFORM_ERROR;
      default: return state2;
      }
   case Matrix44f::ORTHOGONAL:
      switch (state2)
      {
      case Matrix44f::IDENTITY: return state1;
      case Matrix44f::TRANS: return Matrix44f::AFFINE;
      case Matrix44f::NON_UNISCALE: return Matrix44f::XFORM_ERROR;
      default: return state2;
      }
   case Matrix44f::AFFINE:
      switch (state2)
      {
      case Matrix44f::IDENTITY:
      case Matrix44f::TRANS:
      case Matrix44f::ORTHOGONAL:  return state1;
      case Matrix44f::NON_UNISCALE: return Matrix44f::XFORM_ERROR;
      case Matrix44f::AFFINE | Matrix44f::NON_UNISCALE:
      default: return state2;
      }
   case Matrix44f::AFFINE | Matrix44f::NON_UNISCALE:
      switch (state2)
      {
      case Matrix44f::IDENTITY:
      case Matrix44f::TRANS:
      case Matrix44f::ORTHOGONAL:
      case Matrix44f::AFFINE:  return state1;
      case Matrix44f::NON_UNISCALE: return Matrix44f::XFORM_ERROR;
      default: return state2;
      }
   case Matrix44f::FULL:
      switch (state2)
      {
      case Matrix44f::XFORM_ERROR: return state2;
      case Matrix44f::NON_UNISCALE: return Matrix44f::XFORM_ERROR;
      default: return state1;
      }
      break;
   default:
      return Matrix44f::XFORM_ERROR;
   }
}

} // end namespace gmtl



#endif
