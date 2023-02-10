//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.
//

#ifndef UTILITY_RUNTIME_ASSERTIONS_INCLUDED
#define UTILITY_RUNTIME_ASSERTIONS_INCLUDED

#include "basic-types.hxx"

// DASSERT is a no-op in release build
// DASSERT_ASSUME tells BE that 'ex' is always true and it impacts codegen
//   Note that if 'ex' can be false, the codegen may be incorrect and cause runtime failures like AV.
//   So if the intention is to see whether the assertion may fail, use DASSERT instead.
// RASSERT is an assertion in both debug and release build

#if VERSP_RELEASE
#define DASSERT(ex)         (void())
#define DASSERT_ASSUME(ex)  __assume(ex)
#ifdef __INTELLISENSE__
#define RASSERT(ex)         ((ex) ? void() : rassertfe(__FILE__, static_cast<msvc::LineNumber>(__LINE__)))
#else
#define RASSERT(ex)         ((ex) ? void() : rassertfe(__FILE__, msvc::LineNumber{__LINE__}))
#endif
#elif VERSP_TEST || VERSP_DEBUG
#ifdef __INTELLISENSE__
#define DASSERT(ex)         ((ex) ? void() : assertfe(#ex, __FILE__, static_cast<msvc::LineNumber>(__LINE__)))
#else
#define DASSERT(ex)         ((ex) ? void() : assertfe(#ex, __FILE__, msvc::LineNumber{__LINE__}))
#endif
#define DASSERT_ASSUME(ex)  DASSERT(ex)
#define RASSERT(ex)         DASSERT(ex)
#else
#  error "no production version selected"
#endif

// This is used to workaround a false positive of C5262 which doesn't take [[noreturn]] function into account.
// Use this variant if RASSERT() is the last statement in a case statement and followed by a case label or a default label.
#define RASSERT_IN_CASE(ex) RASSERT(ex); [[fallthrough]]

// This is used to suppress warning C4189: local variable is initialized but not referenced.
// This macro should only be used when the local variable is only referenced in DASSERT.
// The parameter should be the identifier of the local variable.
#if VERSP_RELEASE
#define UNREFERENCED_LOCAL_VARIABLE_IN_RELEASE(id) (id)
#else
#define UNREFERENCED_LOCAL_VARIABLE_IN_RELEASE(id) (void())
#endif

//  -- The principal dynamic assertion macro, DASSERT, forward to this function
//     in non-release mode.  Each component is expected to provide their own
//     (private) implementation of this function.  Another way to think it is
//     as overriding a virtual function.
//     The general semantics is to print an assertion failure message for an expression
//     the string reprsentation of which is 'expr', at line 'srcline', in input source
//     file designated by 'srcfn'.
void assertfe(const char* exp, const char *srcfn, msvc::LineNumber srcline);
[[noreturn]] void rassertfe(const char* srcfn, msvc::LineNumber srcline);

#endif // UTILITY_RUNTIME_ASSERTIONS_INCLUDED
