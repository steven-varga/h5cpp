// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_COMPARITORS_H_
#define _GMTL_COMPARITORS_H_

// This file contains helper comparitors
//
// They can be used as comparison functors for STL container
// operations (or for anything else you may want to use them for...)
//

#include <gmtl/Vec3.h>
#include <gmtl/Point3.h>

namespace gmtl
{
   // Allows for the comparison of projected point distances
   // onto a given vector
   struct CompareIndexPointProjections
   {
   public:
      CompareIndexPointProjections() : points(NULL)
      {;}

      bool operator()(const unsigned x, const unsigned y)
      {
         float xVal = sortDir.dot((*points)[x]);
         float yVal = sortDir.dot((*points)[y]);

         return (xVal < yVal);
      }

      const std::vector<Point3>* points;
      gmtl::Vec3                 sortDir;       // Direction to sort by
   };
};

#endif

