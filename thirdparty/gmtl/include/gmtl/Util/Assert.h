// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef GMTL_ASSERT_H
#define GMTL_ASSERT_H

// -- VERY simple assertion stuff -- //
#ifdef _DEBUG
#   include <assert.h>
#   define gmtlASSERT(val) assert((val))
#else
#   define gmtlASSERT(val) ((void)0)
#endif


#endif
