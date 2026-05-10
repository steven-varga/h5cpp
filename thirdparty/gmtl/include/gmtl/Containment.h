// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_CONTAINMENT_H_
#define _GMTL_CONTAINMENT_H_

// new stuff
#include <vector>
#include <gmtl/Sphere.h>
#include <gmtl/AABox.h>
#include <gmtl/Frustum.h>
#include <gmtl/Tri.h>
#include <gmtl/VecOps.h>

// old stuff
//#include <gmtl/OOBox.h>
//#include <gmtl/AABox.h>
//#include <gmtl/Fit/GaussPointsFit.h>
//#include <gmtl/matVecFuncs.h>
//#include <gmtl/Quat.h>

namespace gmtl
{

//-----------------------------------------------------------------------------
// Sphere
//-----------------------------------------------------------------------------

/**
 * Tests if the given point is inside or on the surface of the given spherical
 * volume.
 *
 * @param container  the sphere to test against
 * @param pt         the point to test with
 *
 * @return  true if pt is inside container, false otherwise
 */
template< class DATA_TYPE >
bool isInVolume( const Sphere<DATA_TYPE>& container,
                 const Point<DATA_TYPE, 3>& pt )
{
   // The point is inside the sphere if the vector computed from the center of
   // the sphere to the point has a magnitude less than or equal to the radius
   // of the sphere.
   // |pt - center| <= radius
   return ( length(gmtl::Vec<DATA_TYPE,3>(pt - container.mCenter)) <= container.mRadius );
}

/**
 * Tests if the given sphere is completely inside or on the surface of the given
 * spherical volume.
 *
 * @param container  the sphere acting as the container
 * @param sphere     the sphere that may be inside container
 *
 * @return  true if sphere is inside container, false otherwise
 */
template< class DATA_TYPE >
bool isInVolume( const Sphere<DATA_TYPE>& container,
                 const Sphere<DATA_TYPE>& sphere )
{
   // the sphere is inside container if the distance between the centers of the
   // spheres plus the radius of the inner sphere is less than or equal to the
   // radius of the containing sphere.
   // |sphere.center - container.center| + sphere.radius <= container.radius
   return ( length(gmtl::Vec<DATA_TYPE,3>(sphere.mCenter - container.mCenter)) + sphere.mRadius
            <= container.mRadius );
}

/**
 * Modifies the existing sphere to tightly enclose itself and the given point.
 *
 * @param container  [in,out] the sphere that will be extended
 * @param pt         [in]     the point which the sphere should contain
 */
template< class DATA_TYPE >
void extendVolume( Sphere<DATA_TYPE>& container,
                   const Point<DATA_TYPE, 3>& pt )
{
   // check if we already contain the point
   if ( isInVolume( container, pt ) )
   {
      return;
   }

   // make a vector pointing from the center of the sphere to pt. this is the
   // direction in which we need to move the sphere's center
   Vec<DATA_TYPE, 3> dir = pt - container.mCenter;
   DATA_TYPE len = normalize( dir );

   // compute what the new radius should be
   DATA_TYPE newRadius = (len + container.mRadius) * static_cast<DATA_TYPE>(0.5);

   // compute the new center for the sphere
   Point<DATA_TYPE, 3> newCenter = container.mCenter +
                                   (dir * (newRadius - container.mRadius));

   // modify container to its new values
   container.mCenter = newCenter;
   container.mRadius = newRadius;
}

/**
 * Modifies the container to tightly enclose itself and the given sphere.
 *
 * @param container  [in,out] the sphere that will be extended
 * @param sphere     [in]     the sphere which container should contain
 */
template< class DATA_TYPE >
void extendVolume( Sphere<DATA_TYPE>& container,
                   const Sphere<DATA_TYPE>& sphere )
{
   // check if we already contain the sphere
   if ( isInVolume( container, sphere ) )
   {
      return;
   }

   // make a vector pointing from the center of container to sphere. this is the
   // direction in which we need to move container's center
   Vec<DATA_TYPE, 3> dir = sphere.mCenter - container.mCenter;
   DATA_TYPE len = normalize( dir );

   // compute what the new radius should be
   DATA_TYPE newRadius = (len + sphere.mRadius + container.mRadius) *
                         static_cast<DATA_TYPE>(0.5);

   // compute the new center for container
   Point<DATA_TYPE, 3> newCenter = container.mCenter +
                                   (dir * (newRadius - container.mRadius));

   // modify container to its new values
   container.mCenter = newCenter;
   container.mRadius = newRadius;
}

/**
 * Modifies the given sphere to tightly enclose all points in the given
 * std::vector. This operation is O(n) and uses sqrt(..) liberally. :(
 *
 * @param container  [out]    the sphere that will be modified to tightly
 *                            enclose all the points in pts
 * @param pts        [in]     the list of points to contain
 *
 * @pre  pts must contain at least 2 points
 */
template< class DATA_TYPE >
void makeVolume( Sphere<DATA_TYPE>& container,
                 const std::vector< Point<DATA_TYPE, 3> >& pts )
{
   gmtlASSERT( pts.size() > 0  && "pts must contain at least 1 point" );

   // Implementation based on the Sphere Centered at Average of Points algorithm
   // found in "3D Game Engine Design" by Devud G, Eberly (pg. 27)
   typename std::vector< Point<DATA_TYPE, 3> >::const_iterator itr = pts.begin();

   // compute the average of the points as the center
   Point<DATA_TYPE, 3> sum = *itr;
   ++itr;
   while ( itr != pts.end() )
   {
      sum += *itr;
      ++itr;
   }
   container.mCenter = sum / static_cast<DATA_TYPE>(pts.size());

   // compute the distance from the computed center to point furthest from that
   // center as the radius
   DATA_TYPE radiusSqr(0);
   for ( itr = pts.begin(); itr != pts.end(); ++itr )
   {
      DATA_TYPE len = lengthSquared( gmtl::Vec<DATA_TYPE,3>( (*itr) - container.mCenter) );
      if ( len > radiusSqr )
         radiusSqr = len;
   }

   container.mRadius = Math::sqrt( radiusSqr );
}

/*
template< class DATA_TYPE >
void makeVolume( Sphere<DATA_TYPE>& container,
                 const std::vector< Point<DATA_TYPE, 3> >& pts )
{
   gmtlASSERT( pts.size() > 1  && "pts must contain at least 2 points" );

   // make a sphere around the first 2 points and then extend the sphere by each
   // point thereafter. we could probably be smarter about this ...
   std::vector< Point<DATA_TYPE, 3> >::const_iterator itr = pts.begin();

   // make the initial sphere
   const Point<DATA_TYPE, 3>& first = *itr;
   ++itr;
   const Point<DATA_TYPE, 3>& second = *itr;
   ++itr;
   const Vec<DATA_TYPE, 3> dir = second - first;
   container.mRadius = length(dir) * static_cast<DATA_TYPE>(0.5);
   container.mCenter = first + (dir * container.mRadius);

   // iterate through the remaining points and extend the container to fit each
   // point. yay code reuse!
   while ( itr != pts.end() )
   {
      extendVolume( container, *itr );
      ++itr;
   }
}
*/
/**
 * Modifies the given sphere to tightly enclose all spheres in the given
 * std::vector. This operation is O(n) and uses sqrt(..) liberally. :(
 *
 * @param container  [out]    the sphere that will be modified to tightly
 *                            enclose all the spheres in spheres
 * @param spheres    [in]     the list of spheres to contain
 *
 * @pre  spheres must contain at least 2 points
 */
/*
template< class DATA_TYPE >
void makeVolume( Sphere<DATA_TYPE>& container,
                 const std::vector< Sphere<DATA_TYPE> >& spheres )
{
   gmtlASSERT( spheres.size() > 1  && "spheres must contain at least 2 points" );

   // make a sphere around the first 2 points and then extend the sphere by each
   // point thereafter. we could probably be smarter about this ...
   std::vector< Sphere<DATA_TYPE> >::const_iterator itr = spheres.begin();

   // make the initial sphere
   const Sphere<DATA_TYPE>& first = *itr;
   ++itr;
   const Sphere<DATA_TYPE>& second = *itr;
   ++itr;
   const Vec<DATA_TYPE, 3> dir = second.mCenter - first.mCenter;
   container.mRadius = (length(dir) + first.mRadius + second.mRadius) *
                       static_cast<DATA_TYPE>(0.5);
   container.mCenter = first.mCenter +
                       (dir * (container.mRadius - first.mRadius));

   // iterate through the remaining points and extend the container to fit each
   // point. yay code reuse!
   while ( itr != spheres.end() )
   {
      extendVolume( container, *itr );
      ++itr;
   }
}
*/
/**
 * Tests if the given point is on the surface of the container with zero
 * tolerance.
 *
 * @param container     the container to test against
 * @param pt            the test point
 *
 * @return  true if pt is on the surface of container, false otherwise
 */
template< class DATA_TYPE >
bool isOnVolume( const Sphere<DATA_TYPE>& container,
                 const Point<DATA_TYPE, 3>& pt )
{
   // |center - pt| - radius == 0
   return ( length(gmtl::Vec<DATA_TYPE,3>(container.mCenter - pt)) - container.mRadius == 0 );
}

/**
 * Tests of the given point is on the surface of the container with the given
 * tolerance.
 *
 * @param container     the container to test against
 * @param pt            the test point
 * @param tol           the epsilon tolerance
 *
 * @return  true if pt is on the surface of container, false otherwise
 */
template< class DATA_TYPE >
bool isOnVolume( const Sphere<DATA_TYPE>& container,
                 const Point<DATA_TYPE, 3>& pt,
                 const DATA_TYPE& tol )
{
   gmtlASSERT( tol >= 0 && "tolerance must be positive" );

   // abs( |center-pt| - radius ) < tol
   return ( Math::abs( length( gmtl::Vec<DATA_TYPE,3>(container.mCenter - pt)) - container.mRadius )
            <= tol );
}

//-----------------------------------------------------------------------------
// AABox
//-----------------------------------------------------------------------------

/**
 * Tests if the given point is inside (or on) the surface of the given AABox
 * volume.
 *
 * @param container  the AABox to test against
 * @param pt         the point to test with
 *
 * @return  true if pt is inside container, false otherwise
 */
template< class DATA_TYPE>
bool isInVolume(const AABox<DATA_TYPE>& container,
                const Point<DATA_TYPE, 3>& pt)
{
   if (! container.isEmpty())
   {
      return ( pt[0] >= container.mMin[0] &&
               pt[1] >= container.mMin[1] &&
               pt[2] >= container.mMin[2] &&
               pt[0] <= container.mMax[0] &&
               pt[1] <= container.mMax[1] &&
               pt[2] <= container.mMax[2]);
   }
   else
   {
      return false;
   }
}

/**
 * Tests if the given point is inside (not on) the surface of the given AABox
 * volume.  This method is "exclusive" because it does not consider the surface
 * to be a part of the space.
 *
 * @param container  the AABox to test against
 * @param pt         the point to test with
 *
 * @return  true if pt is inside container (but not on surface), false otherwise
 */
template< class DATA_TYPE>
bool isInVolumeExclusive(const AABox<DATA_TYPE>& container,
                const Point<DATA_TYPE, 3>& pt)
{
   if (! container.isEmpty())
   {
      return ( pt[0] > container.mMin[0] &&
               pt[1] > container.mMin[1] &&
               pt[2] > container.mMin[2] &&
               pt[0] < container.mMax[0] &&
               pt[1] < container.mMax[1] &&
               pt[2] < container.mMax[2]);
   }
   else
   {
      return false;
   }
}




/**
 * Tests if the given AABox is completely inside or on the surface of the given
 * AABox container.
 *
 * @param container  the AABox acting as the container
 * @param box        the AABox that may be inside container
 *
 * @return  true if AABox is inside container, false otherwise
 */
template< class DATA_TYPE >
bool isInVolume(const AABox<DATA_TYPE>& container,
                const AABox<DATA_TYPE>& box)
{
   // Empty boxes don't overlap
   if (container.isEmpty() || box.isEmpty())
   {
      return false;
   }
 
   if (container.mMin[0] <= box.mMin[0] && container.mMax[0] >= box.mMax[0] &&
       container.mMin[1] <= box.mMin[1] && container.mMax[1] >= box.mMax[1] &&
       container.mMin[2] <= box.mMin[2] && container.mMax[2] >= box.mMax[2])
   {
      return true;
   }
   else
   {
      return false;
   }
}

/**
 * Modifies the existing AABox to tightly enclose itself and the given point.
 *
 * @param container  [in,out] the AABox that will be extended
 * @param pt         [in]     the point which the AABox should contain
 */
template< class DATA_TYPE >
void extendVolume(AABox<DATA_TYPE>& container,
                  const Point<DATA_TYPE, 3>& pt)
{
   if (! container.isEmpty())
   {
      // X coord
      if (pt[0] > container.mMax[0])
      {
         container.mMax[0] = pt[0];
      }
      else if (pt[0] < container.mMin[0])
      {
         container.mMin[0] = pt[0];
      }

      // Y coord
      if (pt[1] > container.mMax[1])
      {
         container.mMax[1] = pt[1];
      }
      else if (pt[1] < container.mMin[1])
      {
         container.mMin[1] = pt[1];
      }

      // Z coord
      if (pt[2] > container.mMax[2])
      {
         container.mMax[2] = pt[2];
      }
      else if (pt[2] < container.mMin[2])
      {
         container.mMin[2] = pt[2];
      }
   }
   else
   {
      // Make a box with essentially zero volume at the point
      container.setMin(pt);
      container.setMax(pt);
      container.setEmpty(false);
   }
}

/**
 * Modifies the container to tightly enclose itself and the given AABox.
 *
 * @param container  [in,out] the AABox that will be extended
 * @param box        [in]     the AABox which container should contain
 */
template< class DATA_TYPE >
void extendVolume(AABox<DATA_TYPE>& container,
                  const AABox<DATA_TYPE>& box)
{
   // Can't extend by an empty box
   if (box.isEmpty())
   {
      return;
   }

   // An empty container is extended to be the box
   if (container.isEmpty())
   {
      container = box;
   }

   // Just extend by the corners of the box
   extendVolume(container, box.getMin());
   extendVolume(container, box.getMax());
}

/**
 * Creates an AABox that tightly encloses the given Sphere.
 *
 * @param box     set to the box
 */
template< class DATA_TYPE >
void makeVolume(AABox<DATA_TYPE>& box, const Sphere<DATA_TYPE>& sph)
{
   const gmtl::Point<DATA_TYPE, 3>& center = sph.getCenter();
   const DATA_TYPE& radius = sph.getRadius();

   // Calculate the min and max points for the box
   gmtl::Point<DATA_TYPE, 3> min_pt(center[0] - radius,
                                    center[1] - radius,
                                    center[2] - radius);
   gmtl::Point<DATA_TYPE, 3> max_pt(center[0] + radius,
                                    center[1] + radius,
                                    center[2] + radius);

   box.setMin(min_pt);
   box.setMax(max_pt);
   box.setEmpty(radius == DATA_TYPE(0));
}

//-----------------------------------------------------------------------------
// Frustum
//-----------------------------------------------------------------------------

const unsigned int IN_FRONT_OF_ALL_PLANES = 6;

template<typename T>
inline bool isInVolume(const Frustum<T>& f, const Point<T, 3>& p,
                       unsigned int& idx /*out*/)
{
   for ( unsigned int i = 0; i < 6; ++i )
   {
      T dist = dot(f.mPlanes[i].mNorm, static_cast< Vec<T, 3> >(p)) + f.mPlanes[i].mOffset;
      if (dist < T(0.0) )
      {
         idx = i;
         return false;
      }
   }
     
   idx = IN_FRONT_OF_ALL_PLANES;
   return true;
}

template<typename T>
inline bool isInVolume(const Frustum<T>& f, const Sphere<T>& s)
{
   for ( unsigned int i = 0; i < 6; ++i )
   {
      T dist = dot(f.mPlanes[i].mNorm, static_cast< Vec<T, 3> >(s.getCenter())) + f.mPlanes[i].mOffset;
      if ( dist <= -T(s.getRadius()) )
      {
         return false;
      }
   }

   return true;
}

template<typename T>
inline bool isInVolume(const Frustum<T>& f, const AABox<T>& box)
{
   const Point<T, 3>& min = box.getMin();
   const Point<T, 3>& max = box.getMax();
   Point<T, 3> p[8];
   p[0] = min;
   p[1] = max;
   p[2] = Point<T, 3>(max[0], min[1], min[2]);
   p[3] = Point<T, 3>(min[0], max[1], min[2]);
   p[4] = Point<T, 3>(min[0], min[1], max[2]);
   p[5] = Point<T, 3>(max[0], max[1], min[2]);
   p[6] = Point<T, 3>(min[0], max[1], max[2]);
   p[7] = Point<T, 3>(max[0], min[1], max[2]);

   unsigned int idx = 6;

   if ( isInVolume(f, p[0], idx) )
   {
      return true;
   }

   // now we have the index of the seperating plane int idx, so check if all
   // other points lie on the backside of this plane too

   for ( unsigned int i = 1; i < 8; ++i )
   {
      T dist = dot(f.mPlanes[idx].mNorm, static_cast< Vec<T, 3> >(p[i])) + f.mPlanes[idx].mOffset;      
      if ( dist > T(0.0) )
      {
         return true;
      }
   }
      
   return false;
}

template<typename T>
inline bool isInVolume(const Frustum<T>& f, const Tri<T>& tri)
{
   unsigned int junk;

   if ( isInVolume(f, tri[0], junk) )
   {
      return true;
   }

   if ( isInVolume(f, tri[1], junk) )
   {
      return true;
   }

   if ( isInVolume(f, tri[2], junk) )
   {
      return true;
   }

   return false;
}

/*
//------------------------------------------------------------------------------
// Begin old GMTL code
//------------------------------------------------------------------------------
inline void computeContainment( AABox& box, const std::vector<gmtl::Point3>& points,
                         Point3& minPt, Point3& maxPt )
//void MgcContAlignedBox (int iQuantity, const MgcVector3* akPoint,
//    MgcVector3& rkMin, MgcVector3& rkMax)
{
   if (points.empty())
      return;

   minPt = points[0];
   maxPt = minPt;

   for (unsigned i = 1; i < points.size(); i++)
   {
      if ( points[i][Xelt] < minPt[Xelt] )
         minPt[Xelt] = points[i][Xelt];
      else if ( points[i][Xelt] > maxPt[Xelt] )
         maxPt[Xelt] = points[i][Xelt];

      if ( points[i][Yelt] < minPt[Yelt] )
         minPt[Yelt] = points[i][Yelt];
      else if ( points[i][Yelt] > maxPt[Yelt] )
         maxPt[Yelt] = points[i][Yelt];

      if ( points[i][Zelt] < minPt[Zelt] )
         minPt[Zelt] = points[i][Zelt];
      else if ( points[i][Zelt] > maxPt[Zelt] )
         maxPt[Zelt] = points[i][Zelt];
   }

   // Now update the box
   box.makeEmpty();
   box.mMax = maxPt;
   box.mMin = minPt;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//MgcBox3 MgcContOrientedBox (int iQuantity, const MgcVector3* akPoint)
inline void computeContainment( OOBox& box, const std::vector<gmtl::Point3>& points)
{
    //MgcGaussPointsFit(iQuantity,akPoint,kBox.Center(),kBox.Axes(),
    //    kBox.Extents());

    gmtl::GaussPointsFit(points.size(), &points[0], box.center(), box.axes(), box.halfLens());

    // Let C be the box center and let U0, U1, and U2 be the box axes.  Each
    // input point is of the form X = C + y0*U0 + y1*U1 + y2*U2.  The
    // following code computes min(y0), max(y0), min(y1), max(y1), min(y2),
    // and max(y2).  The box center is then adjusted to be
    //   C' = C + 0.5*(min(y0)+max(y0))*U0 + 0.5*(min(y1)+max(y1))*U1 +
    //        0.5*(min(y2)+max(y2))*U2
#ifdef _DEBUG
   gmtl::OOBox box_test;
   box_test = box;
   gmtl::Vec3 ax0 = box_test.axis(0);
   gmtl::Vec3 ax1 = box_test.axis(1);
   gmtl::Vec3 ax2 = box_test.axis(2);
#endif

    gmtlASSERT(box.axis(0).isNormalized());
    gmtlASSERT(box.axis(1).isNormalized());
    gmtlASSERT(box.axis(2).isNormalized());

    // XXX: Sign is sometimes wrong here
    // This is hack code to make it "work right"
    Vec3 cross;
    cross = box.axis(0).cross(box.axis(1));
    cross.normalize();
    box.axis(2) = cross;

    Vec3 kDiff = points[0] - box.center();
    float fY0Min = kDiff.dot(box.axis(0)), fY0Max = fY0Min;
    float fY1Min = kDiff.dot(box.axis(1)), fY1Max = fY1Min;
    float fY2Min = kDiff.dot(box.axis(2)), fY2Max = fY2Min;

    float fY0, fY1, fY2;

    for (unsigned i = 1; i < points.size(); i++)
    {
        kDiff = points[i] - box.center();

        fY0 = kDiff.dot(box.axis(0));
        if ( fY0 < fY0Min )
            fY0Min = fY0;
        else if ( fY0 > fY0Max )
            fY0Max = fY0;

        fY1 = kDiff.dot(box.axis(1));
        if ( fY1 < fY1Min )
            fY1Min = fY1;
        else if ( fY1 > fY1Max )
            fY1Max = fY1;

        fY2 = kDiff.dot(box.axis(2));
        if ( fY2 < fY2Min )
            fY2Min = fY2;
        else if ( fY2 > fY2Max )
            fY2Max = fY2;
    }

    box.center() += (0.5*(fY0Min+fY0Max))*box.axis(0) +
                    (0.5*(fY1Min+fY1Max))*box.axis(1) +
                    (0.5*(fY2Min+fY2Max))*box.axis(2);

    box.halfLen(0) = 0.5*(fY0Max - fY0Min);
    box.halfLen(1) = 0.5*(fY1Max - fY1Min);
    box.halfLen(2) = 0.5*(fY2Max - fY2Min);
}

*/
/*
inline void computeContainment (OOBox& out_box, const OOBox& box0, const OOBox& box1, bool fast=true)
{
   gmtl::OOBox ret_box;    // The resulting box

   if (fast)
   {
      ret_box.center() = 0.5*(box0.center() + box1.center());       // Center at average

      // Average the quats to get a new orientation
      Quat quat0, quat1;
      quat0.makeAxes(box0.axis(0), box0.axis(1), box0.axis(2));
      quat1.makeAxes(box1.axis(0), box1.axis(1), box1.axis(2));
      if ( quat0.dot(quat1) < 0.0 )
         quat1 = -quat1;

      Quat full_quat = quat0 + quat1;

      //float inv_len = 1.0/Math::sqrt(full_quat.norm());
      //full_quat = inv_len*full_quat;
      full_quat.normalize();
      full_quat.getAxes(ret_box.axis(0), ret_box.axis(1), ret_box.axis(2));

      // Now that we have new orientation, extend half lens to cover the volume
      //
      unsigned i, j;
      Point3 verts[8];
      Vec3 diff;
      float aDot;

      ret_box.halfLen(0) = 0.0;
      ret_box.halfLen(1) = 0.0;
      ret_box.halfLen(2) = 0.0;

      box0.getVerts(verts);
      for (i = 0; i < 8; i++)
      {
         diff = verts[i] - ret_box.center();
         for (j = 0; j < 3; j++)                       // For each axis of box0
         {
            aDot = Math::abs(diff.dot(ret_box.axis(j)));
            if ( aDot > ret_box.halfLen(j) )
               ret_box.halfLen(j) = aDot;
         }
      }

      box1.getVerts(verts);
      for (i = 0; i < 8; i++)
      {
         diff = verts[i] - ret_box.center();
         for (j = 0; j < 3; j++)
         {
            aDot = Math::abs(diff.dot(ret_box.axis(j)));
            if ( aDot > ret_box.halfLen(j) )
               ret_box.halfLen(j) = aDot;
         }
      }
   }
   else     // Tighter fit
   {
      // Hack that will do it correctly, but is slow
      Point3 verts[8];
      std::vector<gmtl::Point3> vert_points;

      box0.getVerts(verts);
      for (unsigned i=0;i<8;i++) vert_points.push_back(verts[i]);
      box1.getVerts(verts);
      for (unsigned i=0;i<8;i++) vert_points.push_back(verts[i]);

      computeContainment(ret_box, vert_points);
   }

   out_box = ret_box;
}
*/


}

#endif
