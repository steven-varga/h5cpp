// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_VECBASE_H_
#define _GMTL_VECBASE_H_

#include <gmtl/Defines.h>
#include <gmtl/Util/Assert.h>
#include <gmtl/Util/StaticAssert.h>
#include <gmtl/Util/Meta.h>
#include <gmtl/Config.h>
#include <gmtl/Helpers.h>


namespace gmtl
{

#ifndef GMTL_NO_METAPROG
namespace meta
{
   struct DefaultVecTag
   {};
}
#endif


/**
 * Base type for vector-like objects including Points and Vectors. It is
 * templated on the component datatype as well as the number of components that
 * make it up.
 *
 * @param DATA_TYPE  the datatype to use for the components
 * @param SIZE       the number of components this VecB has
 * @param REP        the representation to use for the vector.  (expression template or default)
 */
#ifndef GMTL_NO_METAPROG
template<class DATA_TYPE, unsigned SIZE, typename REP=meta::DefaultVecTag>
class VecBase
{
protected:
   const REP  expRep;      // The expression rep

public:
   /// The datatype used for the components of this VecB.
   typedef DATA_TYPE DataType;

   /// The number of components this VecB has.
   enum Params { Size = SIZE };

public:
   VecBase()
   {;}

   VecBase(const REP& rep)
      : expRep(rep)
   {;}

   /** Conversion operator to default vecbase type. */
   /*
   operator VecBase<DATA_TYPE,SIZE,meta::DefaultVecTag>()
   {
      return VecBase<DATA_TYPE,SIZE,meta::DefaultVecTag>(*this);
   }
   */

   /** Return the value at given location. */
   inline DATA_TYPE operator [](const unsigned i)
   {
      gmtlASSERT(i < SIZE);
      return expRep[i];
   }
   inline const DATA_TYPE  operator [](const unsigned i) const
   {
      gmtlASSERT(i < SIZE);
      return expRep[i];
   }
};
#endif


/**
 * Specialized version of VecBase that is actually used for all user interaction
 * with a traditional vector.
 *
 * @param DATA_TYPE  the datatype to use for the components
 * @param SIZE       the number of components this VecBase has
 */
template<class DATA_TYPE, unsigned SIZE>
#ifdef GMTL_NO_METAPROG
class VecBase
#else
class VecBase<DATA_TYPE,SIZE,meta::DefaultVecTag>
#endif
{
public:
   /// The datatype used for the components of this VecBase.
   typedef DATA_TYPE DataType;

#ifdef GMTL_NO_METAPROG
   typedef VecBase<DATA_TYPE, SIZE> VecType;
#else
   typedef VecBase<DATA_TYPE, SIZE, meta::DefaultVecTag> VecType;
#endif

   /// The number of components this VecBase has.
   enum Params { Size = SIZE };

public:
   /**
    * Default constructor.
    * Does nothing, leaves data alone.
    * This is for performance because this constructor is called by derived class constructors
    * Even when they just want to set the data directly
    */
   VecBase()
   {
#ifdef GMTL_COUNT_CONSTRUCT_CALLS
      gmtl::helpers::VecCtrCounterInstance()->inc();
#endif
   }

   /**
    * Makes an exact copy of the given VecBase object.
    *
    * @param rVec    the VecBase object to copy
    */
   VecBase(const VecBase<DATA_TYPE, SIZE>& rVec)
   {
#ifdef GMTL_COUNT_CONSTRUCT_CALLS
      gmtl::helpers::VecCtrCounterInstance()->inc();
#endif
#ifdef GMTL_NO_METAPROG
      for(unsigned i=0;i<SIZE;++i)
         mData[i] = rVec.mData[i];
#else
      gmtl::meta::AssignVecUnrolled<SIZE-1, VecBase<DATA_TYPE,SIZE> >::func(*this, rVec);
#endif
   }

#ifndef GMTL_NO_METAPROG
   template<typename REP2>
   VecBase(const VecBase<DATA_TYPE, SIZE, REP2>& rVec)
   {
#ifdef GMTL_COUNT_CONSTRUCT_CALLS
      gmtl::helpers::VecCtrCounterInstance()->inc();
#endif
      for(unsigned i=0;i<SIZE;++i)
      {  mData[i] = rVec[i]; }
   }
#endif

   //@{
   /**
    * Creates a new VecBase initialized to the given values.
    */
   VecBase(const DATA_TYPE& val0,const DATA_TYPE& val1)
   {
#ifdef GMTL_COUNT_CONSTRUCT_CALLS
      gmtl::helpers::VecCtrCounterInstance()->inc();
#endif
      GMTL_STATIC_ASSERT( SIZE == 2, Invalid_constructor_of_size_2_used);
      mData[0] = val0; mData[1] = val1;
   }
   VecBase(const DATA_TYPE& val0,const DATA_TYPE& val1,const DATA_TYPE& val2)
   {
#ifdef GMTL_COUNT_CONSTRUCT_CALLS
      gmtl::helpers::VecCtrCounterInstance()->inc();
#endif
      GMTL_STATIC_ASSERT( SIZE == 3, Invalid_constructor_of_size_3_used );
      mData[0] = val0;  mData[1] = val1;  mData[2] = val2;
   }
   VecBase(const DATA_TYPE& val0,const DATA_TYPE& val1,const DATA_TYPE& val2,const DATA_TYPE& val3)
   {
#ifdef GMTL_COUNT_CONSTRUCT_CALLS
      gmtl::helpers::VecCtrCounterInstance()->inc();
#endif
      // @todo need compile time assert
      GMTL_STATIC_ASSERT( SIZE == 4, Invalid_constructor_of_size_4_used);
      mData[0] = val0;  mData[1] = val1;  mData[2] = val2;  mData[3] = val3;
   }
   //@}

   /**
    * Sets the components in this VecBase using the given array.
    *
    * @param dataPtr    the array containing the values to copy
    * @pre dataPtr has at least SIZE elements
    */
   inline void set(const DATA_TYPE* dataPtr)
   {
#ifdef GMTL_NO_METAPROG
      for ( unsigned int i = 0; i < SIZE; ++i )
      {
         mData[i] = dataPtr[i];
      }
#else
      gmtl::meta::AssignArrayUnrolled<SIZE-1, DATA_TYPE>::func(&(mData[0]),
                                                               dataPtr);
#endif
   }

   //@{
   /**
    * Sets the components in this VecBase to the given values.
    */
   inline void set(const DATA_TYPE& val0)
   { mData[0] = val0; }

   inline void set(const DATA_TYPE& val0,const DATA_TYPE& val1)
   {
      GMTL_STATIC_ASSERT( SIZE >= 2, Set_out_of_valid_range);
      mData[0] = val0; mData[1] = val1;
   }
   inline void set(const DATA_TYPE& val0,const DATA_TYPE& val1,const DATA_TYPE& val2)
   {
      GMTL_STATIC_ASSERT( SIZE >= 3, Set_out_of_valid_range);
      mData[0] = val0;  mData[1] = val1;  mData[2] = val2;
   }
   inline void set(const DATA_TYPE& val0,const DATA_TYPE& val1,const DATA_TYPE& val2,const DATA_TYPE& val3)
   {
      GMTL_STATIC_ASSERT( SIZE >= 4, Set_out_of_valid_range);
      mData[0] = val0;  mData[1] = val1;  mData[2] = val2;  mData[3] = val3;
   }
   //@}

   //@{
   /**
    * Gets the ith component in this VecBase.
    *
    * @param i    the zero-based index of the component to access.
    * @pre i < SIZE
    *
    * @return  a reference to the ith component
    */
   inline DATA_TYPE& operator [](const unsigned i)
   {
      gmtlASSERT(i < SIZE);
      return mData[i];
   }
   inline const DATA_TYPE&  operator [](const unsigned i) const
   {
      gmtlASSERT(i < SIZE);
      return mData[i];
   }
   //@}

   /** Assign from different rep. */
#ifdef GMTL_NO_METAPROG
   inline VecType& operator=(const VecBase<DATA_TYPE,SIZE>& rhs)
   {
      for(unsigned i=0;i<SIZE;++i)
      {
         mData[i] = rhs[i];
      }

      return *this;
   }
#else
   template<typename REP2>
   inline VecType& operator=(const VecBase<DATA_TYPE,SIZE,REP2>& rhs)
   {
      for(unsigned i=0;i<SIZE;++i)
      {
         mData[i] = rhs[i];
      }

      //gmtl::meta::AssignVecUnrolled<SIZE-1, VecBase<DATA_TYPE,SIZE> >::func(*this, rVec);
      return *this;
   }
#endif

   /*
    Assign from another of same type.
   inline VecType& operator=(const VecType&  rhs)
   {
      for(unsigned i=0;i<SIZE;++i)
      {
         mData[i] = rhs[i];
      }
      return *this;
   }
*/

   //@{
   /**
    * Gets the internal array of the components.
    *
    * @return  a pointer to the component array with length SIZE
    */
   DATA_TYPE* getData()
   { return mData; }
   const DATA_TYPE* getData() const
   { return mData; }
   //@}

public:
   /// The array of components.
   DATA_TYPE mData[SIZE];
};


}

#endif
