// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_VEC_H_
#define _GMTL_VEC_H_

#include <gmtl/Defines.h>
#include <gmtl/Config.h>
#include <gmtl/VecBase.h>
#include <gmtl/Util/StaticAssert.h>

namespace gmtl
{

/**
 * A representation of a vector with SIZE components using DATA_TYPE as the data
 * type for each component.
 *
 * @param DATA_TYPE     the datatype to use for the components
 * @param SIZE          the number of components this VecBase has
 * @see Vec3f
 * @see Vec4f
 * @see Vec3d
 * @see Vec4f
 * @ingroup Types
 */
template<class DATA_TYPE, unsigned SIZE>
#ifdef GMTL_NO_METAPROG
class Vec : public VecBase<DATA_TYPE, SIZE>
#else
class Vec : public VecBase<DATA_TYPE, SIZE, meta::DefaultVecTag>
#endif
{
public:
   /// The datatype used for the components of this Vec.
   typedef DATA_TYPE DataType;

   /// The number of components this Vec has.
   enum Params { Size = SIZE };

   /// The superclass type.
   typedef VecBase<DATA_TYPE, SIZE> BaseType;
   typedef Vec<DATA_TYPE, SIZE> VecType;

public:
   /**
    * Default constructor. All components are initialized to zero.
    */
   Vec()
   {
      for (unsigned i = 0; i < SIZE; ++i)
         this->mData[i] = (DATA_TYPE)0;
   }

   /// @name Value constructors
   //@{
   /**
    * Make an exact copy of the given Vec object.
    * @pre  Vector should be the same size and type as the one copied
    * @param rVec    the Vec object to copy
    */
   /*
   Vec( const Vec<DATA_TYPE, SIZE>& rVec )
      : BaseType( static_cast<BaseType>( rVec ) )
   {;}
   */

#ifdef GMTL_NO_METAPROG
   Vec( const VecBase<DATA_TYPE, SIZE>& rVec )
      : BaseType( rVec )
   {
   }
#else
   template<typename REP2>
   Vec( const VecBase<DATA_TYPE, SIZE, REP2>& rVec )
      : BaseType( rVec )
   {
   }
#endif

   /**
    * Creates a new Vec initialized to the given values.
    */
   Vec(const DATA_TYPE& val0,const DATA_TYPE& val1)
   : BaseType(val0, val1)
   {
      GMTL_STATIC_ASSERT( SIZE == 2, Out_Of_Bounds_Element_Access_In_Vec );
   }

   Vec(const DATA_TYPE& val0,const DATA_TYPE& val1,const DATA_TYPE& val2)
   : BaseType(val0, val1, val2)
   {
      GMTL_STATIC_ASSERT( SIZE == 3, Out_Of_Bounds_Element_Access_In_Vec );
   }

   Vec(const DATA_TYPE& val0,const DATA_TYPE& val1,const DATA_TYPE& val2,const DATA_TYPE& val3)
   : BaseType(val0, val1, val2, val3)
   {
      GMTL_STATIC_ASSERT( SIZE == 4, Out_Of_Bounds_Element_Access_In_Vec );
   }
   //@}

   /** Assign from different rep. */
#ifdef GMTL_NO_METAPROG
   inline VecType& operator=(const VecBase<DATA_TYPE,SIZE>& rhs)
   {
      BaseType::operator=(rhs);
      return *this;
   }
#else
   template<typename REP2>
   inline VecType& operator=(const VecBase<DATA_TYPE,SIZE,REP2>& rhs)
   {
      BaseType::operator=(rhs);
      return *this;
   }
#endif
};

// --- helper types --- //
typedef Vec<int, 2> Vec2i;
typedef Vec<float,2> Vec2f;
typedef Vec<double,2> Vec2d;
typedef Vec<int, 3> Vec3i;
typedef Vec<float,3> Vec3f;
typedef Vec<double,3> Vec3d;
typedef Vec<int, 4> Vec4i;
typedef Vec<float,4> Vec4f;
typedef Vec<double,4> Vec4d;

}

#endif
