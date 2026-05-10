// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_COORD_H_
#define _GMTL_COORD_H_

#include <gmtl/Defines.h>
#include <gmtl/Vec.h>
#include <gmtl/AxisAngle.h>
#include <gmtl/EulerAngle.h>
#include <gmtl/Quat.h>
#include <gmtl/Util/Meta.h>
#include <gmtl/Util/StaticAssert.h>

namespace gmtl
{
   
/** coord is a position/rotation pair. 
 *  coord consists of a position element and a rotation element.
 *
 * <h3> "How to define an Vector/Euler pair (32 bit float precision):" </h3>
 * \code
 *    Coord<Vec3f, EulerAngleXYZf> myEulerCoord;
 * \endcode
 *
 * <h3> "Or use the built in typedefs:" </h3>
 * \code
 *    CoordVec3fEulerAngleXYZf myEulerCoord;
 *    Coord3fQuat myOtherEulerCoord;
 * \endcode
 * @see Vec, AxisAngle, EulerAngle 
 * @ingroup Types
 */
template <typename POS_TYPE, typename ROT_TYPE>
class Coord
{
public:
   Coord() : mPos(), mRot()
   {
   }
   
   typedef typename POS_TYPE::DataType DataType;
   typedef POS_TYPE PosDataType;
   typedef ROT_TYPE RotDataType;
   enum Params
   {
       PosSize = POS_TYPE::Size,
       RotSize = ROT_TYPE::Size
   };
    
   Coord( const Coord<POS_TYPE, ROT_TYPE>& coord ) : mPos( coord.mPos ), mRot( coord.mRot )
   {
   }
   
   Coord( const POS_TYPE& pos, const ROT_TYPE& rot ) : mPos( pos ), mRot( rot )
   {
   }

   /** @name Multi-arg Constructors 
   *  Construct objects from primitive types
   *  Just assigns values in order to the pos and rot members' members.
   */
   //@{
   Coord( DataType a0, DataType a1, DataType a2, DataType a3, DataType a4, DataType a5 )
   {
      GMTL_STATIC_ASSERT(PosSize == 3, Using_incorrect_number_of_args_for_type_size);
      GMTL_STATIC_ASSERT(RotSize == 3, Using_incorrect_number_of_args_for_type_size);
      if(PosSize == 3)
      {
         mPos[0] = a0; mPos[1] = a1; mPos[2] = a2;
         mRot[0] = a3; mRot[1] = a4; mRot[2] = a5; 
      }
      else
      {
         gmtlASSERT(false && "Constructor not supported for pos size");
      }
   }

   Coord( DataType a0, DataType a1, DataType a2, DataType a3, DataType a4, DataType a5, DataType a6 )
   {
      GMTL_STATIC_ASSERT( (PosSize == 3 && RotSize == 4) || (PosSize == 4 && RotSize == 3), Using_incorrect_number_of_args_for_type_size);
      if(PosSize == 3)
      {
         mPos[0] = a0; mPos[1] = a1; mPos[2] = a2;
         mRot[0] = a3; mRot[1] = a4; mRot[2] = a5; mRot[3] = a6; 
      }
      else if(PosSize == 4)
      {
         mPos[0] = a0; mPos[1] = a1; mPos[2] = a2; mPos[3] = a3;
         mRot[0] = a4; mRot[1] = a5; mRot[2] = a6; 
      }
      else
      {
         gmtlASSERT(false && "Constructor not supported for pos size");
      }

   }
  
   Coord( DataType a0, DataType a1, DataType a2, DataType a3, DataType a4, DataType a5, DataType a6, DataType a7 )
   {
      GMTL_STATIC_ASSERT(PosSize == 4, Using_incorrect_number_of_args_for_type_size);
      GMTL_STATIC_ASSERT(RotSize == 4, Using_incorrect_number_of_args_for_type_size);
      if(PosSize == 4)
      {
         mPos[0] = a0; mPos[1] = a1; mPos[2] = a2; mPos[3] = a3;
         mRot[0] = a4; mRot[1] = a5; mRot[2] = a6; mRot[3] = a7;   
      }
      else
      {
         gmtlASSERT(false && "Constructor not supported for pos size");
      }
   }
   //@}

   const POS_TYPE& getPos() const { return mPos; }
   const ROT_TYPE& getRot() const { return mRot; }
   
   /// @todo what about having a pos, and a const_pos naming convention?
   /// @todo what about having a rot, and a const_rot naming convention?
   
   /** accessor to the position element */
   POS_TYPE& pos() { return mPos; }

   /** accessor to the rotation element */
   ROT_TYPE& rot() { return mRot; }
   
   /** const accessor to the position element */
   //const POS_TYPE& pos() const { return mPos; }

   /** const accessor to the rotation element */
   //const ROT_TYPE& rot() const  { return mRot; }

public:
   POS_TYPE mPos;
   ROT_TYPE mRot;
};

typedef Coord<Vec3d, EulerAngleXYZd> CoordVec3EulerAngleXYZd;
typedef Coord<Vec3f, EulerAngleXYZf> CoordVec3EulerAngleXYZf;
typedef Coord<Vec4d, EulerAngleXYZd> CoordVec4EulerAngleXYZd;
typedef Coord<Vec4f, EulerAngleXYZf> CoordVec4EulerAngleXYZf;

typedef Coord<Vec3d, EulerAngleZYXd> CoordVec3EulerAngleZYXd;
typedef Coord<Vec3f, EulerAngleZYXf> CoordVec3EulerAngleZYXf;
typedef Coord<Vec4d, EulerAngleZYXd> CoordVec4EulerAngleZYXd;
typedef Coord<Vec4f, EulerAngleZYXf> CoordVec4EulerAngleZYXf;

typedef Coord<Vec3d, EulerAngleZXYd> CoordVec3EulerAngleZXYd;
typedef Coord<Vec3f, EulerAngleZXYf> CoordVec3EulerAngleZXYf;
typedef Coord<Vec4d, EulerAngleZXYd> CoordVec4EulerAngleZXYd;
typedef Coord<Vec4f, EulerAngleZXYf> CoordVec4EulerAngleZXYf;

typedef Coord<Vec3d, AxisAngled> CoordVec3AxisAngled;
typedef Coord<Vec3f, AxisAnglef> CoordVec3AxisAnglef;
typedef Coord<Vec4d, AxisAngled> CoordVec4AxisAngled;
typedef Coord<Vec4f, AxisAnglef> CoordVec4AxisAnglef;



/** 3 elt types */
typedef Coord<Vec3f, EulerAngleXYZf> Coord3fXYZ;
typedef Coord<Vec3f, EulerAngleZYXf> Coord3fZYX;
typedef Coord<Vec3f, EulerAngleZXYf> Coord3fZXY;
typedef Coord<Vec3d, EulerAngleXYZd> Coord3dXYZ;
typedef Coord<Vec3d, EulerAngleZYXd> Coord3dZYX;
typedef Coord<Vec3d, EulerAngleZXYd> Coord3dZXY;

/** 4 elt types */
typedef Coord<Vec4f, EulerAngleXYZf> Coord4fXYZ;
typedef Coord<Vec4f, EulerAngleZYXf> Coord4fZYX;
typedef Coord<Vec4f, EulerAngleZXYf> Coord4fZXY;
typedef Coord<Vec4d, EulerAngleXYZd> Coord4dXYZ;
typedef Coord<Vec4d, EulerAngleZYXd> Coord4dZYX;
typedef Coord<Vec4d, EulerAngleZXYd> Coord4dZXY;

/** 3 elt types */
typedef Coord<Vec3f, Quatf> Coord3fQuat;
typedef Coord<Vec3d, Quatd> Coord3dQuat;

/** 4 elt types */
typedef Coord<Vec4f, Quatf> Coord4fQuat;
typedef Coord<Vec4d, Quatd> Coord4dQuat;


/** 3 elt types */
typedef Coord<Vec3f, AxisAnglef> Coord3fAxisAngle;
typedef Coord<Vec3d, AxisAngled> Coord3dAxisAngle;

/** 4 elt types */
typedef Coord<Vec4f, AxisAnglef> Coord4fAxisAngle;
typedef Coord<Vec4d, AxisAngled> Coord4dAxisAngle;

}

#endif
