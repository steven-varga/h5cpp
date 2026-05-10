// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_LINESEG_H_
#define _GMTL_LINESEG_H_

#include <gmtl/Point.h>
#include <gmtl/Vec.h>
#include <gmtl/VecOps.h>
#include <gmtl/Ray.h>

namespace gmtl {

/**
 * Describes a line segment. This is represented by a point origin O and a
 * vector spanning the length of the line segement originating at O. Thus any
 * point on the line segment can be described as
 *
 * P(s) = O + Vs
 *
 * where 0 <= s <= 1
 *
 * @param DATA_TYPE     the internal type used for the point and vector
 */
template <typename DATA_TYPE>
class LineSeg : public Ray<DATA_TYPE>
{
public:
   /**
    * Constructs a line segment at the origin with a zero vector.
    */
   LineSeg()
   {}

   /**
    * Constructs a line segment with the given origin and vector.
    *
    * @param origin     the point at which the line segment starts
    * @param dir        the vector describing the direction and length of the
    *                   line segment starting at origin
    */
   LineSeg( const Point<DATA_TYPE, 3>& origin, const Vec<DATA_TYPE, 3>& dir )
      : Ray<DATA_TYPE>( origin, dir )
   {}

   /**
    * Constructs an exact duplicate of the given line segment.
    *
    * @param ray    the line segment to copy
    */
   LineSeg( const LineSeg& ray ) : Ray<DATA_TYPE>( ray )
   {
   }

   /**
    * Constructs a line segment with the given beginning and ending points.
    *
    * @param beg     the point at the beginning of the line segment
    * @param end     the point at the end of the line segment
    */
   LineSeg( const Point<DATA_TYPE, 3>& beg, const Point<DATA_TYPE, 3>& end )
      : Ray<DATA_TYPE>()
   {
      this->mOrigin = beg;
      this->mDir = end - beg;
   }

   /**
    * Gets the length of this line segment.
    * @return the length of the line segment
    */
   DATA_TYPE getLength() const
   {
      return length(this->mDir);
   }
};


// --- helper types --- //
typedef LineSeg<float>  LineSegf;
typedef LineSeg<double> LineSegd;
}

#endif
