// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_PP_FORMS_INCLUDED
#define IFC_PP_FORMS_INCLUDED

#include "index-utils.hxx"

namespace ifc {
    // Classification of pre-syntactic forms used to group
    // pp-tokens during, and out of, translation phases 1-4.
    enum class FormSort : uint8_t {
        Identifier,    // identifier form
        Number,        // number form
        Character,     // built-in or ud-suffixed character literal form
        String,        // built-int or ud-suffixed string literal form
        Operator,      // operator or punctuator form
        Keyword,       // implementation-defined keywords form - e.g. 'module', 'export', 'import'.
        Whitespace,    // whitespace forms - including comments
        Parameter,     // reference to a macro parameter form
        Stringize,     // stringification form
        Catenate,      // catenation form
        Pragma,        // _Pragma form
        Header,        // header name
        Parenthesized, // parenthesized form
        Tuple,         // A sequence of zero or more forms
        Junk,          // invalid form
        Count
    };

    struct FormIndex : index_like::Over<FormSort> {
        using Over<FormSort>::Over;
    };
} // namespace ifc

#endif // IFC_PP_FORMS_INCLUDED
