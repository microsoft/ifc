//
// Copyright Microsoft.
//

#ifndef IFC_ASSERTIONS_HXX_INCLUDED
#define IFC_ASSERTIONS_HXX_INCLUDED

void ifc_assert(char const* message, char const* file, int line);

// IFCVERIFY is always on.
#define IFCVERIFY(expression) (void)((not not(expression)) or (ifc_assert(#expression, __FILE__, __LINE__), 0))

#ifdef NDEBUG
#define IFCASSERT(expression) ((void)0)
#else
#define IFCASSERT(expression) (void)((not not(expression)) or (ifc_assert(#expression, __FILE__, __LINE__), 0))
#endif // NDEBUG

/* Note that a consumer of this library that wants to "handle" assertions in a
   different way should override ifc_assert. */

#endif // IFC_ASSERTIONS_HXX_INCLUDED
