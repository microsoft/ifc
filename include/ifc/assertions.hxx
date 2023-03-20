//
// Copyright Microsoft.
//

#ifndef ASSERTIONS_HXX_INCLUDED
#define ASSERTIONS_HXX_INCLUDED

#include <cassert>

// Consider getting rid of IFCASSERT and just use "assert" directly.

#ifndef IFCASSERT
#define IFCASSERT(ex) assert(ex)
#endif // IFCASSERT

/* Note that a consumer of this library that wants to "handle" assertions in a
   different way should override _wassert (on Windows) or __assert on Linux/Mac. */

#endif // ASSERTIONS_HXX_INCLUDED
