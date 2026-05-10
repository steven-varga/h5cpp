// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_FRUSTUM_H_
#define _GMTL_FRUSTUM_H_

#include <gmtl/Defines.h>
#include <gmtl/Plane.h>
#include <gmtl/MatrixOps.h>


namespace gmtl
{

/**
 * This class defines a View Frustum Volume as a set of 6 planes
 *
 * @ingroup Types
 */
template<typename DATA_TYPE>
class Frustum
{
public:
   typedef DATA_TYPE DataType;
   typedef Frustum<DATA_TYPE> FrustumType;

   /**
    * An enum to name the plane indicies. 
    * To have you not must remember those numbers.
    */
   enum PlaneNames
   {
      PLANE_LEFT = 0,      /**< left   clipping plane equals 0 */
      PLANE_RIGHT = 1,   /**< right   clipping plane equals 1 */
      PLANE_BOTTOM = 2,   /**< bottom   clipping plane equals 2 */
      PLANE_TOP = 3,      /**< top   clipping plane equals 3 */
      PLANE_NEAR = 4,      /**< near   clipping plane equals 4 */
      PLANE_FAR = 5      /**< far   clipping plane equals 5 */
   };

   /**
    * Constructs a new frustum with all planes in default state.
    */
   Frustum()
   {
   
   }

   /**
    * Constructs a new frustum with the given projection matrix.
    *
    * @param projMatrix The projection matrix of your camera or light etc. to
    *                   construct the planes from.
    */
   Frustum(const gmtl::Matrix<DATA_TYPE, 4, 4>& projMatrix)
   {
      extractPlanes(projMatrix);
   }

   /**
    * Constructs a new frustum with given projection and modelview matricies.
    * the matricies are multiplied in this order:
    *
    * \code
    * M = projMatrix * modelviewMatrix
    * \endcode
    *
    * The planes are then extracted from M.
    *
    * @param modelviewMatrix The modelview matrix of you camera or light etc.
    *                        to construct the planes from.
    * @param projMatrix      The projection matrix of your camera or light or
    *                        whatever.
    */
   Frustum(const gmtl::Matrix<DATA_TYPE, 4, 4>& modelviewMatrix,
           const gmtl::Matrix<DATA_TYPE, 4, 4>& projMatrix)
   {
      extractPlanes(modelviewMatrix, projMatrix);
   }

   /**
    * Extracts the planes from the given projection matrix.
    *
    * @param projMatrix The projection matrix of you camera or light or
    *                   what ever.
    */
   void extractPlanes(const gmtl::Matrix<DATA_TYPE, 4, 4>& modelviewMatrix,
                      const gmtl::Matrix<DATA_TYPE, 4, 4>& projMatrix)
   {
      extractPlanes(projMatrix * modelviewMatrix);
   }

   /**
    * Extracts the planes from the given projection and modelview matricies.
    * The matricies are multiplied in this order:
    *
    * \code
    * M = projMatrix * modelviewMatrix
    * \endcode
    *
    * The planes are then extracted from M.
    *
    * @param modelviewMatrix The modelview matrix of you camera or light etc.
    *                        to construct the planes from.
    * @param projMatrix      The projection matrix of you camera or light etc.
    *                        to construct the planes from.
    */
   void extractPlanes(const gmtl::Matrix<DATA_TYPE, 4, 4>& projMatrix)
   {
      const gmtl::Matrix<DATA_TYPE, 4, 4>& m = projMatrix;

      //left
      mPlanes[PLANE_LEFT].setNormal(gmtl::Vec<DATA_TYPE, 3>(m[3][0] + m[0][0],
                                                            m[3][1] + m[0][1],
                                                            m[3][2] + m[0][2]));
      mPlanes[PLANE_LEFT].setOffset(m[3][3] + m[0][3]);
      //right
      mPlanes[PLANE_RIGHT].setNormal(gmtl::Vec<DATA_TYPE, 3>(m[3][0] - m[0][0],
                                                             m[3][1] - m[0][1],
                                                             m[3][2] - m[0][2]));
      mPlanes[PLANE_RIGHT].setOffset(m[3][3] - m[0][3]);
      //bottom
      mPlanes[PLANE_BOTTOM].setNormal(gmtl::Vec<DATA_TYPE, 3>(m[3][0] + m[1][0],
                                                              m[3][1] + m[1][1],
                                                              m[3][2] + m[1][2]));
      mPlanes[PLANE_BOTTOM].setOffset(m[3][3] + m[1][3]);
      //top
      mPlanes[PLANE_TOP].setNormal(gmtl::Vec<DATA_TYPE, 3>(m[3][0] - m[1][0],
                                                           m[3][1] - m[1][1],
                                                           m[3][2] - m[1][2]));
      mPlanes[PLANE_TOP].setOffset(m[3][3] - m[1][3]);
      //near
      mPlanes[PLANE_NEAR].setNormal(gmtl::Vec<DATA_TYPE, 3>(m[3][0] + m[2][0],
                                                            m[3][1] + m[2][1],
                                                            m[3][2] + m[2][2]));
      mPlanes[PLANE_NEAR].setOffset(m[2][3] + m[3][3]);
      //far
      mPlanes[PLANE_FAR].setNormal(gmtl::Vec<DATA_TYPE, 3>(m[3][0] - m[2][0],
                                                           m[3][1] - m[2][1],
                                                           m[3][2] - m[2][2]));
      mPlanes[PLANE_FAR].setOffset(m[3][3] - m[2][3]);
   }

   gmtl::Plane<DATA_TYPE> mPlanes[6];
};

typedef Frustum<float> Frustumf;
typedef Frustum<double> Frustumd;

}


#endif /* _GMTL_FRUSTUM_H_ */
