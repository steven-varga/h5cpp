// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_INTERSECTION_H_
#define _GMTL_INTERSECTION_H_

#include <algorithm>
#include <limits>
#include <gmtl/AABox.h>
#include <gmtl/Point.h>
#include <gmtl/Sphere.h>
#include <gmtl/Vec.h>
#include <gmtl/Plane.h>
#include <gmtl/VecOps.h>
#include <gmtl/Math.h>
#include <gmtl/Ray.h>
#include <gmtl/LineSeg.h>
#include <gmtl/Tri.h>
#include <gmtl/PlaneOps.h>

namespace gmtl
{
   /**
    * Tests if the given AABoxes intersect with each other. Sharing an edge IS
    * considered intersection by this algorithm.
    *
    * @param box1    the first AA box to test
    * @param box2    the second AA box to test
    *
    * @return  true if the boxes intersect; false otherwise
    */
   template<class DATA_TYPE>
   bool intersect(const AABox<DATA_TYPE>& box1, const AABox<DATA_TYPE>& box2)
   {
      // Look for a separating axis on each box for each axis
      if (box1.getMin()[0] > box2.getMax()[0])  return false;
      if (box1.getMin()[1] > box2.getMax()[1])  return false;
      if (box1.getMin()[2] > box2.getMax()[2])  return false;

      if (box2.getMin()[0] > box1.getMax()[0])  return false;
      if (box2.getMin()[1] > box1.getMax()[1])  return false;
      if (box2.getMin()[2] > box1.getMax()[2])  return false;

      // No separating axis ... they must intersect
      return true;
   }

   /**
    * Tests if the given AABox and point intersect with each other. On an edge IS
    * considered intersection by this algorithm.
    *
    * @param box    the box to test
    * @param point  the point to test
    *
    * @return  true if the point is within the box's bounds; false otherwise
    */
   template<class DATA_TYPE>
   bool intersect( const AABox<DATA_TYPE>& box, const Point<DATA_TYPE, 3>& point )
   {
      // Look for a separating axis on each box for each axis
      if (box.getMin()[0] > point[0])  return false;
      if (box.getMin()[1] > point[1])  return false;
      if (box.getMin()[2] > point[2])  return false;

      if (point[0] > box.getMax()[0])  return false;
      if (point[1] > box.getMax()[1])  return false;
      if (point[2] > box.getMax()[2])  return false;

      // they must intersect
      return true;
   }

   /**
    * Tests if the given AABoxes intersect if moved along the given paths. Using
    * the AABox sweep test, the normalized time of the first and last points of
    * contact are found.
    *
    * @param box1          the first box to test
    * @param path1         the path the first box should travel along
    * @param box2          the second box to test
    * @param path2         the path the second box should travel along
    * @param firstContact  set to the normalized time of the first point of contact
    * @param secondContact set to the normalized time of the second point of contact
    *
    * @return  true if the boxes intersect at any time; false otherwise
    */
   template<class DATA_TYPE>
   bool intersect( const AABox<DATA_TYPE>& box1, const Vec<DATA_TYPE, 3>& path1,
                   const AABox<DATA_TYPE>& box2, const Vec<DATA_TYPE, 3>& path2,
                   DATA_TYPE& firstContact, DATA_TYPE& secondContact )
   {
      // Algorithm taken from Gamasutra's article, "Simple Intersection Test for
      // Games" - http://www.gamasutra.com/features/19991018/Gomez_3.htm
      //
      // This algorithm is solved from the frame of reference of box1

      // Get the relative path (in normalized time)
      Vec<DATA_TYPE, 3> path = path2 - path1;

      // The first time of overlap along each axis
      Vec<DATA_TYPE, 3> overlap1(DATA_TYPE(0), DATA_TYPE(0), DATA_TYPE(0));

      // The second time of overlap along each axis
      Vec<DATA_TYPE, 3> overlap2(DATA_TYPE(1), DATA_TYPE(1), DATA_TYPE(1));

      // Check if the boxes already overlap
      if (gmtl::intersect(box1, box2))
      {
         firstContact = secondContact = DATA_TYPE(0);
         return true;
      }

      // Find the possible first and last times of overlap along each axis
      for (int i=0; i<3; ++i)
      {
         if ((box1.getMax()[i] < box2.getMin()[i]) && (path[i] < DATA_TYPE(0)))
         {
            overlap1[i] = (box1.getMax()[i] - box2.getMin()[i]) / path[i];
         }
         else if ((box2.getMax()[i] < box1.getMin()[i]) && (path[i] > DATA_TYPE(0)))
         {
            overlap1[i] = (box1.getMin()[i] - box2.getMax()[i]) / path[i];
         }

         if ((box2.getMax()[i] > box1.getMin()[i]) && (path[i] < DATA_TYPE(0)))
         {
            overlap2[i] = (box1.getMin()[i] - box2.getMax()[i]) / path[i];
         }
         else if ((box1.getMax()[i] > box2.getMin()[i]) && (path[i] > DATA_TYPE(0)))
         {
            overlap2[i] = (box1.getMax()[i] - box2.getMin()[i]) / path[i];
         }
      }

      // Calculate the first time of overlap
      firstContact = Math::Max(overlap1[0], overlap1[1], overlap1[2]);

      // Calculate the second time of overlap
      secondContact = Math::Min(overlap2[0], overlap2[1], overlap2[2]);

      // There could only have been a collision if the first overlap time
      // occurred before the second overlap time
      return firstContact <= secondContact;
   }

   /**
    * Given an axis-aligned bounding box and a ray (or subclass thereof),
    * returns whether the ray intersects the box, and if so, \p tIn and
    * \p tOut are set to the parametric terms on the ray where the segment
    * enters and exits the box respectively.
    *
    * The implementation of this function comes from the book
    * <i>Geometric Tools for Computer Graphics</i>, pages 626-630.
    *
    * @note Internal function for performing an intersection test between an
    *       axis-aligned bounding box and a ray. User code should not call this
    *       function directly. It is used to capture the common code between
    *       the gmtl::Ray<T> and gmtl::LineSeg<T> overloads of
    *       gmtl::intersect() when intersecting with a gmtl::AABox<T>.
    */
   template<class DATA_TYPE>
   bool intersectAABoxRay(const AABox<DATA_TYPE>& box,
                          const Ray<DATA_TYPE>& ray, DATA_TYPE& tIn,
                          DATA_TYPE& tOut)
   {
      tIn  = -(std::numeric_limits<DATA_TYPE>::max)();
      tOut = (std::numeric_limits<DATA_TYPE>::max)();
      DATA_TYPE t0, t1;
      const DATA_TYPE epsilon(0.0000001);

      // YZ plane.
      if ( gmtl::Math::abs(ray.mDir[0]) < epsilon )
      {
         // Ray parallel to plane.
         if ( ray.mOrigin[0] < box.mMin[0] || ray.mOrigin[0] > box.mMax[0] )
         {
            return false;
         }
      }

      // XZ plane.
      if ( gmtl::Math::abs(ray.mDir[1]) < epsilon )
      {
         // Ray parallel to plane.
         if ( ray.mOrigin[1] < box.mMin[1] || ray.mOrigin[1] > box.mMax[1] )
         {
            return false;
         }
      }

      // XY plane.
      if ( gmtl::Math::abs(ray.mDir[2]) < epsilon )
      {
         // Ray parallel to plane.
         if ( ray.mOrigin[2] < box.mMin[2] || ray.mOrigin[2] > box.mMax[2] )
         {
            return false;
         }
      }

      // YZ plane.
      t0 = (box.mMin[0] - ray.mOrigin[0]) / ray.mDir[0];
      t1 = (box.mMax[0] - ray.mOrigin[0]) / ray.mDir[0];

      if ( t0 > t1 )
      {
         std::swap(t0, t1);
      }

      if ( t0 > tIn )
      {
         tIn = t0;
      }
      if ( t1 < tOut )
      {
         tOut = t1;
      }

      if ( tIn > tOut || tOut < DATA_TYPE(0) )
      {
         return false;
      }

      // XZ plane.
      t0 = (box.mMin[1] - ray.mOrigin[1]) / ray.mDir[1];
      t1 = (box.mMax[1] - ray.mOrigin[1]) / ray.mDir[1];

      if ( t0 > t1 )
      {
         std::swap(t0, t1);
      }

      if ( t0 > tIn )
      {
         tIn = t0;
      }
      if ( t1 < tOut )
      {
         tOut = t1;
      }

      if ( tIn > tOut || tOut < DATA_TYPE(0) )
      {
         return false;
      }

      // XY plane.
      t0 = (box.mMin[2] - ray.mOrigin[2]) / ray.mDir[2];
      t1 = (box.mMax[2] - ray.mOrigin[2]) / ray.mDir[2];

      if ( t0 > t1 )
      {
         std::swap(t0, t1);
      }

      if ( t0 > tIn )
      {
         tIn = t0;
      }
      if ( t1 < tOut )
      {
         tOut = t1;
      }

      if ( tIn > tOut || tOut < DATA_TYPE(0) )
      {
         return false;
      }

      return true;
   }

   /**
    * Given a line segment and an axis-aligned bounding box, returns whether
    * the line intersects the box, and if so, \p tIn and \p tOut are set to
    * the parametric terms on the line segment where the segment enters and
    * exits the box respectively.
    *
    * @since 0.4.11
    */
   template<class DATA_TYPE>
   bool intersect(const AABox<DATA_TYPE>& box, const LineSeg<DATA_TYPE>& seg,
                  unsigned int& numHits, DATA_TYPE& tIn, DATA_TYPE& tOut)
   {
      numHits = 0;
      bool result = intersectAABoxRay(box, seg, tIn, tOut);
      if (tIn < 0.0 && tOut > 1.0)
      {
	  return false;
      }
      if ( result )
      {
         // If tIn is less than 0, then the origin of the line segment is
         // inside the bounding box (not on an edge)but the endpoint is
         // outside.
         if ( tIn < DATA_TYPE(0) )
         {
            numHits = 1;
            tIn = tOut;
         }
         // If tIn is less than 0, then the origin of the line segment is
         // outside the bounding box but the endpoint is inside (not on an
         // edge).
         else if ( tOut > DATA_TYPE(1) )
         {
            numHits = 1;
            tOut = tIn;
         }
         // Otherwise, the line segement intersects the bounding box in two
         // places. tIn and tOut reflect those points of intersection.
         else
         {
            numHits = 2;
         }
      }

      return result;
   }

   /**
    * Given a line segment and an axis-aligned bounding box, returns whether
    * the line intersects the box, and if so, \p tIn and \p tOut are set to
    * the parametric terms on the line segment where the segment enters and
    * exits the box respectively.
    *
    * @since 0.4.11
    */
   template<class DATA_TYPE>
   bool intersect(const LineSeg<DATA_TYPE>& seg, const AABox<DATA_TYPE>& box,
                  unsigned int& numHits, DATA_TYPE& tIn, DATA_TYPE& tOut)
   {
      return intersect(box, seg, numHits, tIn, tOut);
   }

   /**
    * Given a ray and an axis-aligned bounding box, returns whether the ray
    * intersects the box, and if so, \p tIn and \p tOut are set to the
    * parametric terms on the ray where it enters and exits the box
    * respectively.
    *
    * @since 0.4.11
    */
   template<class DATA_TYPE>
   bool intersect(const AABox<DATA_TYPE>& box, const Ray<DATA_TYPE>& ray,
                  unsigned int& numHits, DATA_TYPE& tIn, DATA_TYPE& tOut)
   {
      numHits = 0;

      bool result = intersectAABoxRay(box, ray, tIn, tOut);

      if ( result )
      {
         // Ray is inside the box.
         if ( tIn < DATA_TYPE(0) )
         {
            tIn = tOut;
            numHits = 1;
         }
         else
         {
            numHits = 2;
         }
      }

      return result;
   }

   /**
    * Given a ray and an axis-aligned bounding box, returns whether the ray
    * intersects the box, and if so, \p tIn and \p tOut are set to the
    * parametric terms on the ray where it enters and exits the box
    * respectively.
    *
    * @since 0.4.11
    */
   template<class DATA_TYPE>
   bool intersect(const Ray<DATA_TYPE>& ray, const AABox<DATA_TYPE>& box,
                  unsigned int& numHits, DATA_TYPE& tIn, DATA_TYPE& tOut)
   {
      return intersect(box, ray, numHits, tIn, tOut);
   }

   /**
    * Tests if the given Spheres intersect if moved along the given paths. Using
    * the Sphere sweep test, the normalized time of the first and last points of
    * contact are found.
    *
    * @param sph1          the first sphere to test
    * @param path1         the path the first sphere should travel along
    * @param sph2          the second sphere to test
    * @param path2         the path the second sphere should travel along
    * @param firstContact  set to the normalized time of the first point of contact
    * @param secondContact set to the normalized time of the second point of contact
    *
    * @return  true if the spheres intersect; false otherwise
    */
   template<class DATA_TYPE>
   bool intersect(const Sphere<DATA_TYPE>& sph1, const Vec<DATA_TYPE, 3>& path1,
                  const Sphere<DATA_TYPE>& sph2, const Vec<DATA_TYPE, 3>& path2,
                  DATA_TYPE& firstContact, DATA_TYPE& secondContact)
   {
      // Algorithm taken from Gamasutra's article, "Simple Intersection Test for
      // Games" - http://www.gamasutra.com/features/19991018/Gomez_2.htm
      //
      // This algorithm is solved from the frame of reference of sph1

      // Get the relative path (in normalized time)
      const Vec<DATA_TYPE, 3> path = path2 - path1;

      // Get the vector from sph1's starting point to sph2's starting point
      const Vec<DATA_TYPE, 3> start_offset = sph2.getCenter() - sph1.getCenter();

      // Compute the sum of the radii
      const DATA_TYPE radius_sum = sph1.getRadius() + sph2.getRadius();

      // u*u coefficient
      const DATA_TYPE a = dot(path, path);

      // u coefficient
      const DATA_TYPE b = DATA_TYPE(2) * dot(path, start_offset);

      // constant term
      const DATA_TYPE c = dot(start_offset, start_offset) - radius_sum * radius_sum;

      // Check if they're already overlapping
      if (dot(start_offset, start_offset) <= radius_sum * radius_sum)
      {
         firstContact = secondContact = DATA_TYPE(0);
         return true;
      }

      // Find the first and last points of intersection
      if (Math::quadraticFormula(firstContact, secondContact, a, b, c))
      {
         // Swap first and second contacts if necessary
         if (firstContact > secondContact)
         {
            std::swap(firstContact, secondContact);
            return true;
         }
      }

      return false;
   }

   /**
    * Tests if the given AABox and Sphere intersect with each other. On an edge
    * IS considered intersection by this algorithm.
    *
    * @param box  the box to test
    * @param sph  the sphere to test
    *
    * @return  true if the items intersect; false otherwise
    */
   template<class DATA_TYPE>
   bool intersect(const AABox<DATA_TYPE>& box, const Sphere<DATA_TYPE>& sph)
   {
      DATA_TYPE dist_sqr = DATA_TYPE(0);

      // Compute the square of the distance from the sphere to the box
      for (int i=0; i<3; ++i)
      {
         if (sph.getCenter()[i] < box.getMin()[i])
         {
            DATA_TYPE s = sph.getCenter()[i] - box.getMin()[i];
            dist_sqr += s*s;
         }
         else if (sph.getCenter()[i] > box.getMax()[i])
         {
            DATA_TYPE s = sph.getCenter()[i] - box.getMax()[i];
            dist_sqr += s*s;
         }
      }

      return dist_sqr <= (sph.getRadius()*sph.getRadius());
   }

   /**
    * Tests if the given AABox and Sphere intersect with each other. On an edge
    * IS considered intersection by this algorithm.
    *
    * @param sph  the sphere to test
    * @param box  the box to test
    *
    * @return  true if the items intersect; false otherwise
    */
   template<class DATA_TYPE>
   bool intersect(const Sphere<DATA_TYPE>& sph, const AABox<DATA_TYPE>& box)
   {
      return gmtl::intersect(box, sph);
   }

   /**
    * intersect point/sphere.
    * @param point   the point to test
    * @param sphere  the sphere to test
    * @return true if point is in or on sphere
    */
   template<class DATA_TYPE>
   bool intersect( const Sphere<DATA_TYPE>& sphere, const Point<DATA_TYPE, 3>& point )
   {
      gmtl::Vec<DATA_TYPE, 3> offset = point - sphere.getCenter();
      DATA_TYPE dist = lengthSquared( offset ) - sphere.getRadius() * sphere.getRadius();

      // point is inside the sphere when true
      return  dist <= 0;
   }

   /**
    * intersect ray/sphere-shell (not volume).
    * only register hits with the surface of the sphere.
    * note: after calling this, you can find the intersection point with: ray.getOrigin() + ray.getDir() * t
    *
    * @param ray     the ray to test
    * @param sphere  the sphere to test
    * @return returns intersection point in t, and the number of hits
    * @return numhits, t0, t1 are undefined if return value is false
    */
   template<typename T>
   inline bool intersect( const Sphere<T>& sphere, const Ray<T>& ray, int& numhits, T& t0, T& t1 )
   {
      numhits = -1;

      // set up quadratic Q(t) = a*t^2 + 2*b*t + c
      const Vec<T, 3> offset = ray.getOrigin() - sphere.getCenter();
      const T a = lengthSquared( ray.getDir() );
      const T b = dot( offset, ray.getDir() );
      const T c = lengthSquared( offset ) - sphere.getRadius() * sphere.getRadius();



      // no intersection if Q(t) has no real roots
      const T discriminant = b * b - a * c;
      if (discriminant < 0.0f)
      {
         numhits = 0;
         return false;
      }
      else if (discriminant > 0.0f)
      {
         T root = Math::sqrt( discriminant );
         T invA = T(1) / a;
         t0 = (-b - root) * invA;
         t1 = (-b + root) * invA;

         // assert: t0 < t1 since A > 0

         if (t0 >= T(0))
         {
            numhits = 2;
            return true;
         }
         else if (t1 >= T(0))
         {
            numhits = 1;
            t0 = t1;
            return true;
         }
         else
         {
            numhits = 0;
            return false;
         }
      }
      else
      {
         t0 = -b / a;
         if (t0 >= T(0))
         {
            numhits = 1;
            return true;
         }
         else
         {
            numhits = 0;
            return false;
         }
      }
   }

   /** intersect LineSeg/Sphere-shell (not volume).
    * does intersection on sphere surface, point inside sphere doesn't count as an intersection
    * returns intersection point(s) in t
    * find intersection point(s) with: ray.getOrigin() + ray.getDir() * t
    * numhits, t0, t1 are undefined if return value is false
    */
   template<typename T>
   inline bool intersect( const Sphere<T>& sphere, const LineSeg<T>& lineseg, int& numhits, T& t0, T& t1 )
   {
      if (intersect( sphere, Ray<T>( lineseg ), numhits, t0, t1 ))
      {
         // throw out hits that are past 1 in segspace (off the end of the lineseg)
         while (0 < numhits && 1.0f < t0)
         {
            --numhits;
            t0 = t1;
         }
         if (2 == numhits && 1.0f < t1)
         {
            --numhits;
         }
         return 0 < numhits;
      }
      else
      {
         return false;
      }
   }

   /**
    * intersect lineseg/sphere-volume.
    * register hits with both the surface and when end points land on the interior of the sphere.
    * note: after calling this, you can find the intersection point with: ray.getOrigin() + ray.getDir() * t
    *
    * @param ray     the lineseg to test
    * @param sphere  the sphere to test
    * @return returns intersection point in t, and the number of hits
    * @return numhits, t0, t1 are undefined if return value is false
    */
   template<typename T>
   inline bool intersectVolume( const Sphere<T>& sphere, const LineSeg<T>& ray, int& numhits, T& t0, T& t1 )
   {
      bool result = intersect( sphere, ray, numhits, t0, t1 );
      if (result && numhits == 2)
      {
         return true;
      }
      // todo: make this faster (find an early out) since 1 or 0 hits is the common case.
      // volume test has some additional checks before we can throw it out because
      // one of both points may be inside the volume, so we want to return hits for those as well...
      else // 1 or 0 hits.
      {
         const T rsq = sphere.getRadius() * sphere.getRadius();
         const Vec<T, 3> dist = ray.getOrigin() - sphere.getCenter();
         const T a = lengthSquared( dist ) - rsq;
         const T b = lengthSquared( gmtl::Vec<T,3>(dist + ray.getDir()) ) - rsq;

         bool inside1 = a <= T( 0 );
         bool inside2 = b <= T( 0 );

         // one point is inside
         if (numhits == 1 && inside1 && !inside2)
         {
            t1 = t0;
            t0 = T(0);
            numhits = 2;
            return true;
         }
         else if (numhits == 1 && !inside1 && inside2)
         {
            t1 = T(1);
            numhits = 2;
            return true;
         }
         // maybe both points are inside?
         else if (inside1 && inside2) // 0 hits.
         {
            t0 = T(0);
            t1 = T(1);
            numhits = 2;
            return true;
         }
      }
      return result;
   }

   /**
    * intersect ray/sphere-volume.
    * register hits with both the surface and when the origin lands in the interior of the sphere.
    * note: after calling this, you can find the intersection point with: ray.getOrigin() + ray.getDir() * t
    *
    * @param ray     the ray to test
    * @param sphere  the sphere to test
    * @return returns intersection point in t, and the number of hits
    * @return numhits, t0, t1 are undefined if return value is false
    */
   template<typename T>
   inline bool intersectVolume( const Sphere<T>& sphere, const Ray<T>& ray, int& numhits, T& t0, T& t1 )
   {
      bool result = intersect( sphere, ray, numhits, t0, t1 );
      if (result && numhits == 2)
      {
         return true;
      }
      else
      {
         const T rsq = sphere.getRadius() * sphere.getRadius();
         const Vec<T, 3> dist = ray.getOrigin() - sphere.getCenter();
         const T a = lengthSquared( dist ) - rsq;

         bool inside = a <= T( 0 );

         // start point is inside
         if (inside)
         {
            t1 = t0;
            t0 = T(0);
            numhits = 2;
            return true;
         }
      }
      return result;
   }

   /**
    * Tests if the given plane and ray intersect with each other.
    *
    *  @param ray - the Ray
    *  @param plane - the Plane
    *  @param t - t gives you the intersection point:
    *         isect_point = ray.origin + ray.dir * t
    *
    *  @return true if the ray intersects the plane.
    *  @note If ray is parallel to plane: t=0, ret:true -> on plane, ret:false -> No hit
    */
   template<class DATA_TYPE>
   bool intersect( const Plane<DATA_TYPE>& plane, const Ray<DATA_TYPE>& ray, DATA_TYPE& t )
   {
      const DATA_TYPE eps(0.00001f);

      // t = -(n·P + d)
      Vec<DATA_TYPE, 3> N( plane.getNormal() );
      DATA_TYPE denom( dot(N,ray.getDir()) );
      if(gmtl::Math::abs(denom) < eps)    // Ray parallel to plane
      {
         t = 0;
         if(distance(plane, ray.mOrigin) < eps)     // Test for ray on plane
         { return true; }
         else
         { return false; }
      }
      t = dot( N, Vec<DATA_TYPE,3>(N * plane.getOffset() - ray.getOrigin()) ) / denom;

      return (DATA_TYPE)0 <= t;
   }

   /**
    * Tests if the given plane and lineseg intersect with each other.
    *
    *  @param ray - the lineseg
    *  @param plane - the Plane
    *  @param t - t gives you the intersection point:
    *         isect_point = lineseg.origin + lineseg.dir * t
    *
    *  @return true if the lineseg intersects the plane.
    */
   template<class DATA_TYPE>
   bool intersect( const Plane<DATA_TYPE>& plane, const LineSeg<DATA_TYPE>& seg, DATA_TYPE& t )
   {
      bool res(intersect(plane, static_cast<Ray<DATA_TYPE> >(seg), t));
      return res && t <= (DATA_TYPE)1.0;
   }

   /**
    * Tests if the given triangle and ray intersect with each other.
    *
    *  @param tri - the triangle (ccw ordering)
    *  @param ray - the ray
    *  @param u,v - tangent space u/v coordinates of the intersection
    *  @param t - an indicator of the intersection location
    *  @post t gives you the intersection point:
    *         isect = ray.dir * t + ray.origin
    *  @return true if the ray intersects the triangle.
    *  @see from http://www.acm.org/jgt/papers/MollerTrumbore97/code.html
    */
   template<class DATA_TYPE>
   bool intersect( const Tri<DATA_TYPE>& tri, const Ray<DATA_TYPE>& ray,
                        float& u, float& v, float& t )
   {
      const float EPSILON = (DATA_TYPE)0.00001f;
      Vec<DATA_TYPE, 3> edge1, edge2, tvec, pvec, qvec;
      float det,inv_det;

      /* find vectors for two edges sharing vert0 */
      edge1 = tri[1] - tri[0];
      edge2 = tri[2] - tri[0];

      /* begin calculating determinant - also used to calculate U parameter */
      gmtl::cross( pvec, ray.getDir(), edge2 );

      /* if determinant is near zero, ray lies in plane of triangle */
      det = gmtl::dot( edge1, pvec );

      if (det < EPSILON)
         return false;

      /* calculate distance from vert0 to ray origin */
      tvec = ray.getOrigin() - tri[0];

      /* calculate U parameter and test bounds */
      u = gmtl::dot( tvec, pvec );
      if (u < 0.0 || u > det)
         return false;

      /* prepare to test V parameter */
      gmtl::cross( qvec, tvec, edge1 );

      /* calculate V parameter and test bounds */
      v = gmtl::dot( ray.getDir(), qvec );
      if (v < 0.0 || u + v > det)
         return false;

      /* calculate t, scale parameters, ray intersects triangle */
      t = gmtl::dot( edge2, qvec );
      inv_det = ((DATA_TYPE)1.0) / det;
      t *= inv_det;
      u *= inv_det;
      v *= inv_det;

      // test if t is within the ray boundary (when t >= 0)
      return t >= (DATA_TYPE)0;
   }

   /**
    * Tests if the given triangle intersects with the given ray, from both sides.
    *
    * @param tri The triangle (ccw ordering).
    * @param ray The ray.
    * @param u Tangent space u-coordinate of the intersection.
    * @param v Tangent space v-coordinate of the intersection.
    * @param t An indicator of the intersection location.
    *
    * @post \p t gives you the intersection point:
    *       \code isect = ray.dir * t + ray.origin \endcode
    *
    * @return true if the ray intersects the triangle.
    *
    * @see from http://www.acm.org/jgt/papers/MollerTrumbore97/code.html
    */
   template<class DATA_TYPE>
   bool intersectDoubleSided(const Tri<DATA_TYPE>& tri, const Ray<DATA_TYPE>& ray,
                             DATA_TYPE& u, DATA_TYPE& v, DATA_TYPE& t)
   {
      const DATA_TYPE EPSILON = (DATA_TYPE)0.00001f;
      Vec<DATA_TYPE, 3> edge1, edge2, tvec, pvec, qvec;
      DATA_TYPE det, inv_det;

      // Find vectors for two edges sharing vert0.
      edge1 = tri[1] - tri[0];
      edge2 = tri[2] - tri[0];

      // Begin calculating determinant - also used to calculate U parameter.
      gmtl::cross(pvec, ray.getDir(), edge2);

      // If determinant is near zero, ray lies in plane of triangle.
      det = gmtl::dot( edge1, pvec );

      if ( det < EPSILON && det > -EPSILON )
      {
         return false;
      }

      // calculate distance from vert0 to ray origin>
      tvec = ray.getOrigin() - tri[0];

      // Calc inverse deteriminant.
      inv_det = ((DATA_TYPE)1.0) / det; 

      // Calculate U parameter and test bounds.
      u =  inv_det * gmtl::dot(tvec, pvec);
      if ( u < 0.0 || u > 1.0 )
      {
         return false;
      }

      // Prepare to test V parameter.
      gmtl::cross(qvec, tvec, edge1);

      // Calculate V parameter and test bounds.
      v = inv_det * gmtl::dot(ray.getDir(), qvec);
      if (v < 0.0 || u + v > 1.0)
      {
         return false;
      }

      // Calculate t, scale parameters, ray intersects triangle.
      t = inv_det * gmtl::dot(edge2, qvec);

      // Test if t is within the ray boundary (when t >= 0).
      return t >= (DATA_TYPE)0;
   }

   /**
    * Tests if the given triangle and line segment intersect with each other.
    *
    *  @param tri - the triangle (ccw ordering)
    *  @param lineseg - the line segment
    *  @param u,v - tangent space u/v coordinates of the intersection
    *  @param t - an indicator of the intersection point
    *  @post t gives you the intersection point:
    *         isect = lineseg.getDir() * t + lineseg.getOrigin()
    *
    *  @return true if the line segment intersects the triangle.
    */
   template<class DATA_TYPE>
   bool intersect( const Tri<DATA_TYPE>& tri, const LineSeg<DATA_TYPE>& lineseg,
                   DATA_TYPE& u, DATA_TYPE& v, DATA_TYPE& t )
   {
      const DATA_TYPE eps = (DATA_TYPE)0.0001010101;
      DATA_TYPE l = length( lineseg.getDir() );
      if (eps < l)
      {
         Ray<DATA_TYPE> temp( lineseg.getOrigin(), lineseg.getDir() );
         bool result = intersect( tri, temp, u, v, t );
         return result && t <= (DATA_TYPE)1.0;
      }
      else
      {  return false; }
   }
}



#endif
