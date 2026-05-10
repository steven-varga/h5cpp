// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_PLANE_H
#define _GMTL_PLANE_H

#include <gmtl/Vec.h>
#include <gmtl/Point.h>
#include <gmtl/VecOps.h>

namespace gmtl
{

/**
 *  Plane: Defines a geometrical plane.
 *
 *  All points on the plane satify the equation dot(Pt,Normal) = offset
 *  normal is assumed to be normalized
 *
 *  NOTE: Some plane implementation store D instead of offset.  Thus
 *       those implementation have opposite sign from what we have
 *
 * pg. 309 Computer Graphics 2nd Edition Hearn Baker
 *<pre>
 *  N dot P = -D
 *   |
 *   |-d-|
 * __|___|-->N
 *   |   |
 *</pre>
 * @ingroup Types
 */
template< class DATA_TYPE>
class Plane
{
public:
   /**
    * Creates an uninitialized Plane. In other words, the normal is (0,0,0) and
    * the offset is 0.
    */
   Plane()
      : mOffset( 0 )
   {}

   /**
    * Creates a plane that the given points lie on.
    *
    * @param pt1     a point on the plane
    * @param pt2     a point on the plane
    * @param pt3     a point on the plane
    */
   Plane( const Point<DATA_TYPE, 3>& pt1, const Point<DATA_TYPE, 3>& pt2,
          const Point<DATA_TYPE, 3>& pt3)
   {
      Vec<DATA_TYPE, 3> vec12( pt2-pt1 );
      Vec<DATA_TYPE, 3> vec13( pt3-pt1 );

      cross( mNorm, vec12, vec13 );
      normalize( mNorm );

      mOffset = dot( static_cast< Vec<DATA_TYPE, 3> >(pt1), mNorm );  // Graphics Gems I: Page 390
   }

   /**
    * Creates a plane with the given normal on which pt resides.
    *
    * @param norm    the normal of the plane
    * @param pt      a point that lies on the plane
    */
   Plane( const Vec<DATA_TYPE, 3>& norm, const Point<DATA_TYPE, 3>& pt )
      : mNorm( norm )
   {
      mOffset = dot( static_cast< Vec<DATA_TYPE, 3> >(pt), norm );
   }

   /**
    * Creates a plane with the given normal and offset.
    *
    * @param norm          the normal of the plane
    * @param dPlaneConst   the plane offset constant
    */
   Plane( const Vec<DATA_TYPE, 3>& norm, const DATA_TYPE& dPlaneConst )
      : mNorm( norm ), mOffset( dPlaneConst )
   {}

   /**
    * Creates an exact duplicate of the given plane.
    *
    * @param plane   the plane to copy
    */
   Plane( const Plane<DATA_TYPE>& plane )
      : mNorm( plane.mNorm ), mOffset( plane.mOffset )
   {}

   /**
    * Gets the normal for this plane.
    *
    * @return  this plane's normal vector
    */
   const Vec<DATA_TYPE, 3>& getNormal() const
   {
      return mNorm;
   }

   /**
    * Sets the normal for this plane to the given vector.
    *
    * @param norm    the new normalized vector
    *
    * @pre |norm| = 1
    */
   void setNormal( const Vec<DATA_TYPE, 3>& norm )
   {
      mNorm = norm;
   }

   /**
    * Gets the offset of this plane from the origin such that the offset is the
    * negative distance from the origin.
    *
    * @return  this plane's offset
    */
   const DATA_TYPE& getOffset() const
   {
      return mOffset;
   }

   /**
    * Sets the offset of this plane from the origin.
    *
    * @param offset     the new offset
    */
   void setOffset( const DATA_TYPE& offset )
   {
      mOffset = offset;
   }

public:
   // dot(Pt,mNorm) = mOffset
   /**
    * The normal for this vector. For any point on the plane,
    * dot( pt, mNorm) = mOffset.
    */
   Vec<DATA_TYPE, 3> mNorm;

   /**
    * This plane's offset from the origin such that for any point pt,
    * dot( pt, mNorm ) = mOffset. Note that mOffset = -D (neg dist from the
    * origin).
    */
   DATA_TYPE mOffset;
};

typedef Plane<float> Planef;
typedef Plane<double> Planed;

/*
#include <geomdist.h>


// Intersects the plane with a given segment.
// Returns TRUE if there is a hit (within the seg).
// Also returns the distance "down" the segment of the hit in t.
//
// PRE: seg.dir must be normalized
//
int sgPlane::isect(sgSeg& seg, float* t)
{
	// Graphic Gems I: Page 391
	float denom = normal.dot(seg.dir);
	if (SG_IS_ZERO(denom))		// No Hit
	{
		//: So now, it is just dist to plane tested against length
		sgVec3	hit_pt;
		float		hit_dist;		// Dist to hit
		
		hit_dist = findNearestPt(seg.pos, hit_pt);
		*t = hit_dist;			// Since dir is normalized

		if(seg.tValueOnSeg(*t))
			return 1;
		else
			return 0;
	}
	else
	{
		float numer = offset + normal.dot(seg.pos);
		(*t) = -1.0f * (numer/denom);
		
		if(seg.tValueOnSeg(*t))
			return 1;
		else
			return 0;
	}
}

///
 // Intersects the plane with the line defined
 // by the given segment
 //
 // seg - seg that represents the line to isect
 // t   - the t value of the isect
 //
int sgPlane::isectLine(const sgSeg& seg, float* t)
{
	// GGI: Pg 299
	// Lu = seg.pos;
	// Lv = seg.dir;
	// Jn = normal;
	// Jd = offset;
	
	float denom = normal.dot(seg.dir);
	if(denom == 0.0f)
      return 0;
	else
	{
      *t = - (offset+ normal.dot(seg.pos))/(denom);
      return 1;
	}
}
*/

} // namespace gmtl
#endif
