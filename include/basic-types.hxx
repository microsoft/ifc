//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft Corp 1984-2016. All rights reserved.
//

#ifndef UTILITY_BASIC_TYPES_H
#define UTILITY_BASIC_TYPES_H

#include <stdint.h>

#include "enum-utils.hxx"

namespace msvc {
    // Evaluate to true if the type parameter is a character type.
    template<typename T>
    concept CharType = std::is_same_v<T, char> ||
        std::is_same_v<T, unsigned char> ||
        std::is_same_v<T, signed char> ||
        std::is_same_v<T, char8_t>;

    // Data type for line numbers.  Various places in the front-end code base assume a 32-bit integer type.
    // Make it distinct so we can keep track of the eggs.
    enum LineNumber : int32_t {
        Max = 0x00ffffff,
    };

    // Return the next line number.
    constexpr LineNumber next(LineNumber n)
    {
        return LineNumber{ bits::rep(n) + 1 };
    }

    // Return the previous line number.
    constexpr LineNumber previous(LineNumber n)
    {
        return LineNumber{ bits::rep(n) - 1 };
    }

    // Data type for column numbers.  Various places in the front-end code base assume a 32-bit integer type.
    // Make it distinct so we can keep track of the eggs.
    enum class ColumnNumber : int32_t {
        Invalid = -1
    };

    // Return the next column number.
    constexpr ColumnNumber next(ColumnNumber n)
    {
        return ColumnNumber{ bits::rep(n) + 1 };
    }

    // Simple version of std::pair without the templated ctors and SFINAE. When only aggregate init
    // is required, this is easier on build time.
    template <typename T1, typename T2>
    struct pair
    {
        T1 first;
        T2 second;
    };
}

#endif // UTILITY_BASIC_TYPES_H
