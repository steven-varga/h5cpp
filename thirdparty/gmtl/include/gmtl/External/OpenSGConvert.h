// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef GMTL_OPENSG_CONVERT_H_
#define GMTL_OPENSG_CONVERT_H_

/** @file OpenSGConvert.h GMTL/OpenSG conversion functions
 * 
 * Methods to convert between GTML and OpenSG matrix classes.
 */

#include <gmtl/Matrix.h>
#include <gmtl/Generate.h>
#include <OpenSG/OSGMatrix.h>

namespace gmtl
{

/**
 * Converts an OpenSG matrix to a gmtl::Matrix.
 *
 * @param mat        The matrix to write the OpenSG matrix data into.
 * @param osgMat     The source OpenSG matrix.
 *
 * @return The equivalent GMTL matrix.
 */
inline Matrix44f& set(Matrix44f& mat, const OSG::Matrix& osgMat)
{
   mat.set(osgMat.getValues());
   return mat;
}

/**
 * Converts a GMTL matrix to an OpenSG matrix.
 *
 * @param osgMat     The matrix to write the GMTL matrix data into.
 * @param mat        The source GMTL matrix.
*
 * @return The equivalent OpenSG matrix.
 */
inline OSG::Matrix& set(OSG::Matrix& osgMat, const Matrix44f& mat)
{
   osgMat.setValue(mat.getData());
   return osgMat;
}

}

#endif
