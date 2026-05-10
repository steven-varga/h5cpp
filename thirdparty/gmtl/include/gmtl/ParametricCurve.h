// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

/********************************************************************
created:	2010/01/14
created:	14:1:2010   22:14
filename: 	ParametricCurve.h
author:		Gao Yang

purpose:	Defines a series of parametric curves include linear,
		quadratic, Bezier, Hermite, Catmull-Rom, B-Spline. Custom
		curve definitions are also possible via user-defined basis
		matrices.
*********************************************************************/

#ifndef _GMTL_PARAMETRIC_CURVE_H_
#define _GMTL_PARAMETRIC_CURVE_H_

#include <gmtl/Matrix.h>
#include <gmtl/MatrixOps.h>
#include <gmtl/Vec.h>
#include <gmtl/VecOps.h>

namespace gmtl
{

/**
 * A base representation of a parametric curve with SIZE component using
 * DATA_TYPE as the data type, ORDER as the order for each component.
 *
 * @tparam DATA_TYPE The data type to use for the components.
 * @tparam SIZE      The number of components this curve has.
 * @tparam ORDER     The order of this curve.
 *
 * @since 0.6.1
 */
template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
class ParametricCurve
{
public:
   ParametricCurve();
   ParametricCurve(const ParametricCurve& other);
   ~ParametricCurve();
   ParametricCurve& operator=(const ParametricCurve& other);

   void setWeights(DATA_TYPE weights[ORDER]);
   void setControlPoints(Vec<DATA_TYPE, SIZE> control_points[ORDER]);
   void setBasisMatrix(const Matrix<DATA_TYPE, ORDER, ORDER>& basis_matrix);
   Vec<DATA_TYPE, SIZE> getInterpolatedValue(DATA_TYPE value) const;
   Vec<DATA_TYPE, SIZE> getInterpolatedDerivative(DATA_TYPE value) const;

protected:
   DATA_TYPE mWeights[ORDER];
   Vec<DATA_TYPE, SIZE> mControlPoints[ORDER];
   Matrix<DATA_TYPE, ORDER, ORDER> mBasisMatrix;
};

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
ParametricCurve<DATA_TYPE, SIZE, ORDER>::ParametricCurve()
{
   for (unsigned int i = 0; i < ORDER; ++i)
   {
      mWeights[i] = (DATA_TYPE)1.0;
   }
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
ParametricCurve<DATA_TYPE, SIZE, ORDER>::
ParametricCurve(const ParametricCurve<DATA_TYPE, SIZE, ORDER>& other)
{
   *this = other;
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
ParametricCurve<DATA_TYPE, SIZE, ORDER>::~ParametricCurve()
{
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
ParametricCurve<DATA_TYPE, SIZE, ORDER>& ParametricCurve<DATA_TYPE, SIZE,
ORDER>::operator=(const ParametricCurve<DATA_TYPE, SIZE, ORDER>& other)
{
   for (unsigned int i = 0; i < ORDER; ++i)
   {
      mWeights[i] = other.mWeights[i];
      mControlPoints[i] = other.mControlPoints[i];
   }

   mBasisMatrix = other.mBasisMatrix;

   return *this;
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
void ParametricCurve<DATA_TYPE, SIZE, ORDER>::
setWeights(const DATA_TYPE weights[ORDER])
{
   for (unsigned int i = 0; i < ORDER; ++i)
   {
      mWeights[i] = weights[i];
   }
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
void ParametricCurve<DATA_TYPE, SIZE, ORDER>::
setControlPoints(const Vec<DATA_TYPE, SIZE>& control_points[ORDER])
{
   for (unsigned int i = 0; i < ORDER; ++i)
   {
      mControlPoints[i] = control_points[i];
   }
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
void ParametricCurve<DATA_TYPE, SIZE, ORDER>::
setBasisMatrix(const Matrix<DATA_TYPE, ORDER, ORDER>& basis_matrix)
{
   mBasisMatrix = basis_matrix;
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
Vec<DATA_TYPE, SIZE> ParametricCurve<DATA_TYPE, SIZE, ORDER>::
getInterpolatedValue(const DATA_TYPE value) const
{
   Vec<DATA_TYPE, SIZE> ret_vec;
   DATA_TYPE power_vector[ORDER];
   DATA_TYPE exponent;
   DATA_TYPE coefficient[ORDER];

   for (unsigned int i = 0; i < ORDER; ++i)
   {
      exponent = (DATA_TYPE)(ORDER - i - 1);
      power_vector[i] = Math::pow(value, exponent);
   }

   for (unsigned int column = 0; column < ORDER; ++column)
   {
      coefficient[column] = (DATA_TYPE)0.0;

      for (unsigned int row = 0; row < ORDER; ++row)
      {
         coefficient[column] += power_vector[row] * mBasisMatrix[row][column];
      }

      ret_vec += coefficient[column] * mWeights[column] * mControlPoints[column];
   }

   return ret_vec;
}

template<typename DATA_TYPE, unsigned SIZE, unsigned ORDER>
Vec<DATA_TYPE, SIZE> ParametricCurve<DATA_TYPE, SIZE, ORDER>::
getInterpolatedDerivative(const DATA_TYPE value) const
{
   Vec<DATA_TYPE, SIZE> ret_vec;
   DATA_TYPE power_vector[ORDER];
   DATA_TYPE exponent;
   DATA_TYPE coefficient[ORDER];

   for (unsigned int i = 0; i < ORDER; ++i)
   {
      exponent = static_cast<DATA_TYPE>(ORDER - i - 1);

      if (exponent > 0)
      {
         power_vector[i] = exponent * Math::pow(value, exponent - 1);
      }
      else
      {
         power_vector[i] = (DATA_TYPE)0.0;
      }
   }

   for (unsigned int column = 0; column < ORDER; ++column)
   {
      coefficient[column] = static_cast<DATA_TYPE>(0.0);

      for (unsigned int row = 0; row < ORDER; ++row)
      {
         coefficient[column] += power_vector[row] * mBasisMatrix[row][column];
      }

      ret_vec += coefficient[column] * mWeights[column] * mControlPoints[column];
   }

   return ret_vec;
}

/**
 * A representation of a line with order set to 2.
 *
 * @tparam DATA_TYPE The data type to use for the components.
 * @tparam SIZE      The number of components this curve has.
 */
template <typename DATA_TYPE, unsigned int SIZE>
class LinearCurve : public ParametricCurve<DATA_TYPE, SIZE, 2>
{
public:
   LinearCurve();
   LinearCurve(const LinearCurve& other);
   ~LinearCurve();
   LinearCurve& operator=(const LinearCurve& other);

   void makeLerp();
};

template <typename DATA_TYPE, unsigned int SIZE>
LinearCurve<DATA_TYPE, SIZE>::LinearCurve()
{
}

template <typename DATA_TYPE, unsigned int SIZE>
LinearCurve<DATA_TYPE, SIZE>::LinearCurve(const LinearCurve& other)
{
   *this = other;
}

template <typename DATA_TYPE, unsigned int SIZE>
LinearCurve<DATA_TYPE, SIZE>::~LinearCurve()
{
}

template <typename DATA_TYPE, unsigned int SIZE>
LinearCurve<DATA_TYPE, SIZE>&
LinearCurve<DATA_TYPE, SIZE>::operator=(const LinearCurve& other)
{
   ParametricCurve::operator =(other);

   return *this;
}

template <typename DATA_TYPE, unsigned int SIZE>
void LinearCurve<DATA_TYPE, SIZE>::makeLerp()
{
   mBasisMatrix.set(
      -1.0, 1.0,
      1.0, 0.0
   );
}

/**
 * A representation of a quadratic curve with order set to 3.
 *
 * @tparam DATA_TYPE The data type to use for the components.
 * @tparam SIZE      The number of components this curve has.
 */
template<typename DATA_TYPE, unsigned SIZE>
class QuadraticCurve : public ParametricCurve<DATA_TYPE, SIZE, 3>
{
public:
   QuadraticCurve();
   QuadraticCurve(const QuadraticCurve& other);
   ~QuadraticCurve();
   QuadraticCurve& operator=(const QuadraticCurve& other);

   void makeBezier();
};

template<typename DATA_TYPE, unsigned SIZE>
QuadraticCurve<DATA_TYPE, SIZE>::QuadraticCurve()
{
}

template<typename DATA_TYPE, unsigned SIZE>
QuadraticCurve<DATA_TYPE, SIZE>::QuadraticCurve(const QuadraticCurve& other)
{
   *this = other;
}

template<typename DATA_TYPE, unsigned SIZE>
QuadraticCurve<DATA_TYPE, SIZE>::~QuadraticCurve()
{
}

template<typename DATA_TYPE, unsigned SIZE>
QuadraticCurve<DATA_TYPE, SIZE>&
QuadraticCurve<DATA_TYPE, SIZE>::operator=(const QuadraticCurve& other)
{
   ParametricCurve::operator =(other);

   return *this;
}

template<typename DATA_TYPE, unsigned SIZE>
void QuadraticCurve<DATA_TYPE, SIZE>::makeBezier()
{
   mBasisMatrix.set(
      1.0, -2.0, 1.0,
      -2.0, 2.0, 0.0,
      1.0, 0.0, 0.0
   );
}

/**
 * A representation of a cubic curve with order set to 4.
 *
 * @tparam DATA_TYPE The data type to use for the components.
 * @tparam SIZE      The number of components this curve has.
 */
template<typename DATA_TYPE, unsigned SIZE>
class CubicCurve : public ParametricCurve<DATA_TYPE, SIZE, 4>
{
public:
   CubicCurve();
   CubicCurve(const CubicCurve& other);
   ~CubicCurve();
   CubicCurve& operator=(const CubicCurve& other);

   void makeBezier();
   void makeCatmullRom();
   void makeHermite();
   void makeBspline();
};

template<typename DATA_TYPE, unsigned SIZE>
CubicCurve<DATA_TYPE, SIZE>::CubicCurve()
{
}

template<typename DATA_TYPE, unsigned SIZE>
CubicCurve<DATA_TYPE, SIZE>::CubicCurve(const CubicCurve& other)
{
   *this = other;
}

template<typename DATA_TYPE, unsigned SIZE>
CubicCurve<DATA_TYPE, SIZE>::~CubicCurve()
{
}

template<typename DATA_TYPE, unsigned SIZE>
CubicCurve<DATA_TYPE, SIZE>&
CubicCurve<DATA_TYPE, SIZE>::operator=(const CubicCurve& other)
{
   ParametricCurve::operator =(other);

   return *this;
}

template<typename DATA_TYPE, unsigned SIZE>
void CubicCurve<DATA_TYPE, SIZE>::makeBezier()
{
   mBasisMatrix.set(
      -1.0, 3.0, -3.0, 1.0,
      3.0, -6.0, 3.0, 0.0,
      -3.0, 3.0, 0.0, 0.0,
      1.0, 0.0, 0.0, 0.0
   );
}

template<typename DATA_TYPE, unsigned SIZE>
void CubicCurve<DATA_TYPE, SIZE>::makeCatmullRom()
{
   mBasisMatrix.set(
      -0.5, 1.5, -1.5, 0.5,
      1.0, -2.5, 2.0, -0.5,
      -0.5, 0.0, 0.5, 0.0,
      0.0, 1.0, 0.0, 0.0
   );
}

template<typename DATA_TYPE, unsigned SIZE>
void CubicCurve<DATA_TYPE, SIZE>::makeHermite()
{
   mBasisMatrix.set(
      2.0, -2.0, 1.0, 1.0,
      -3.0, 3.0, -2.0, -1.0,
      0.0, 0.0, 1.0, 0.0,
      1.0, 0.0, 0.0, 0.0
   );
}

template<typename DATA_TYPE, unsigned SIZE>
void CubicCurve<DATA_TYPE, SIZE>::makeBspline()
{
   mBasisMatrix.set(
      -1.0 / 6.0, 0.5, -0.5, 1.0 / 6.0,
      0.5, -1.0, 0.5, 0.0,
      -0.5, 0.0, 0.5, 0.0,
      1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0, 0.0
   );
}

// --- helper types --- //
typedef LinearCurve<float, 1> LinearCurve1f;
typedef LinearCurve<float, 2> LinearCurve2f;
typedef LinearCurve<float, 3> LinearCurve3f;
typedef LinearCurve<double, 1> LinearCurve1d;
typedef LinearCurve<double, 2> LinearCurve2d;
typedef LinearCurve<double, 3> LinearCurve3d;
typedef QuadraticCurve<float, 1> QuadraticCurve1f;
typedef QuadraticCurve<float, 2> QuadraticCurve2f;
typedef QuadraticCurve<float, 3> QuadraticCurve3f;
typedef QuadraticCurve<double, 1> QuadraticCurve1d;
typedef QuadraticCurve<double, 2> QuadraticCurve2d;
typedef QuadraticCurve<double, 3> QuadraticCurve3d;
typedef CubicCurve<float, 1> CubicCurve1f;
typedef CubicCurve<float, 2> CubicCurve2f;
typedef CubicCurve<float, 3> CubicCurve3f;
typedef CubicCurve<double, 1> CubicCurve1d;
typedef CubicCurve<double, 2> CubicCurve2d;
typedef CubicCurve<double, 3> CubicCurve3d;

}

#endif
