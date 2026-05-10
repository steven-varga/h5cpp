// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_HELPERS_H_
#define _GMTL_HELPERS_H_

#include <gmtl/Config.h>

// Helper classes
namespace gmtl
{
namespace helpers
{

struct ConstructorCounter
{
   unsigned mCount;

   ConstructorCounter()
   { mCount = 0; }

   void inc()
   { mCount += 1; }
   unsigned get()
   { return mCount; }
};

// Global version of the contructor counters
//#ifdef GMTL_COUNT_CONSTRUCT_CALLS

//ConstructorCounter VecConstructCounter;   // Counter for vec objects
inline ConstructorCounter* VecCtrCounterInstance()
{
   static ConstructorCounter vec_counter;
   return &vec_counter;
}

//#endif


}
}

#endif


