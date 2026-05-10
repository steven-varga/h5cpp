// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_AABOX_H_
#define _GMTL_AABOX_H_

#include <gmtl/Point.h>

namespace gmtl
{
   /**
    * Describes an axially aligned box in 3D space. This is usually used for
    * graphics applications. It is defined by its minimum and maximum points.
    *
    * @param DATA_TYPE     the internal type used for the points
    *
    * @ingroup Types
    */
   template< class DATA_TYPE >
   class AABox
   {
      // This is a hack to work around a bug with GCC 3.3 on Mac OS X
      // where boost::is_polymorphic returns a false positive.  The details
      // can be found in the Boost.Python FAQ:
      //    http://www.boost.org/libs/python/doc/v2/faq.html#macosx
#if defined(__MACH__) && defined(__APPLE_CC__) && defined(__GNUC__) && \
    __GNUC__ == 3 && __GNUC_MINOR__ == 3
      bool dummy_;
#endif
   public:
      typedef DATA_TYPE DataType;

   public:
      /**
       * Creates a new empty box.
       */
      AABox()
         : mMin(0,0,0), mMax(0,0,0), mEmpty(true)
      {}

      /**
       * Creates a new box with the given min and max points.
       *
       * @param min     the minimum point on the box
       * @param max     the maximum point on the box
       *
       * @pre  all elements of min are less than max
       * @pre  bot min and max are not zero
       */
      AABox(const Point<DATA_TYPE, 3>& min, const Point<DATA_TYPE, 3>& max)
         : mMin(min), mMax(max), mEmpty(false)
      {}

      /**
       * Construcst a duplicate of the given box.
       *
       * @param box     the box the make a copy of
       */
      AABox(const AABox<DATA_TYPE>& box)
         : mMin(box.mMin), mMax(box.mMax), mEmpty(box.mEmpty)
      {}

      /**
       * Gets the minimum point of the box.
       *
       * @return  the min point
       */
      const Point<DATA_TYPE, 3>& getMin() const
      {
         return mMin;
      }

      /**
       * Gets the maximum point of the box.
       *
       * @return  the max point
       */
      const Point<DATA_TYPE, 3>& getMax() const
      {
         return mMax;
      }

      /**
       * Tests if this box occupies no space.
       *
       * @return  true if the box is empty, false otherwise
       */
      bool isEmpty() const
      {
         return mEmpty;
      }

      /**
       * Sets the minimum point of the box.
       *
       * @param min     the min point
       */
      void setMin(const Point<DATA_TYPE, 3>& min)
      {
         mMin = min;
      }

      /**
       * Sets the maximum point of the box.
       *
       * @param max     the max point
       */
      void setMax(const Point<DATA_TYPE, 3>& max)
      {
         mMax = max;
      }

      /**
       * Sets the empty flag on this box.
       *
       * @param empty   true to make the box empty, false otherwise
       */
      void setEmpty(bool empty)
      {
         mEmpty = empty;
      }

   public:
      /**
       * The minimum point of the box.
       */
      Point<DATA_TYPE, 3> mMin;

      /**
       * The maximum point on the box.
       */
      Point<DATA_TYPE, 3> mMax;

      /**
       * Flag for empty box. True if the box is empty.
       */
      bool mEmpty;
   };

   // --- helper types --- //
   typedef AABox<float>    AABoxf;
   typedef AABox<double>   AABoxd;
}

#endif
